# Embedded Doghouse runtime

The runtime consumes `doghouse_inference_geometry.v1` JSON directly. It does
not import FreeCAD or PythonOCC and does not create an intermediate STEP file.

```bash
python infer_from_geometry.py \
  --geometry /tmp/pscad-doghouse/job/geometry.json \
  --output-dir /tmp/pscad-doghouse/job/result \
  --cpu
```

The command writes the intermediate point dataset, Point-MAE face embeddings,
and `<geometry-stem>_doghouse_result.json`. Each face prediction preserves the
input `face_idx`, `kernel_face_tag`, and `persistent_face_id` when available.

Install dependencies from `requirements-linux.txt`. CUDA is optional because
the migrated Point-MAE helpers include pure PyTorch CPU fallbacks.

Geometry extraction parity is validated separately with the scripts under
`../tools`. PythonOCC is required only for generating the STEP reference and is
not a dependency of the production Geometry JSON inference path.
