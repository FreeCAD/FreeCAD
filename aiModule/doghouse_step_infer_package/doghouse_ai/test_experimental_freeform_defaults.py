from doghouse_ai.doghouse_assembly_features import _build_arg_parser as build_assembly_parser
from doghouse_ai.infer_from_step import (
    _attach_pmae_face_embeddings,
    _build_arg_parser as build_infer_parser,
    _ensure_pmae_face_embeddings,
    _write_pmae_input,
)
import numpy as np
import pytest


def test_assembly_experimental_freeform_endpoint_is_default_on():
    parser = build_assembly_parser()

    args = parser.parse_args(["--step", "part.step", "--prediction-json", "pred.json", "--output", "out.json"])

    assert args.experimental_freeform_endpoint is True


def test_assembly_can_disable_experimental_freeform_endpoint():
    parser = build_assembly_parser()

    args = parser.parse_args(
        [
            "--step",
            "part.step",
            "--prediction-json",
            "pred.json",
            "--output",
            "out.json",
            "--no-experimental-freeform-endpoint",
        ]
    )

    assert args.experimental_freeform_endpoint is False


def test_infer_experimental_freeform_endpoint_is_default_on():
    parser = build_infer_parser()

    args = parser.parse_args(["--step", "part.step", "--checkpoint", "model.pt"])

    assert args.experimental_freeform_endpoint is True


def test_infer_can_disable_experimental_freeform_endpoint():
    parser = build_infer_parser()

    args = parser.parse_args(
        ["--step", "part.step", "--checkpoint", "model.pt", "--no-experimental-freeform-endpoint"]
    )

    assert args.experimental_freeform_endpoint is False


def test_infer_parser_accepts_pmae_face_embedding_directory():
    parser = build_infer_parser()

    args = parser.parse_args(
        [
            "--step",
            "part.step",
            "--checkpoint",
            "model.pt",
            "--pmae-face-emb-dir",
            "emb",
            "--pmae-ckpt",
            "ckpt.pth",
        ]
    )

    assert args.pmae_face_emb_dir == "emb"
    assert args.pmae_ckpt == "ckpt.pth"


def test_attach_pmae_face_embeddings_loads_stem_file(tmp_path):
    emb_dir = tmp_path / "emb"
    emb_dir.mkdir()
    emb = np.arange(6, dtype=np.float32).reshape(2, 3)
    np.save(emb_dir / "part_pmae_face_emb.npy", emb)
    data = {"face_features": np.zeros((2, 12), dtype=np.float32)}

    _attach_pmae_face_embeddings(data, emb_dir, "part")

    np.testing.assert_array_equal(data["face_pmae"], emb)


def test_attach_pmae_face_embeddings_rejects_face_count_mismatch(tmp_path):
    emb_dir = tmp_path / "emb"
    emb_dir.mkdir()
    np.save(emb_dir / "part_pmae_face_emb.npy", np.zeros((3, 1152), dtype=np.float32))
    data = {"face_features": np.zeros((2, 12), dtype=np.float32)}

    with pytest.raises(ValueError, match="PMAE face embedding count"):
        _attach_pmae_face_embeddings(data, emb_dir, "part")


def test_write_pmae_input_normalizes_points(tmp_path):
    data = {
        "points": np.asarray([[1.0, 0.0, 0.0], [3.0, 0.0, 0.0]], dtype=np.float32),
        "face_idx": np.asarray([0, 1], dtype=np.int64),
        "face_features": np.zeros((2, 12), dtype=np.float32),
    }

    out = _write_pmae_input(data, tmp_path, "part")

    saved = np.load(out)
    assert out.name == "part_pmae_input.npz"
    np.testing.assert_allclose(saved["points"], np.asarray([[-1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], dtype=np.float32))
    np.testing.assert_array_equal(saved["face_idx"], data["face_idx"])
    assert int(saved["num_faces"]) == 2


def test_ensure_pmae_face_embeddings_generates_when_missing(tmp_path, monkeypatch):
    data = {
        "points": np.zeros((2, 3), dtype=np.float32),
        "face_idx": np.asarray([0, 1], dtype=np.int64),
        "face_features": np.zeros((2, 12), dtype=np.float32),
    }
    emb_dir = tmp_path / "emb"
    ckpt = tmp_path / "ckpt.pth"
    ckpt.write_bytes(b"placeholder")
    calls = []

    def fake_run(cmd, check, cwd):
        calls.append((cmd, check, cwd))
        emb_dir.mkdir(parents=True, exist_ok=True)
        np.save(emb_dir / "part_pmae_face_emb.npy", np.ones((2, 3), dtype=np.float32))

    monkeypatch.setattr("doghouse_ai.infer_from_step.subprocess.run", fake_run)

    emb_path = _ensure_pmae_face_embeddings(data, emb_dir, "part", ckpt, tmp_path / "input", 256, 32, False)

    assert emb_path == emb_dir / "part_pmae_face_emb.npy"
    assert calls
    np.testing.assert_array_equal(data["face_pmae"], np.ones((2, 3), dtype=np.float32))
