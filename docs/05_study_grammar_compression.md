# Study: Grammar-Based Compression

> Part of **HyperCore Phase 1 — Research & Planning**

---

## What Is Grammar Compression?

Grammar compression represents an input string as a **Straight-Line Program (SLP)** —
a context-free grammar that generates exactly one string: the input.

Repeated patterns become reusable **non-terminal symbols** (grammar rules), reducing
redundancy before entropy coding begins.

**Example:**
```
Input:   "abcabc"

Grammar:
  S → A A
  A → "abc"

Result:  2 rules instead of 6 raw characters.
         After entropy coding: significant size reduction.
```

---

## Why Not Just Use a Dictionary?

| Approach | Handles |
|---|---|
| Dictionary (LZ77/LZW) | Repeated substrings within a sliding window |
| Grammar compression | **Hierarchical, nested** repetitions across the entire file |

Grammar compression captures **long-range dependencies** that windowed dictionaries miss.
It is especially effective on highly structured text: source code, XML, JSON, logs.

---

## Key Algorithms

### Sequitur (Online)

- Processes text **left-to-right, incrementally** — works as a stream.
- Maintains two invariants:
  1. **Digram Uniqueness:** No adjacent symbol pair appears twice in the grammar.
  2. **Rule Utility:** Every rule created must be referenced at least twice.
- Good for real-time / streaming scenarios.
- Slightly lower compression ratios than Re-Pair in practice.

### Re-Pair (Offline, Greedy)

- Reads the **entire input first** — requires a full pass.
- Repeatedly finds and replaces the **most frequent adjacent pair** with a new symbol.
- Continues until no pair appears more than once.
- Generally achieves **better compression ratios** than Sequitur.
- High memory overhead (4–8× input size) — must be managed carefully for large files.
- **Frequency counting is a parallel histogram problem — GPU-friendly.**

---

## Sequitur vs Re-Pair

| Feature | Sequitur | Re-Pair |
|---|---|---|
| Type | Online (streaming) | Offline (full input required) |
| Strategy | Incremental invariant maintenance | Greedy, most-frequent-first |
| Compression quality | Good | Better |
| Memory | Lower | High (4–8× input) |
| GPU acceleration | Difficult (sequential) | Frequency phase is GPU-parallelizable |
| HyperCore use case | Streaming / real-time path | Primary block compression |

---

## Applying Grammar Compression to HyperCore

### Pattern Classes by Context Island

| Island Type | Grammar Rule Focus | Examples |
|---|---|---|
| **Natural Language** | Word n-grams, common phrases | "in order to", "as well as" |
| **Source Code** | Syntactic constructs | `for (int i = 0; i < n; i++)`, `if () {}` |
| **JSON** | Key-value schemata, structural patterns | `{"status": VALUE, "id": VALUE}` |
| **XML/HTML** | Tag pairs, attribute patterns | `<div class="container">...</div>` |
| **CSV** | Row templates, field type patterns | `field1,field2,field3\n` |
| **Log** | Timestamp + level prefixes | `[2026-07-17 21:00:00] [INFO] ` |
| **Markdown** | Headers, emphasis, code fences | `## Title\n`, ` ```lang\n` |

---

## Grammar + Context Islands Synergy

The grammar system is specialized **per island**. A JSON rule will never pollute a natural
language model, and vice versa. This is one of HyperCore's most powerful differentiators
over traditional compressors that use a single global grammar.

```
Context Island: Source Code
  Grammar Rules:
    R1 → "std::vector<"
    R2 → ">::iterator "
    R3 → "for (size_t i = 0; i < "
    R4 → "; ++i) {"
    ...
  These rules are only used within this island's entropy model.
```

---

## Implementation Plan for HyperCore

### Primary Path: Re-Pair per Block (CPU, Phase 5)

```
1. Receive tokenized block from GPU stage
2. Build adjacency frequency table (can be seeded by GPU histogram)
3. While max_pair_freq > threshold:
   a. Find most frequent adjacent pair
   b. Assign new non-terminal symbol
   c. Replace all occurrences in token stream
   d. Update frequency table incrementally
4. Output: compressed token stream + grammar table
5. Grammar table is stored in .htx archive for decoder
```

### Streaming Path: Sequitur (Phase 13 — Streaming Mode)

```
1. Process token stream left-to-right
2. Maintain digram hash table
3. On each new symbol pair:
   a. Check if pair exists in table
   b. If yes: create rule, replace all occurrences
   c. If no: add to table
4. Output rules incrementally
```

### GPU Acceleration of Re-Pair (Phase 8)

The bottleneck in Re-Pair is the **frequency counting** step. This maps directly to:

```cuda
// GPU kernel: count all adjacent pairs
__global__ void count_pairs(uint32_t* tokens, uint64_t* freq_table, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n - 1) {
        uint64_t pair = ((uint64_t)tokens[i] << 32) | tokens[i+1];
        atomicAdd(&freq_table[hash(pair)], 1);
    }
}
```

Top-K selection of most frequent pairs uses GPU radix sort (CUB).

---

## NP-Hardness Note

Finding the **smallest grammar** for a string is NP-hard.
Re-Pair and Sequitur are both **greedy approximations** that run in polynomial time
and achieve compression ratios within a small constant factor of optimal.
This is well-established and acceptable for HyperCore's goals.

---

## References

- Re-Pair: Larsson & Moffat, "Offline Dictionary-Based Compression" (DCC 1999)
- Sequitur: Craig Nevill-Manning, "Inferring Sequential Structure" (PhD thesis, 1996)
- Grammar compression survey: arXiv:1207.1270
- Wikipedia — Grammar-based codes: https://en.wikipedia.org/wiki/Grammar-based_codes
