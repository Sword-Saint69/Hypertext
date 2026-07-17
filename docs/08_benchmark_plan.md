# Benchmark Plan

> Part of **HyperCore Phase 1 — Research & Planning**

---

## Overview

The benchmark plan defines:
1. Which corpora to test against
2. Which competitor compressors to baseline
3. Which metrics to capture
4. Which tools to use for profiling and measurement
5. How results will be reported

---

## Test Corpora

| Corpus | Size | Description | Why |
|---|---|---|---|
| **Canterbury Corpus** | ~2.8 MB | Classic mix: text, C code, HTML, binaries | Standard baseline for all compressors |
| **Calgary Corpus** | ~3.1 MB | Older reference mix | Historical comparison and regression detection |
| **Silesia Corpus** | ~211 MB | Modern mix: C++ source, XML, HTML, PDF text, DB | Realistic modern workloads |
| **Large GitHub repos** | ~500 MB | C++/Python source code (e.g., llvm, linux kernel) | Primary target domain validation |
| **Wikipedia dump (enwik8)** | 100 MB | First 10^8 bytes of Wikipedia XML | Hutter Prize standard — NL + XML mix |
| **Wikipedia dump (enwik9)** | 1 GB | First 10^9 bytes | Large-scale NL performance |
| **JSON datasets** | ~100 MB | Synthetic + real API responses | JSON Context Island validation |
| **Log datasets** | ~100 MB | Apache/nginx/syslog logs | Log Context Island validation |

### Corpus Acquisition

```bash
# Canterbury
wget http://corpus.canterbury.ac.nz/resources/cantrbry.tar.gz

# Silesia
wget http://sun.aei.polsl.pl/~sdeor/corpus/silesia.zip

# enwik8 / enwik9
wget http://mattmahoney.net/dc/enwik8.zip
wget http://mattmahoney.net/dc/enwik9.zip
```

---

## Competitor Compressors

| Compressor | Modes Tested | Why |
|---|---|---|
| **Zstandard (zstd)** | -3 (default), -12, -19, --ultra -22 | Modern fast compressor, best general-purpose baseline |
| **Brotli (brotli)** | -q 6, -q 9, -q 11 | Web standard, excellent on text/HTML |
| **XZ / LZMA2** | -6, -9, -e9 | High-ratio archiver, best practical text ratio |
| **7-Zip (7za)** | -mx=5, -mx=9 | Common archiver, LZMA2 based |
| **PAQ8pxd** | default | Near-CMIX ratio, representative of PAQ family |
| **CMIX v21** | default | Gold standard compression ratio |

### Competitor Versions to Lock

- Record exact version numbers in benchmark logs.
- All competitors compiled from source with `-O3 -march=native` for fair CPU comparison.
- CMIX and PAQ tested only on smaller corpora (< 10 MB) due to extreme time requirements.

---

## Metrics Captured

For every **(corpus file, compressor, mode)** triple:

| Metric | Unit | How Measured |
|---|---|---|
| Compressed size | bytes | File system stat |
| Compression ratio | original / compressed | Computed |
| Bits per byte (bpb) | bits | `8 × compressed / original` |
| Compression time | milliseconds | `clock_gettime(CLOCK_MONOTONIC)` |
| Decompression time | milliseconds | Same |
| Compression throughput | MB/s | original_size / compress_time |
| Decompression throughput | MB/s | original_size / decompress_time |
| Peak RSS memory | MB | `/proc/self/status` or `getrusage` |
| GPU utilization | % | Nsight Systems timeline |
| CPU core utilization | % | perf / VTune |

### Statistical Rigor

- Run each benchmark **5 times**, report median.
- Drop the min and max (outlier removal).
- Report standard deviation when > 5% variance.
- Disable system swap, close browser during GPU runs.
- Flush filesystem cache before each decompression run: `echo 3 > /proc/sys/vm/drop_caches`

---

## Profiling Toolchain

| Tool | Purpose | Phase |
|---|---|---|
| **Google Benchmark** | Micro-benchmarks for individual pipeline stages | All phases |
| **Tracy Profiler** | Frame-based profiler — good for pipelined, multi-threaded workloads | Phase 8+ |
| **Nsight Compute** | Per-CUDA-kernel roofline analysis, memory bandwidth, occupancy | Phase 8+ |
| **Nsight Systems** | System-wide CPU+GPU timeline, PCIe transfer visualization | Phase 8+ |
| **Intel VTune Profiler** | CPU cache miss analysis, instruction throughput, NUMA effects | Phase 9 |
| **perf** (Linux) | Low-overhead CPU sampling, IPC, cache stats | All phases |
| **heaptrack** | Heap allocation profiling — detect excessive allocations | Phase 2+ |

---

## Reporting Format

All benchmark results are stored as **JSON** for programmatic analysis and
rendered as Markdown tables for human review.

### Example Report Table

```
Corpus: enwik8 (100,000,000 bytes — Wikipedia XML)
═══════════════════════════════════════════════════════════════════════════
Compressor          │  bpb   │ Ratio  │ C MB/s  │ D MB/s  │ RAM
────────────────────┼────────┼────────┼─────────┼─────────┼────────
HyperCore v0.1      │  2.80  │  2.86  │  25.0   │ 280.0   │ 1.8 GB
HyperCore v1.0      │  2.40  │  3.33  │  55.0   │ 480.0   │ 2.1 GB
────────────────────┼────────┼────────┼─────────┼─────────┼────────
CMIX v21            │  1.72  │  4.65  │   0.02  │   0.02  │  32 GB
PAQ8pxd             │  1.98  │  4.04  │   0.3   │   0.3   │   8 GB
XZ -9               │  2.47  │  3.24  │   1.2   │  78.0   │ 686 MB
Brotli -q11         │  2.58  │  3.10  │   0.9   │ 320.0   │ 512 MB
Zstd -22            │  2.97  │  2.69  │   4.1   │ 1200.0  │ 512 MB
Zstd -3             │  3.86  │  2.07  │ 350.0   │ 1400.0  │ 128 MB
═══════════════════════════════════════════════════════════════════════════
```

### JSON Schema for Stored Results

```json
{
  "run_id": "2026-07-17T00:00:00Z",
  "system": {
    "cpu": "AMD Ryzen 9 7950X",
    "gpu": "NVIDIA RTX 4090",
    "ram_gb": 64,
    "os": "Windows 11 / Ubuntu 24.04"
  },
  "results": [
    {
      "corpus": "enwik8",
      "compressor": "hypercore",
      "version": "v0.1.0",
      "mode": "default",
      "original_bytes": 100000000,
      "compressed_bytes": 35000000,
      "bpb": 2.80,
      "ratio": 2.857,
      "compress_ms": 4000,
      "decompress_ms": 357,
      "compress_mbs": 25.0,
      "decompress_mbs": 280.0,
      "peak_ram_mb": 1800,
      "gpu_util_pct": 74.2
    }
  ]
}
```

---

## Benchmark Automation

A Python script `tools/benchmark.py` will:

1. Download and verify all corpora (BLAKE3 checksums)
2. Compile all competitors from source
3. Run all (corpus, compressor, mode) triples
4. Store results in `benchmarks/results/YYYY-MM-DD/`
5. Generate Markdown report tables
6. Plot ratio vs speed scatter charts (matplotlib)

```bash
python tools/benchmark.py --corpus silesia enwik8 \
                           --compressors hypercore zstd brotli xz \
                           --runs 5 \
                           --output benchmarks/results/
```
