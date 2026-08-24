import json
import os
import sys
import tempfile
import types
import unittest
from pathlib import Path

from freecad_doghouse_assembly.clip_library_model import scan_clip_library
from freecad_doghouse_assembly.doghouse_task_panel import (
    resolve_prediction_json_from_names,
    resolve_source_step_from_prediction_json,
)
from freecad_doghouse_assembly.freecad_io import clear_preview_group
from freecad_doghouse_assembly.runner import DoghouseRunner


class FreeCADRunnerTest(unittest.TestCase):
    def test_recommend_command_uses_uv01_and_no_assemble(self):
        runner = DoghouseRunner(
            project_root=Path("E:/repo"),
            python_exe=Path("D:/tools/envs/uv01/python.exe"),
            clip_library=Path("E:/repo/卡扣库"),
        )

        cmd = runner.build_recommend_command(Path("tmp/model.step"), Path("tmp/recommend.json"))

        self.assertEqual(cmd[0], "D:/tools/envs/uv01/python.exe")
        self.assertIn("recommend_and_assemble.py", cmd[1])
        self.assertIn("--auto-doghouse", cmd)
        self.assertIn("--recommend-output", cmd)
        self.assertIn("tmp/recommend.json", cmd)
        self.assertIn("--all-doghouses-same-clip", cmd)
        self.assertIn("--doghouse-infer-output-dir", cmd)
        self.assertIn("tmp/infer", cmd)
        self.assertIn("--infer-cpu", cmd)
        self.assertNotIn("--prediction-json", cmd)
        self.assertIn("--no-assemble", cmd)

    def test_recommend_command_can_pass_prediction_json(self):
        runner = DoghouseRunner(
            project_root=Path("E:/repo"),
            python_exe=Path("D:/tools/envs/uv01/python.exe"),
            clip_library=Path("E:/repo/卡扣库"),
        )

        cmd = runner.build_recommend_command(
            Path("tmp/model.step"),
            Path("tmp/recommend.json"),
            prediction_json=Path("step - 副本2/pillar annotation.json"),
        )

        self.assertIn("--prediction-json", cmd)
        self.assertIn("step - 副本2/pillar annotation.json", cmd)

    def test_placement_command_uses_selected_clip_and_no_step_output(self):
        runner = DoghouseRunner(
            project_root=Path("E:/repo"),
            python_exe=Path("D:/tools/envs/uv01/python.exe"),
            clip_library=Path("E:/repo/卡扣库"),
        )

        cmd = runner.build_placement_command(
            Path("tmp/model.step"),
            Path("tmp/place.json"),
            clip_name="IX-05402112",
        )

        self.assertIn("--clip-name", cmd)
        self.assertIn("IX-05402112", cmd)
        self.assertIn("--placement-output", cmd)
        self.assertIn("--all-doghouses-same-clip", cmd)
        self.assertIn("--doghouse-infer-output-dir", cmd)
        self.assertIn("tmp/infer", cmd)
        self.assertIn("--infer-cpu", cmd)
        self.assertIn("--no-step-output", cmd)

    def test_placement_command_can_invert_all_indices(self):
        runner = DoghouseRunner(
            project_root=Path("E:/repo"),
            python_exe=Path("D:/tools/envs/uv01/python.exe"),
            clip_library=Path("E:/repo/卡扣库"),
        )

        cmd = runner.build_placement_command(
            Path("tmp/model.step"),
            Path("tmp/place.json"),
            clip_name="09855056",
            invert_indices=[1, 2, 3],
        )

        self.assertIn("--invert-direction-indices", cmd)
        self.assertIn("1,2,3", cmd)

    def test_clear_preview_group_removes_group_children_and_old_clip_residue(self):
        class FakeObject:
            def __init__(self, name, label=None, group=None):
                self.Name = name
                self.Label = label or name
                self.Group = group or []

        class FakeDoc:
            def __init__(self, objects):
                self.Objects = list(objects)
                self.recomputed = False

            def getObject(self, name):
                return next((obj for obj in self.Objects if obj.Name == name), None)

            def removeObject(self, name):
                self.Objects = [obj for obj in self.Objects if obj.Name != name]

            def recompute(self):
                self.recomputed = True

        child = FakeObject("ClipChild", "Q693022_1")
        group = FakeObject("Doghouse_Auto_Assembly_Preview", group=[child])
        residue = FakeObject("OldClip", "Q693022_2")
        source = FakeObject("SourceModel", "M5-6302161")
        fake_doc = FakeDoc([source, group, child, residue])
        old_freecad = sys.modules.get("FreeCAD")
        sys.modules["FreeCAD"] = types.SimpleNamespace(ActiveDocument=fake_doc)
        try:
            removed = clear_preview_group(clip_names=["Q693022"])
        finally:
            if old_freecad is None:
                sys.modules.pop("FreeCAD", None)
            else:
                sys.modules["FreeCAD"] = old_freecad

        self.assertEqual(removed, 3)
        self.assertEqual([obj.Name for obj in fake_doc.Objects], ["SourceModel"])
        self.assertTrue(fake_doc.recomputed)

    def test_subprocess_env_prioritizes_uv01_and_filters_freecad_path(self):
        old_path = os.environ.get("PATH", "")
        try:
            os.environ["PATH"] = os.pathsep.join(
                ["C:/Program Files/FreeCAD 1.1/bin", "C:/Windows/System32"]
            )
            runner = DoghouseRunner(
                project_root=Path("E:/repo"),
                python_exe=Path("D:/tools/envs/uv01/python.exe"),
                clip_library=Path("E:/repo/卡扣库"),
            )

            env = runner._subprocess_env()
        finally:
            os.environ["PATH"] = old_path

        self.assertIn("D:\\tools\\envs\\uv01\\Library\\bin", env["PATH"])
        self.assertNotIn("FreeCAD 1.1", env["PATH"])
        self.assertEqual(env["PYTHONNOUSERSITE"], "1")
        self.assertEqual(env["PYTHONUTF8"], "1")
        self.assertEqual(env["PYTHONIOENCODING"], "utf-8")

    def test_resolve_prediction_json_prefers_current_model_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            step_dir = root / "step - 副本2"
            step_dir.mkdir()
            (step_dir / "pillar annotation.json").write_text("{}", encoding="utf-8")
            (step_dir / "M5-5402231_annotation.json").write_text("{}", encoding="utf-8")

            resolved = resolve_prediction_json_from_names(root, ["M5-5402231"])

        self.assertEqual(resolved.name, "M5-5402231_annotation.json")

    def test_resolve_prediction_json_returns_explicit_when_provided(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            step_dir = root / "step - 副本2"
            step_dir.mkdir()
            explicit = step_dir / "pillar annotation.json"
            explicit.write_text("{}", encoding="utf-8")
            (step_dir / "M5-5402231_annotation.json").write_text("{}", encoding="utf-8")

            resolved = resolve_prediction_json_from_names(
                root,
                ["M5-5402231"],
                explicit=str(explicit),
            )

        self.assertEqual(resolved.name, "pillar annotation.json")

    def test_resolve_source_step_from_annotation_json(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            step_dir = root / "step - 副本2"
            step_dir.mkdir()
            prediction = step_dir / "B pillar trim lower-0612_annotation.json"
            source_step = step_dir / "B pillar trim lower-0612.step"
            prediction.write_text("{}", encoding="utf-8")
            source_step.write_text("step", encoding="utf-8")

            resolved = resolve_source_step_from_prediction_json(prediction)

        self.assertEqual(resolved, source_step)

    def test_scan_clip_library_reads_bolt_cyl_geom(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "clip_a.json").write_text("{}", encoding="utf-8")
            (root / "clip_a.step").write_text("step", encoding="utf-8")
            (root / "clip_a.png").write_text("png", encoding="utf-8")
            (root / "clip_a.geom.json").write_text(
                json.dumps(
                    {
                        "faces": [
                            {"label": "BOLT_CYL", "radius": 4.0, "v_min": 0.0, "v_max": 2.0},
                            {"label": "BOLT_CYL", "radius": 4.0, "v_min": 0.0, "v_max": 4.0},
                        ]
                    }
                ),
                encoding="utf-8",
            )

            rows = scan_clip_library(root)

        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["name"], "clip_a")
        self.assertTrue(rows[0]["has_step"])
        self.assertTrue(rows[0]["thumbnail_path"].endswith("clip_a.png"))
        self.assertAlmostEqual(rows[0]["bolt_cyl_diameter_mm"], 8.0)
        self.assertAlmostEqual(rows[0]["bolt_cyl_height_mm"], 3.0)

    def test_scan_clip_library_uses_label_json_to_find_bolt_cyl_faces(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "clip_b.json").write_text(
                json.dumps({"faces": [{"index": 2, "type": "BOLT_CYL"}]}),
                encoding="utf-8",
            )
            (root / "clip_b.step").write_text("step", encoding="utf-8")
            (root / "clip_b.geom.json").write_text(
                json.dumps(
                    {
                        "faces": [
                            {"face_idx": 0, "face_type": "plane", "radius": 9.0, "depth": 9.0},
                            {"face_idx": 1, "face_type": "cylinder", "radius": 2.5, "depth": 2.3},
                        ]
                    }
                ),
                encoding="utf-8",
            )

            rows = scan_clip_library(root)

        self.assertAlmostEqual(rows[0]["bolt_cyl_diameter_mm"], 5.0)
        self.assertAlmostEqual(rows[0]["bolt_cyl_height_mm"], 2.3)


if __name__ == "__main__":
    unittest.main()
