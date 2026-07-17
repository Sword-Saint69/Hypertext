# Study: PAQ Family

> Part of **HyperCore Phase 1 — Research & Planning**

---

## What Is PAQ?

PAQ (Matt Mahoney) is the predecessor family to CMIX. It pioneered **context mixing** for
lossless compression and consistently tops the Hutter Prize leaderboard alongside CMIX.
ZPAQ is its self-describing, versioned variant.

---

## Evolution: PPM → PAQ

### Prediction by Partial Matching (PPM)

- Predicts the next symbol using the *n* preceding symbols as context.
- Falls back to shorter contexts when a pattern hasn't been seen.
- Selects **one** best-matching context — major limitation.
- Byte-level processing.

### PAQ's Breakthrough

- Instead of selecting one context, PAQ runs **all contexts simultaneously** and mixes
  their outputs.
- Bit-level processing — more granular and avoids PPM's binary-escape inefficiency.
- PAQ7+ uses a small neural network (logistic regressor) as the mixing layer.
- Non-stationary: models aggressively decay counters to adapt to shifting data.

---

## Comparison: PPM vs PAQ

| Feature | PPM | PAQ Family |
|---|---|---|
| Prediction basis | Single longest matching context | Weighted combination of many models |
| Granularity | Byte-level | Bit-level |
| Strategy | Fallback through context orders | Context mixing |
| Complexity | Moderate | Very high (memory + CPU) |

---

## PAQ Model Taxonomy

```
PAQ Model Types:
├─ CharModel      — simple byte-order statistics
├─ WordModel      — word-boundary-aware contexts
├─ MatchModel     — long-match LZ-style predictor
├─ SparseModel    — non-contiguous byte contexts (e.g., skip every 4 bytes)
├─ RecordModel    — detects fixed-width record structures (CSV, binary tables)
├─ NestModel      — tracks nesting depth in structured text (XML, JSON, code)
├─ XMLModel       — tag-aware prediction for XML/HTML
└─ NNModel        — small neural net predictor (PAQ8+)
```

---

## Key Techniques Worth Adopting

| PAQ Technique | HyperCore Adaptation |
|---|---|
| Multiple simultaneous models | Adaptive predictor selection — run only the relevant subset |
| Bit-level logistic mixing | Applied on a per-Context-Island basis |
| SparseModel for non-contiguous patterns | Include in GPU phase as a sliding pattern hash |
| RecordModel for structural detection | Core of the Context Island classifier |
| NestModel for bracket tracking | Part of the JSON/XML/code Context Island predictor |

---

## Key Lesson

PAQ proves that a **mixture of specialized models** always outperforms any single model,
regardless of how sophisticated that model is. HyperCore embraces this principle but makes
the mixture **parallel** and **hardware-aware**, solving PAQ's speed and scalability problem.

---

## References

- Matt Mahoney's site: http://mattmahoney.net/dc/
- PAQ8 source: https://github.com/kaitz/paq8px
- Hutter Prize: http://prize.hutter1.net/
