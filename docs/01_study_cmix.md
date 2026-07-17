# Study: CMIX

> Part of **HyperCore Phase 1 — Research & Planning**

---

## What Is CMIX?

CMIX (by Byron Knoll) is the current state-of-the-art in lossless compression ratio.
It consistently ranks #1 on benchmarks such as the Hutter Prize.

It compresses data one **bit at a time**, using an enormous ensemble of prediction models
fed into a context mixer, producing a final probability fed to an arithmetic coder.

---

## Architecture

```
Input Bits
    │
    ▼
[Preprocessors] — detect & transform text, executables, images
    │
    ▼
[Model Ensemble] — 2,000+ statistical models running in parallel
  ├─ Byte-order models
  ├─ Word models
  ├─ Sparse context models
  ├─ Domain-specific models (text, binary, image)
  └─ Neural network predictors
    │
    ▼
[Context Mixer] — weighted averaging of model outputs
  └─ SSE (Secondary Symbol Estimation) refinement
    │
    ▼
[Arithmetic Coder] → compressed bitstream
```

---

## Key Concepts

| Concept | Description |
|---|---|
| **Context Mixing** | Combines outputs from many models via weighted average; weights adapt in real time |
| **SSE** | A second-stage calibrator that adjusts the mixer's output probability using recent history |
| **Non-stationarity** | Models decay their counters so they can adapt to changing data statistics |
| **Bit-level processing** | Operates at the bit level, not byte level — finer grained but entirely sequential |

---

## CMIX's Fatal Limitations for HyperCore

| Limitation | Impact |
|---|---|
| Purely sequential — every bit depends on the previous | Cannot be parallelized at all |
| 32 GB+ RAM required | Impractical for most real-world workloads |
| No GPU utilization | Modern GPU compute power is completely wasted |
| Compresses ~1 MB/min on modern CPUs | Orders of magnitude too slow for practical use |
| Single global model — no semantic awareness | Natural language mixed with source code → poor prediction for both |

---

## Lessons for HyperCore

- The **context mixing** principle is powerful and worth preserving — but the mixer must be
  fast and selective.
- **SSE** is an effective final calibration stage — we should implement a lightweight analogue.
- CMIX proves that throwing more models at a problem improves ratios — HyperCore must be
  smarter about *which* models to evaluate per context (adaptive selection).
- The **global model** weakness directly motivates HyperCore's **Context Islands** design.
- The arithmetic coder can be replaced by parallel rANS with minimal ratio loss and massive
  speed gain.

---

## Reference

- Source: https://github.com/byronknoll/cmix
- Benchmark: http://prize.hutter1.net/
- v21 uses 2,000+ models, requires 32 GB RAM, compresses enwik8 at ~1 MB/min
