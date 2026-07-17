# Architecture Design

> Part of **HyperCore Phase 1 — Research & Planning**

---

## High-Level Pipeline

```
┌─────────────────────────────────────────────────────────────────────┐
│                       HyperCore Pipeline                            │
│                                                                     │
│  INPUT FILE                                                         │
│     │                                                               │
│     ▼                                                               │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  STAGE 0: FILE ANALYSIS (CPU)                                │   │
│  │  Encoding detection · file type · block partitioning         │   │
│  └──────────────────────────────────────────────────────────────┘   │
│     │                                                               │
│     ▼  [transfer to GPU via pinned DMA]                             │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  STAGE 1: GPU PREPROCESSING                                  │   │
│  │  UTF-8 validation → tokenization → char classification       │   │
│  │  → region tagging (Context Island labels per token)          │   │
│  └──────────────────────────────────────────────────────────────┘   │
│     │                              │                                │
│     │  [GPU, per-island]           │  [CPU, concurrent]             │
│     ▼                              ▼                                │
│  ┌─────────────────────┐  ┌────────────────────────────────────┐   │
│  │ STAGE 2: PATTERN    │  │ STAGE 2B: CONTEXT ISLAND SETUP     │   │
│  │ MINING (GPU)        │  │ Assign predictor sets per island   │   │
│  │ · Rolling hashes    │  │ Initialize per-island models       │   │
│  │ · N-gram counts     │  │ Prepare grammar builders           │   │
│  │ · Phrase mining     │  └────────────────────────────────────┘   │
│  │ · Dict candidates   │                                            │
│  └─────────────────────┘                                            │
│     │                                                               │
│     ▼  [results back to CPU]                                        │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  STAGE 3: DICTIONARY & GRAMMAR OPTIMIZATION (CPU)            │   │
│  │  Prune candidates → finalize 3-tier dictionary               │   │
│  │  Run Re-Pair grammar compression per island                  │   │
│  │  Build probability tables for entropy coder                  │   │
│  └──────────────────────────────────────────────────────────────┘   │
│     │                                                               │
│     ▼                                                               │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  STAGE 4: PREDICTION ENGINE (CPU + optional GPU)             │   │
│  │  Adaptive model selection per context                        │   │
│  │  Context mixer (weighted logistic regression)                │   │
│  │  SSE calibration stage                                       │   │
│  └──────────────────────────────────────────────────────────────┘   │
│     │                                                               │
│     ▼  [back to GPU]                                                │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  STAGE 5: ENTROPY CODING (GPU + CPU)                         │   │
│  │  Multi-stream rANS on GPU  (one stream per block)            │   │
│  │  tANS on CPU  (metadata, headers, grammar tables)            │   │
│  └──────────────────────────────────────────────────────────────┘   │
│     │                                                               │
│     ▼                                                               │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  STAGE 6: ARCHIVE ASSEMBLY (CPU)                             │   │
│  │  Write .htx: header + block index + compressed blocks        │   │
│  │  + dictionaries + grammar tables + metadata + checksums      │   │
│  └──────────────────────────────────────────────────────────────┘   │
│     │                                                               │
│     ▼                                                               │
│  OUTPUT:  filename.htx                                              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Context Islands — Detailed Design

### What Is a Context Island?

A **Context Island** is a contiguous region of text that has a uniform semantic type.
Each island is compressed with a specialized predictor set tuned to that content type,
instead of a single global model that dilutes prediction quality across all types.

### Detection Heuristics

| Island Type | Detection Signal |
|---|---|
| **Natural Language** | High alpha ratio, sentence boundaries, common word frequency |
| **Source Code** | Keywords (`if`, `for`, `def`, `class`), brace density, indentation regularity |
| **JSON** | `{`, `"key":`, balanced brackets, UTF-8 only |
| **XML/HTML** | `<tag>`, balanced tags, attribute patterns |
| **CSV** | Regular delimiter count per line, quoted fields |
| **Markdown** | `#` headers, `*`/`_` emphasis, `` ` `` code fences |
| **Log** | Timestamp prefix, level labels, repeated template prefix |
| **Binary** | High non-printable ratio → fallback to general binary mode |

### Predictor Assignment Per Island

| Island | Active Predictors |
|---|---|
| Natural Language | Word model · phrase model · char model |
| Source Code | Grammar model · structural model · char model |
| JSON | Structural model · key model · value model |
| XML/HTML | Tag model · attribute model · text model |
| CSV | Record model · field model |
| Log | Template model · timestamp model |
| Markdown | Structure model · word model |

---

## Three-Tier Dictionary System

```
Tier 1: Global Dictionary  (64 KB – 256 KB)
  ├─ Pre-trained from large corpora or built during full-file scan
  ├─ Universal high-frequency phrases: "function", "return", "the", etc.
  └─ Shared across all blocks, stored once in the archive header

Tier 2: Document Dictionary  (8 KB – 64 KB)
  ├─ Built per input file during pattern mining stage
  ├─ Document-specific frequent patterns and domain vocabulary
  └─ Stored once per archive

Tier 3: Block Dictionary  (1 KB – 8 KB)
  ├─ Built per 64 KB – 256 KB block
  ├─ Hyper-local frequent patterns in this specific block
  └─ Stored per block in the block index
```

The encoder selects the **cheapest representation** for each token across all three tiers.

---

## Parallel CPU–GPU Concurrency Model

```
Time ──────────────────────────────────────────────────────────────▶

CPU:  ┌────────────┐  ┌────────────────────────┐  ┌──────────────┐
      │ Analyze    │  │ Optimize Dict/Grammar N │  │ Archive N-2  │
      │ Block N+1  │  │ Mix Probabilities N-1   │  │              │
      └────────────┘  └────────────────────────┘  └──────────────┘

GPU:  ┌──────────────────────┐  ┌────────────────────┐
      │ Preprocess + Mine    │  │ rANS Encode Block  │
      │ Block N              │  │ N-1                │
      └──────────────────────┘  └────────────────────┘
```

- Uses **CUDA Streams** for GPU pipeline overlap.
- Uses **CPU thread pools** (TBB or std::execution) for concurrent CPU tasks.
- CPU and GPU are never both idle at the same time.

---

## .htx Archive Format (Preliminary)

```
[MAGIC]         4 bytes   "HTX\x01"
[VERSION]       2 bytes   format version
[FLAGS]         2 bytes   feature flags (GPU path used, islands enabled, etc.)
[ORIG_SIZE]     8 bytes   original file size in bytes
[NUM_BLOCKS]    4 bytes   number of compressed blocks
[DICT_OFFSET]   8 bytes   byte offset to global dictionary
[GRAMMAR_OFFSET]8 bytes   byte offset to global grammar table
[CHECKSUM]      32 bytes  BLAKE3 of original file

[BLOCK INDEX]   NUM_BLOCKS × entry:
  [BLOCK_OFFSET]  8 bytes
  [BLOCK_SIZE]    4 bytes
  [ORIG_SIZE]     4 bytes
  [ISLAND_MAP]    N bytes (per-block island layout)
  [LOCAL_DICT]    variable (block-level dictionary)

[BLOCK DATA]    ... (rANS-encoded bitstreams)
[GLOBAL DICT]   ... (3-tier dictionary storage)
[GRAMMAR TABLE] ... (per-island grammar rules)
[METADATA]      ... (timestamps, tool version, OS, etc.)
[CRC64]         8 bytes  archive integrity check
```

---

## Component Dependency Graph

```
File Analysis
    │
    ▼
GPU Preprocessing ──────────────────────────────────────────┐
    │                                                        │
    ▼                                                        ▼
Pattern Mining (GPU) ──────► Context Island Setup (CPU)
    │                                     │
    ▼                                     ▼
Dict & Grammar Optimization (CPU) ◄───────┘
    │
    ▼
Prediction Engine (CPU)
    │
    ▼
Entropy Coding (GPU + CPU)
    │
    ▼
Archive Assembly (CPU)
```
