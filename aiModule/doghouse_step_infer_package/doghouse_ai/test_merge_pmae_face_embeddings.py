import numpy as np
import pytest

from doghouse_ai.merge_pmae_face_embeddings import merge_pmae_face_embeddings


def test_merge_pmae_face_embeddings_writes_face_pmae(tmp_path):
    npz_path = tmp_path / "part_points.npz"
    emb_path = tmp_path / "part_pmae_face_emb.npy"
    out_path = tmp_path / "part_points_with_pmae.npz"
    np.savez_compressed(npz_path, face_features=np.zeros((2, 12), dtype=np.float32))
    emb = np.arange(6, dtype=np.float32).reshape(2, 3)
    np.save(emb_path, emb)

    merge_pmae_face_embeddings(npz_path, emb_path, out_path)

    merged = np.load(out_path)
    np.testing.assert_array_equal(merged["face_pmae"], emb)


def test_merge_pmae_face_embeddings_rejects_count_mismatch(tmp_path):
    npz_path = tmp_path / "part_points.npz"
    emb_path = tmp_path / "part_pmae_face_emb.npy"
    out_path = tmp_path / "part_points_with_pmae.npz"
    np.savez_compressed(npz_path, face_features=np.zeros((2, 12), dtype=np.float32))
    np.save(emb_path, np.zeros((3, 1152), dtype=np.float32))

    with pytest.raises(ValueError, match="PMAE face embedding count"):
        merge_pmae_face_embeddings(npz_path, emb_path, out_path)
