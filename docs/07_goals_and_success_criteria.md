# Goals & Success Criteria

> Part of **HyperCore Phase 1 — Research & Planning**

---

## Primary Goals

These are the hard targets HyperCore v1.0 must hit to be considered a research success.

| Goal | Target Metric | Notes |
|---|---|---|
| **Compression ratio — text corpora** | Within 5–10% of CMIX on Canterbury/Silesia text files | bpb ≤ 2.0 on enwik8 aspirational |
| **Compression speed** | ≥ 10× faster than CMIX at a comparable ratio setting | CMIX: ~1 MB/min → target: ~10 MB/min minimum |
| **Decompression speed** | ≥ 100 MB/s minimum, targeting ≥ 500 MB/s | Must be usable in real workflows |
| **Peak memory usage** | ≤ 4 GB RAM | vs CMIX's 32 GB+ requirement |
| **GPU utilization** | ≥ 70% sustained during GPU phases | Measured with Nsight Systems |
| **CPU utilization** | ≥ 80% average across all cores during CPU phases | No idle cores during active phases |
| **Lossless correctness** | 100% BLAKE3 round-trip verified | Zero-tolerance — any failure is a critical bug |

---

## Secondary Goals

Goals that would strengthen the project but are not blockers for v1.0.

| Goal | Target |
|---|---|
| Beat **XZ -9** on pure text compression ratio | Yes — XZ is the current best practical archiver for text |
| Approach **Brotli quality=11** ratio on web content | Within 10% on HTML/JS/CSS |
| Beat **Zstd -22** (ultra) on source code corpora | Yes — source code is HyperCore's primary target domain |
| Context Island classifier accuracy | ≥ 95% correct island assignment on mixed documents |
| Grammar compression improvement over raw entropy | ≥ 15% size reduction on structured text before entropy coding |
| Parallel efficiency (CPU) | Linear scaling to at least 8 cores |

---

## Milestone Targets

| Phase | Version | Target bpb on enwik8 | Compression Speed |
|---|---|---|---|
| Phase 7 (end-to-end, no GPU) | v0.1 | ≤ 3.5 | ≥ 5 MB/s |
| Phase 8 (GPU pipeline) | v0.5 | ≤ 3.0 | ≥ 20 MB/s |
| Phase 9 (CPU optimization) | v0.7 | ≤ 2.8 | ≥ 40 MB/s |
| Phase 11 (fully validated) | v1.0 | ≤ 2.5 | ≥ 50 MB/s |
| Phase 13 (advanced features) | v1.5 | ≤ 2.2 | ≥ 80 MB/s |

> **Calibration:** CMIX v21 achieves ~1.7 bpb on enwik8 at ~0.001 MB/s.
> Zstd -22 achieves ~2.5 bpb at ~4 MB/s. HyperCore targets the gap between them.

---

## Non-Goals (Out of Scope)

These are explicitly **not** objectives for HyperCore. Attempting them would dilute focus.

| Non-Goal | Reason |
|---|---|
| Binary file compression (images, video, executables) | Fundamentally different statistical properties — separate problem |
| Lossy compression of any kind | Outside the lossless constraint |
| Real-time streaming (< 10 ms latency) | Incompatible with block-based pipeline in Phases 1–12 |
| FPGA / custom ASIC acceleration | Out of scope for research prototype |
| Cross-platform GPU support (AMD ROCm) | HIP port considered as Phase 13+ extension |
| Sub-megabyte file optimization | HyperCore's multi-tier overhead suits large files best |

---

## Success Criterion Summary

```
HyperCore v1.0 is a SUCCESS if:

✅ It can compress any valid UTF-8 text file losslessly (100% round-trip verified)
✅ It achieves a compression ratio better than Zstd -22 on text corpora
✅ It compresses at ≥ 50 MB/s on a modern workstation (CPU + GPU)
✅ It decompresses at ≥ 200 MB/s
✅ It uses ≤ 4 GB RAM at peak
✅ It runs on any CUDA-capable GPU (Compute Capability ≥ 7.0)
✅ Its .htx format is documented and stable for decoder distribution
```

---

## Aspirational Target (Research Contribution)

If the following is achieved, HyperCore represents a genuine research breakthrough:

> Compress enwik8 to ≤ 2.0 bpb at ≥ 30 MB/s using ≤ 4 GB RAM on a single workstation.

This would define a new point on the compression ratio / speed Pareto frontier that no
existing compressor occupies — combining CMIX-class semantic awareness with practical GPU
acceleration.
