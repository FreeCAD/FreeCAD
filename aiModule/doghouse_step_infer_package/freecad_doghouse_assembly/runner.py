"""Subprocess bridge from FreeCAD to the existing doghouse AI scripts."""
from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


def default_doghouse_python(project_root: Path | None = None) -> Path:
    """Resolve the isolated backend interpreter without machine-specific paths."""
    configured = os.environ.get("DOGHOUSE_PYTHON")
    if configured:
        return Path(configured).expanduser()
    if project_root is not None:
        root = Path(project_root)
        candidates = [
            root / ".pixi" / "envs" / "default" / "python.exe",
            root / ".pixi" / "envs" / "default" / "bin" / "python",
        ]
        for candidate in candidates:
            if candidate.is_file():
                return candidate
    return Path(sys.executable)


# Compatibility name for older imports. It now points at this project's pixi
# interpreter when running from the source tree, never at D:/tools/envs/uv01.
DEFAULT_UV01_PYTHON = default_doghouse_python(Path(__file__).resolve().parent.parent)


@dataclass
class DoghouseRunner:
    project_root: Path
    python_exe: Path | None = None
    clip_library: Path | None = None

    def __post_init__(self):
        self.project_root = Path(self.project_root)
        self.python_exe = (
            Path(self.python_exe)
            if self.python_exe is not None
            else default_doghouse_python(self.project_root)
        )
        if self.clip_library is None:
            self.clip_library = self.project_root / "卡扣库"
        else:
            self.clip_library = Path(self.clip_library)

    @property
    def script_path(self) -> Path:
        return self.project_root / "doghouse_ai" / "recommend_and_assemble.py"

    @staticmethod
    def _path_arg(path: Path) -> str:
        return Path(path).as_posix()

    def _subprocess_env(self) -> dict[str, str]:
        env = dict(os.environ)
        env["KMP_DUPLICATE_LIB_OK"] = "TRUE"
        env["OMP_NUM_THREADS"] = "1"
        env["PYTHONNOUSERSITE"] = "1"
        # FreeCAD on Chinese Windows commonly inherits a GBK console.  The
        # backend emits Chinese diagnostics and Unicode symbols, so force a
        # portable UTF-8 pipe encoding instead of depending on that console.
        env["PYTHONUTF8"] = "1"
        env["PYTHONIOENCODING"] = "utf-8"
        env_root = self.python_exe.parent
        uv_paths = [
            env_root,
            env_root / "Library" / "bin",
            env_root / "Scripts",
        ]
        old_paths = [
            item
            for item in env.get("PATH", "").split(os.pathsep)
            if item and "freecad" not in item.lower()
        ]
        env["PATH"] = os.pathsep.join([str(path) for path in uv_paths] + old_paths)
        return env

    def build_recommend_command(
        self,
        step_path: Path,
        output_json: Path,
        *,
        prediction_json: Path | None = None,
    ) -> list[str]:
        cmd = [
            self._path_arg(self.python_exe),
            self._path_arg(self.script_path),
            "--auto-doghouse",
            "--step",
            self._path_arg(step_path),
            "--recommend-output",
            self._path_arg(output_json),
            "--all-doghouses-same-clip",
            "--doghouse-infer-output-dir",
            self._path_arg(Path(output_json).parent / "infer"),
            "--infer-cpu",
            "--no-assemble",
        ]
        if prediction_json:
            cmd.extend(["--prediction-json", self._path_arg(prediction_json)])
        return cmd

    def build_placement_command(
        self,
        step_path: Path,
        output_json: Path,
        *,
        clip_name: str,
        prediction_json: Path | None = None,
        invert_indices: list[int] | None = None,
    ) -> list[str]:
        cmd = [
            self._path_arg(self.python_exe),
            self._path_arg(self.script_path),
            "--auto-doghouse",
            "--step",
            self._path_arg(step_path),
            "--clip-name",
            clip_name,
            "--placement-output",
            self._path_arg(output_json),
            "--all-doghouses-same-clip",
            "--doghouse-infer-output-dir",
            self._path_arg(Path(output_json).parent / "infer"),
            "--infer-cpu",
            "--no-step-output",
        ]
        if prediction_json:
            cmd.extend(["--prediction-json", self._path_arg(prediction_json)])
        if invert_indices:
            cmd.extend(["--invert-direction-indices", ",".join(str(i) for i in invert_indices)])
        return cmd

    def run_json(self, cmd: list[str], output_json: Path) -> dict:
        if not self.project_root.is_dir():
            raise FileNotFoundError(f"Doghouse project root not found: {self.project_root}")
        if not self.script_path.is_file():
            raise FileNotFoundError(f"Doghouse backend script not found: {self.script_path}")
        if not self.python_exe.is_file():
            raise FileNotFoundError(
                f"Doghouse Python not found: {self.python_exe}. "
                "Run 'pixi install' in the project root or set DOGHOUSE_PYTHON."
            )
        output_json = Path(output_json)
        output_json.parent.mkdir(parents=True, exist_ok=True)
        log_path = output_json.with_suffix(".log.txt")
        log_path.write_text(
            "COMMAND:\n" + " ".join(cmd) + "\n\n",
            encoding="utf-8",
        )
        startupinfo = None
        creationflags = 0
        if os.name == "nt":
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = 0
            creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        proc = subprocess.run(
            cmd,
            cwd=str(self.project_root),
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
            check=False,
            env=self._subprocess_env(),
            startupinfo=startupinfo,
            creationflags=creationflags,
        )
        log_path.write_text(
            log_path.read_text(encoding="utf-8")
            + f"RETURN CODE: {proc.returncode}\n\nSTDOUT:\n{proc.stdout}\n\nSTDERR:\n{proc.stderr}\n",
            encoding="utf-8",
        )
        if proc.returncode != 0:
            message = proc.stderr.strip() or proc.stdout.strip() or f"command failed: {proc.returncode}"
            message += f"\n\nLog: {log_path}"
            raise RuntimeError(message)
        if not output_json.exists():
            details = proc.stderr.strip() or proc.stdout.strip() or "no backend output"
            raise RuntimeError(f"Backend did not create {output_json}\n\n{details}")
        return json.loads(output_json.read_text(encoding="utf-8"))

    def recommend(self, step_path: Path, output_json: Path, *, prediction_json: Path | None = None) -> dict:
        return self.run_json(
            self.build_recommend_command(step_path, output_json, prediction_json=prediction_json),
            output_json,
        )

    def placement(
        self,
        step_path: Path,
        output_json: Path,
        *,
        clip_name: str,
        prediction_json: Path | None = None,
        invert_indices: list[int] | None = None,
    ) -> dict:
        return self.run_json(
            self.build_placement_command(
                step_path,
                output_json,
                clip_name=clip_name,
                prediction_json=prediction_json,
                invert_indices=invert_indices,
            ),
            output_json,
        )
