# Key Risks & Mitigations

> Part of **HyperCore Phase 1 — Research & Planning**

---

## Risk Register

### R1 — GPU-CPU Transfer Bottleneck

| Field | Detail |
|---|---|
| **Description** | PCIe bandwidth becomes the bottleneck when GPU preprocessing time is short relative to data transfer time for small blocks |
| **Likelihood** | High |
| **Impact** | High |
| **Trigger** | Block size < 64 KB or GPU preprocessing < 50 µs |

**Mitigation Strategies:**
- Use **pinned (page-locked) host memory** for all transfer buffers to enable async DMA.
- Set block size floor at **128 KB** — amortizes transfer overhead.
- Use **double-buffering**: while GPU processes block N, transfer block N+1 in background.
- Profile PCIe utilization with Nsight Systems before optimizing anything else.
- On NVLink-equipped systems (A100, H100): bandwidth bottleneck is largely eliminated.

---

### R2 — Grammar Memory Explosion

| Field | Detail |
|---|---|
| **Description** | Re-Pair requires 4–8× the input size in RAM for the frequency table and token stream. A 1 GB file would need 4–8 GB just for grammar processing |
| **Likelihood** | Medium |
| **Impact** | High |

**Mitigation Strategies:**
- Process files in **sharded blocks** (256 KB – 4 MB per shard). Grammar is built per-shard.
- Cap grammar table at a configurable maximum size (default: 512 MB).
- Implement a **streaming Re-Pair variant** that releases processed tokens immediately.
- For very large files: fall back to Sequitur (online, bounded memory) automatically.
- Add a `--no-grammar` mode that skips grammar compression for memory-constrained systems.

---

### R3 — Context Island Misclassification

| Field | Detail |
|---|---|
| **Description** | Mixed-format files (e.g., Markdown with embedded JSON code blocks) may confuse the island classifier, assigning tokens to the wrong island and degrading prediction |
| **Likelihood** | Medium |
| **Impact** | Medium |

**Mitigation Strategies:**
- Train the classifier on labeled mixed-format files; target ≥ 95% accuracy.
- Implement **island blending**: if an island's local model underperforms its prior, blend in the general model with adaptive weight.
- Add an island **confidence score** — low-confidence regions fall back to a universal model.
- Islands with < 256 tokens are merged into the surrounding island to avoid degenerate cases.

---

### R4 — rANS Quantization Error

| Field | Detail |
|---|---|
| **Description** | rANS requires symbol frequencies to be quantized to a table of size M = 2^k. Rounding errors can cause the encoder and decoder to diverge, corrupting the output |
| **Likelihood** | Low |
| **Impact** | Critical (data corruption) |

**Mitigation Strategies:**
- Use **M = 4096 (k=12)** — sufficient precision for all text distributions.
- Implement a **round-trip unit test** on every build: compress → decompress → diff against original. Failure is a CI blocker.
- Validate that `sum(quantized_freqs) == M` exactly after quantization.
- Use BLAKE3 checksum verification in the decoder as an integrity gate.
- Fuzz the entropy coder with afl++ targeting the decode path.

---

### R5 — CMIX-Class Ratio Unreachable at Practical Speed

| Field | Detail |
|---|---|
| **Description** | CMIX achieves its ratio through 2,000+ models updated per bit. HyperCore's adaptive selection may miss long-range dependencies that CMIX captures |
| **Likelihood** | High |
| **Impact** | Medium |

**Mitigation Strategies:**
- Reframe the target: HyperCore does **not** need to match CMIX. It needs to define a **new Pareto point** — better ratio than Zstd at a speed that CMIX can never approach.
- Track ratio vs speed on a 2D Pareto chart. Being dominant on this chart is the research contribution.
- Incrementally add predictors (e.g., a lightweight RWKV/Mamba neural head) if ratio falls short.
- Publish an honest comparison: "HyperCore at 50 MB/s vs CMIX at 0.02 MB/s at comparable memory."

---

### R6 — CUDA Portability (AMD / Intel GPUs)

| Field | Detail |
|---|---|
| **Description** | The codebase is written for NVIDIA CUDA. Users with AMD or Intel GPUs cannot use the GPU acceleration path |
| **Likelihood** | Low (for research context) |
| **Impact** | Low (for research context) |

**Mitigation Strategies:**
- Abstract the GPU backend behind a clean C++ interface (`IGpuBackend`).
- All GPU code is isolated in `src/gpu/cuda/` — a HIP port lives in `src/gpu/hip/`.
- CPU-only fallback path is always available and fully functional — just slower.
- AMD HIP port is planned as a **Phase 13+ extension** after the CUDA version is proven.

---

### R7 — Archive Format Instability

| Field | Detail |
|---|---|
| **Description** | If the .htx format changes between versions, old archives become unreadable |
| **Likelihood** | Medium (during development) |
| **Impact** | Medium |

**Mitigation Strategies:**
- Include a **format version field** in every archive header from Day 1.
- Write a format changelog document alongside every breaking change.
- Freeze the format specification at v1.0 with a compatibility commitment.
- Provide an `htx-migrate` tool for version upgrades.

---

### R8 — Benchmark Gaming

| Field | Detail |
|---|---|
| **Description** | Over-tuning to specific benchmark corpora without achieving general compression quality |
| **Likelihood** | Medium |
| **Impact** | Medium (scientific credibility) |

**Mitigation Strategies:**
- Hold out a **secret validation corpus** not used during development.
- Test on diverse corpora from the start — don't optimize for any single benchmark file.
- Publish all benchmark scripts and corpora lists so results are fully reproducible.

---

## Risk Summary Matrix

| Risk ID | Risk | Likelihood | Impact | Priority |
|---|---|---|---|---|
| R1 | GPU-CPU Transfer Bottleneck | High | High | 🔴 Critical |
| R2 | Grammar Memory Explosion | Medium | High | 🔴 Critical |
| R3 | Context Island Misclassification | Medium | Medium | 🟡 Moderate |
| R4 | rANS Quantization Error | Low | Critical | 🔴 Critical |
| R5 | CMIX Ratio Unreachable | High | Medium | 🟡 Moderate |
| R6 | CUDA Portability | Low | Low | 🟢 Low |
| R7 | Archive Format Instability | Medium | Medium | 🟡 Moderate |
| R8 | Benchmark Gaming | Medium | Medium | 🟡 Moderate |
