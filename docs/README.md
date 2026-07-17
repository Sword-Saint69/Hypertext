# HyperCore — Phase 1 Documentation

> **Phase:** 1 — Research & Planning  
> **Deliverable:** Research document + architecture design  
> **Status:** ✅ Complete

---

## Document Index

| File | Title | Description |
|---|---|---|
| [01_study_cmix.md](./01_study_cmix.md) | Study: CMIX | Architecture, context mixing, limitations, lessons |
| [02_study_paq.md](./02_study_paq.md) | Study: PAQ Family | PPM → PAQ evolution, model taxonomy, adoptable techniques |
| [03_study_ans.md](./03_study_ans.md) | Study: ANS | rANS vs tANS, GPU parallelism strategy, implementation plan |
| [04_study_gpu_compression.md](./04_study_gpu_compression.md) | Study: GPU Compression | nvCOMP, DietGPU, CUDA design principles, memory hierarchy |
| [05_study_grammar_compression.md](./05_study_grammar_compression.md) | Study: Grammar Compression | Sequitur, Re-Pair, per-island grammar specialization |
| [06_architecture_design.md](./06_architecture_design.md) | Architecture Design | Full pipeline, Context Islands, 3-tier dictionary, .htx format |
| [07_goals_and_success_criteria.md](./07_goals_and_success_criteria.md) | Goals & Success Criteria | Primary targets, milestones, non-goals, aspirational target |
| [08_benchmark_plan.md](./08_benchmark_plan.md) | Benchmark Plan | Corpora, competitors, metrics, toolchain, report format |
| [09_key_risks_and_mitigations.md](./09_key_risks_and_mitigations.md) | Key Risks & Mitigations | 8 risks with likelihood, impact, and mitigation strategies |
| [10_phase2_readiness_checklist.md](./10_phase2_readiness_checklist.md) | Phase 2 Readiness Checklist | All Phase 1 completion items + Phase 2 entry requirements |

---

## Quick Summary

**HyperCore** is a next-generation lossless text compression system that distributes work
across CPUs and GPUs to achieve near-CMIX compression ratios at practical speeds.

### The Core Innovation

```
Traditional compressors:
  One global model → sequential encoding → one thread

HyperCore:
  Semantic preprocessing (GPU) → Context Islands → parallel rANS (GPU)
                                → Grammar compression (CPU) → concurrent
```

### Phase 1 Key Decisions

| Decision | Choice | Reason |
|---|---|---|
| GPU entropy coder | rANS (multi-stream) | Parallelizable unlike arithmetic coding |
| CPU entropy coder | tANS (lookup table) | Fastest single-threaded entropy coder |
| Grammar algorithm | Re-Pair (primary) | Better ratios; frequency step is GPU-parallelizable |
| Grammar fallback | Sequitur | Streaming/bounded memory for large files |
| Archive format | .htx (custom) | Need random access, per-block island maps, versioning |
| Target hardware | CUDA ≥ CC 7.0 | Covers GTX 1080 through RTX 5090 |

### Next Step

**Phase 2 — Foundation:** Set up the C++23 + CMake + CUDA project skeleton with
all infrastructure (build system, testing, logging, CLI, interfaces) ready for
algorithm implementation in Phase 3.
