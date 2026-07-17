# Study: GPU Compression

> Part of **HyperCore Phase 1 — Research & Planning**

---

## State of the Art (2026)

| Library | Peak Throughput | Key Approach |
|---|---|---|
| **NVIDIA nvCOMP 4.2+** | Up to 600 GB/s | Hardware decompression engine (Blackwell arch), LZ4/Zstd/gANS |
| **DietGPU** | 250–600 GB/s | Pure parallel rANS on CUDA, generalized ANS for HPC/ML workloads |
| **NCCLZ** | Varies | Compression embedded into GPU collective communication (AllReduce) |

---

## What GPUs Are Good At (for HyperCore)

These tasks are massively parallel and data-independent — ideal for GPU:

```
├─ UTF-8 validation           — O(n) per byte, all bytes independent
├─ Character classification   — lookup table per byte
├─ Rolling hash computation   — sliding window Rabin fingerprint
├─ N-gram counting            — atomic histogram updates
├─ Phrase frequency analysis  — parallel prefix sum + counting sort
├─ Dictionary candidate gen   — top-K selection from frequency tables
├─ Entropy estimation         — parallel per-symbol log₂ computation
└─ rANS encoding              — multi-stream, one stream per CUDA thread block
```

---

## What GPUs Are Bad At (stays on CPU)

These tasks are sequential, adaptive, or synchronization-heavy:

```
├─ Context mixing weights update  — depends on every prior bit
├─ Grammar rule optimization      — graph-based, hard to parallelize
├─ Archive assembly               — sequential byte stream construction
├─ Adaptive probability decisions — control flow heavy, divergence killer
└─ Dictionary selection logic     — requires semantic reasoning per context
```

---

## CUDA Design Principles for HyperCore

### 1. Stream-Level Parallelism
Use **CUDA Streams** to overlap GPU execution with CPU work. While the GPU encodes block N,
the CPU is already preparing block N+1 and archiving block N-1.

### 2. Coalesced Memory Access
Structure GPU data as **Structure-of-Arrays (SoA)**, not Array-of-Structures (AoS).
Uncoalesced access is the #1 cause of memory bandwidth waste.

```cpp
// BAD — AoS, non-coalesced
struct Token { uint8_t type; uint32_t freq; uint16_t island; };
Token tokens[N];

// GOOD — SoA, coalesced
uint8_t  token_types[N];
uint32_t token_freqs[N];
uint16_t token_islands[N];
```

### 3. Warp Efficiency
Group tokens of the same Context Island type into the same warp to minimize branch
divergence. Use warp-level ballot/shuffle for intra-warp communication.

### 4. Library Stack

| Library | Purpose |
|---|---|
| **Thrust** | High-level parallel algorithms: sort, reduce, scan, transform |
| **CUB** | Device-wide and block-wide primitives (block radix sort, warp reduce) |
| **cuCollections (cuco)** | GPU hash maps — `cuco::static_map` for frequency tables |
| **CUDA Streams** | Async GPU-CPU pipeline overlap |
| **Nsight Compute** | Per-kernel roofline analysis |
| **Nsight Systems** | System-wide CPU+GPU timeline profiler |

### 5. Occupancy Optimization
Tune `blockDim` and shared memory usage to maximize occupancy for each kernel.
Use `cudaOccupancyMaxPotentialBlockSize()` as the starting point.

---

## GPU Memory Hierarchy Strategy

```
L1 Cache / Shared Memory (48KB per SM):
  └─ Rolling hash state, per-warp accumulators

L2 Cache (varies by GPU):
  └─ Hot frequency tables, probability tables

Global Memory (VRAM):
  └─ Token arrays, n-gram arrays, candidate dictionaries

Pinned Host Memory (CPU RAM):
  └─ Transfer buffer for CPU↔GPU via PCIe/NVLink (DMA, no copy stall)
```

---

## Key Insight: GPU as Semantic Preprocessor

Modern GPU compression libraries (nvCOMP, DietGPU) focus on encoding **already-modeled data**
at high throughput. They assume a CPU has already built the statistical model.

**HyperCore's innovation:** Use the GPU **earlier in the pipeline** — for semantic analysis,
pattern mining, and dictionary building **before** entropy coding begins.

This is the unexplored territory that separates HyperCore from all existing GPU compressors.

```
Existing GPU compressors:
  CPU: [All modeling + preprocessing] → GPU: [Just entropy coding]

HyperCore:
  GPU: [UTF-8 + tokenize + pattern mine + dict candidates + rANS]
  CPU: [Island setup + grammar opt + prob mixing + archive] (concurrent)
```

---

## Transfer Bottleneck Analysis

PCIe 4.0 x16 bandwidth: ~32 GB/s bidirectional
NVLink 4.0 (if available): ~900 GB/s

For a 256 KB input block:
- Transfer time (PCIe 4.0): ~8 µs
- GPU preprocessing time (estimated): ~50–200 µs

Transfer cost is acceptable. Key rule: **batch transfers**, never send byte-by-byte.
Use **pinned (page-locked) memory** on the host side to enable async DMA.

---

## References

- NVIDIA nvCOMP: https://developer.nvidia.com/nvcomp
- DietGPU: https://github.com/facebookresearch/dietgpu
- CUDA Thrust: https://github.com/NVIDIA/thrust
- cuCollections: https://github.com/NVIDIA/cuCollections
- CUB: https://github.com/NVIDIA/cub
