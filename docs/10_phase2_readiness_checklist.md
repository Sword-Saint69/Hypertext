# Phase 2 Readiness Checklist

> Part of **HyperCore Phase 1 — Research & Planning**

---

## Phase 1 Completion Status

All items below must be checked before Phase 2 (Foundation) begins.

---

### Research Studies

- [x] **CMIX studied**
  - Architecture, model ensemble, context mixing, SSE documented
  - Key limitations identified and mapped to HyperCore solutions
  - See: `docs/01_study_cmix.md`

- [x] **PAQ Family studied**
  - PPM → PAQ evolution documented
  - Full model taxonomy catalogued
  - Adoptable techniques mapped to HyperCore components
  - See: `docs/02_study_paq.md`

- [x] **ANS (rANS / tANS) studied**
  - rANS vs tANS tradeoffs understood
  - GPU multi-stream parallelism strategy defined
  - Implementation plan written
  - See: `docs/03_study_ans.md`

- [x] **GPU Compression studied**
  - State-of-the-art surveyed (nvCOMP, DietGPU, NCCLZ)
  - CUDA design principles documented
  - GPU-suitable vs CPU-suitable task split defined
  - Transfer bottleneck analysis completed
  - See: `docs/04_study_gpu_compression.md`

- [x] **Grammar Compression studied**
  - Sequitur vs Re-Pair compared
  - Per-island grammar specialization designed
  - GPU acceleration of Re-Pair frequency phase planned
  - See: `docs/05_study_grammar_compression.md`

---

### Design

- [x] **Architecture designed**
  - All 6 pipeline stages defined
  - Context Islands: 8 types, detection heuristics, predictor assignment
  - Three-tier dictionary system specified
  - CPU-GPU concurrency model designed
  - `.htx` archive format preliminary spec written
  - Component dependency graph drawn
  - See: `docs/06_architecture_design.md`

- [x] **Goals & Success Criteria defined**
  - Primary metrics with hard targets
  - Milestone targets per phase
  - Non-goals explicitly documented
  - Aspirational research contribution defined
  - See: `docs/07_goals_and_success_criteria.md`

- [x] **Benchmark plan created**
  - 8 corpora defined with acquisition commands
  - 6 competitor compressors with modes
  - Full metrics list (bpb, ratio, throughput, RAM, GPU util)
  - Profiling toolchain selected
  - Report format (JSON + Markdown) specified
  - Automation script planned
  - See: `docs/08_benchmark_plan.md`

- [x] **Key risks documented with mitigations**
  - 8 risks identified and rated
  - Mitigation strategies written for each
  - Risk priority matrix completed
  - See: `docs/09_key_risks_and_mitigations.md`

---

## Phase 2 Entry Requirements

The following must be completed **at the start of Phase 2**, before writing any compression
algorithm code.

### Repository

- [ ] GitHub repository created (`hypercore` or `htx`)
- [ ] Repository initialized with MIT license
- [ ] `.gitignore` configured (C++, CMake, CUDA, build artifacts)
- [ ] `README.md` stub with project description
- [ ] Branch strategy defined (`main`, `develop`, feature branches)

### Build System

- [ ] `CMakeLists.txt` scaffolded:
  - C++23 standard enforced
  - CUDA toolkit integration (`enable_language(CUDA)`)
  - OpenMP found and linked
  - TBB (or std::execution) found and linked
  - CMake minimum version: 3.25+
- [ ] Out-of-source build verified (`cmake -B build && cmake --build build`)
- [ ] Debug and Release configurations working
- [ ] `cmake --preset` profiles defined

### Testing Infrastructure

- [ ] GoogleTest integrated via FetchContent or vcpkg
- [ ] First smoke test compiles and passes: `ctest --test-dir build`
- [ ] Test coverage reporting configured (gcov + lcov or LLVM)

### Logging

- [ ] spdlog selected and integrated
- [ ] Log level configured via CLI flag (`--verbose`, `--quiet`)
- [ ] Log format: `[HH:MM:SS.mmm] [LEVEL] [component] message`

### Core Abstractions (interfaces only, no implementation)

- [ ] `ICompressor` interface defined
- [ ] `IPredictor` interface defined
- [ ] `IGpuBackend` interface defined
- [ ] `IArchiveWriter` / `IArchiveReader` interfaces defined
- [ ] Block abstraction (`Block`, `BlockView`) defined

### CLI Skeleton

- [ ] `htx compress <input> <output>` command parses
- [ ] `htx decompress <input> <output>` command parses
- [ ] `htx info <archive>` command parses
- [ ] `--help` outputs usage correctly
- [ ] CLI library selected (CLI11 or similar)

### Memory Manager

- [ ] Memory pool interface designed
- [ ] CPU allocator wrapper (`PageAlignedAllocator<T>`)
- [ ] GPU pinned memory allocator wrapper

### CI Pipeline

- [ ] GitHub Actions workflow: build on push
- [ ] CI runs all tests
- [ ] CI fails on any test failure or compiler warning (`-Wall -Wextra -Werror`)

---

## Phase 2 Deliverable

> A working **project skeleton** that compiles cleanly on Windows (MSVC or clang-cl) and
> Linux (GCC or Clang), with all infrastructure in place for Phase 3 algorithm development
> to begin without friction.

---

## Document Index

| # | Document | Status |
|---|---|---|
| 01 | [Study: CMIX](./01_study_cmix.md) | ✅ Complete |
| 02 | [Study: PAQ Family](./02_study_paq.md) | ✅ Complete |
| 03 | [Study: ANS](./03_study_ans.md) | ✅ Complete |
| 04 | [Study: GPU Compression](./04_study_gpu_compression.md) | ✅ Complete |
| 05 | [Study: Grammar Compression](./05_study_grammar_compression.md) | ✅ Complete |
| 06 | [Architecture Design](./06_architecture_design.md) | ✅ Complete |
| 07 | [Goals & Success Criteria](./07_goals_and_success_criteria.md) | ✅ Complete |
| 08 | [Benchmark Plan](./08_benchmark_plan.md) | ✅ Complete |
| 09 | [Key Risks & Mitigations](./09_key_risks_and_mitigations.md) | ✅ Complete |
| 10 | [Phase 2 Readiness Checklist](./10_phase2_readiness_checklist.md) | ✅ Complete |
