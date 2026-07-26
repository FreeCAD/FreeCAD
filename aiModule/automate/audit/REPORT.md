# AutoMate 1,000-mate audit

Seed: `20260725`

## Data health

- Audited mates: 1,000
- Successfully processed: 1,000 (100%)
- Audited part sides: 2,000
- Invalid MCF candidates filtered: 2,057 across 620 part sides

## Candidate recall

- Axis recall at 1 degree: 99.40%
- Axis-line recall at 1 degree and 0.1 mm: 98.35%
- Exact-origin recall at 1 degree and 0.1 mm: 25.80%
- Absolute axial offset median: 1.5 mm
- Absolute axial offset p95: 75 mm

The model must select a geometric axis/plane and regress an axial offset. Treating
the task as classification over candidate origins alone would discard most labels.

## Candidate scale

- Candidate count per part side, median: 307
- Candidate count per part side, p95: 6,413

A full Cartesian product of candidates is not tractable for the long tail. Training
and inference need candidate pruning, hard-negative sampling, or a two-stage ranker.

## Mate distribution

| Mate type | Count | Share |
|---|---:|---:|
| FASTENED | 751 | 75.1% |
| PLANAR | 99 | 9.9% |
| SLIDER | 91 | 9.1% |
| REVOLUTE | 31 | 3.1% |
| CYLINDRICAL | 16 | 1.6% |
| BALL | 6 | 0.6% |
| PARALLEL | 4 | 0.4% |
| PIN_SLOT | 2 | 0.2% |

Use class-balanced sampling or class-weighted loss. Rare-class metrics from this
1,000-mate sample are not statistically reliable.

## Training implications

1. Filter all non-finite origins and axes during preprocessing.
2. Use axis/plane selection as the first-stage target.
3. Regress axial offset separately from candidate selection.
4. Predict axis orientation/flip separately because geometric lines are unoriented.
5. Prune candidates before pair scoring and sample hard negatives.
6. Split by assembly or originating document, not by individual mate, to avoid leakage.
7. Audit label world-frame consistency and STEP conversion quality before full training.

Raw rows are in `mates_1000.jsonl`; machine-readable aggregate statistics are in
`mates_1000.summary.json`.
