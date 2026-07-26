# mate_pair_v1 (frozen)

This directory is the frozen candidate-pair location-ranking baseline.

- `best.pt`: epoch 3 checkpoint selected by validation MRR.
- `last.pt`: epoch 99 checkpoint retained for comparison only.
- `metrics.jsonl`: all 100 training epochs.
- `evaluation_test.json`: untouched test-split evaluation.
- `FROZEN.json`: immutable artifact hashes and headline metrics.

The model has only a binary location-ranking head. It does not predict mate
type, axial offset, axis flip, or a complete rigid placement. New experiments
must use a different output directory, beginning with `mate_multitask_v2`.
