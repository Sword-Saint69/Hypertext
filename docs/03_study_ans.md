# Study: Asymmetric Numeral Systems (ANS)

> Part of **HyperCore Phase 1 — Research & Planning**

---

## What Is ANS?

Asymmetric Numeral Systems (Jarek Duda, 2009) is the family of entropy coders that has
replaced arithmetic coding in most modern production compressors — Zstandard, Apple LZFSE,
CRAM, and others. It encodes a stream of symbols into a single large integer, advancing
the "state" with each symbol according to its probability.

---

## The Core Idea

In standard binary: `x' = 2x + bit`  (appending a bit to x)

In ANS:  `x' = f(x, s, p(s))`

Encoding symbol `s` with probability `p(s)` advances the state in a way that expends
exactly `log₂(1/p(s))` bits of information — achieving **near-entropy compression**.

---

## rANS vs tANS

| Feature | rANS | tANS |
|---|---|---|
| Implementation | Multiply/divide on the hot path | Precomputed finite-state machine (lookup table) |
| Speed | Fast, but division is expensive | Extremely fast — just table lookups + bit shifts |
| Parallelism | **Better for GPU** — multiple streams interleave cleanly | Harder to parallelize — table state is shared |
| Precision | Arbitrary precision via integer scaling | Fixed table size (typically 2^12 states) |
| Used by | DietGPU, custom research codecs | Zstd, LZFSE, CRAM |

> **HyperCore Decision:**
> - Use **rANS** for the GPU pipeline — multi-stream, one stream per CUDA thread block.
> - Use **tANS** for the CPU path — metadata, headers, and fallback encoding.

---

## GPU rANS — How Parallelism Works

Standard rANS is sequential: each symbol's output state depends on the previous. To parallelize:

### Multi-Stream rANS
Run *N* independent rANS streams across *N* blocks simultaneously. Each stream is fully
independent — trivially parallel.

### Interleaved rANS
Even within one block, interleave *k* streams (k = 4 or 8). The encoder alternates between
them symbol-by-symbol. The decoder processes in reverse with the same interleaving.

### Decoding Parallelism
rANS decodes in reverse — given independent streams, decoding is **embarrassingly parallel**.
This is the strategy used by DietGPU (250–600 GB/s on A100-class hardware).

---

## ANS Mathematical Properties We Exploit

### Renormalization
When state `x` grows too large, emit bits to output and rescale.
These bit-emitting intervals can be **precomputed**, enabling GPU branch-free execution.

### Quantized Probability Tables
Symbol frequencies are quantized to [0, M] where M = 2^k (typically k = 12, so M = 4096).
Fast integer arithmetic. No floating point on GPU hot path.

---

## Comparison: Huffman vs Arithmetic Coding vs ANS

| Feature | Huffman | Arithmetic Coding | ANS (rANS/tANS) |
|---|---|---|---|
| Speed | Very Fast | Slow | Very Fast |
| Compression Ratio | Moderate (integer bit lengths) | Excellent | Excellent |
| Parallelism | Trivial (per-symbol) | Hard (serial state) | Good (multi-stream) |
| Mechanism | Tree-based | Interval subdivision | State-machine / number system |
| GPU-friendly | Yes (simple) | No | Yes (rANS) |

---

## Implementation Plan for HyperCore

```
GPU path (rANS):
  1. Build quantized symbol frequency table on GPU (histogram kernel)
  2. Normalize frequencies to M = 4096
  3. Precompute renormalization tables
  4. Launch N CUDA thread blocks, each encoding one stream
  5. Interleave k=8 streams within each block
  6. Output bitstream segments to device memory
  7. Concatenate segments to host

CPU path (tANS):
  1. Build FSM lookup table (encode + decode tables)
  2. Encode headers, metadata, dictionary, grammar tables
  3. Write to archive alongside GPU-encoded blocks
```

---

## References

- Original paper: Jarek Duda, "Asymmetric numeral systems" (arXiv:0902.0271)
- DietGPU (rANS on GPU): https://github.com/facebookresearch/dietgpu
- Zstd tANS implementation: https://github.com/facebook/zstd
- Excellent blog walkthrough: https://bjlkeng.io/posts/lossless-compression-with-asymmetric-numeral-systems/
