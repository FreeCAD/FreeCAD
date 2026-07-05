import os
import sys
import subprocess
import signal
import time
import threading
import atexit
import ctypes

import FreeCAD


_process = None
_lock = threading.Lock()
_job_handle = None
_atexit_registered = False


def _create_win_job():
    global _job_handle
    if sys.platform != "win32" or _job_handle is not None:
        return

    kernel32 = ctypes.windll.kernel32

    job = kernel32.CreateJobObjectW(None, None)
    if not job:
        return

    class JOBOBJECT_BASIC_LIMIT_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("PerProcessUserTimeLimit", ctypes.c_int64),
            ("PerJobUserTimeLimit", ctypes.c_int64),
            ("LimitFlags", ctypes.c_uint32),
            ("MinimumWorkingSetSize", ctypes.c_size_t),
            ("MaximumWorkingSetSize", ctypes.c_size_t),
            ("ActiveProcessLimit", ctypes.c_uint32),
            ("Affinity", ctypes.c_size_t),
            ("PriorityClass", ctypes.c_uint32),
            ("SchedulingClass", ctypes.c_uint32),
        ]

    class IO_COUNTERS(ctypes.Structure):
        _fields_ = [
            ("ReadOperationCount", ctypes.c_uint64),
            ("WriteOperationCount", ctypes.c_uint64),
            ("OtherOperationCount", ctypes.c_uint64),
            ("ReadTransferCount", ctypes.c_uint64),
            ("WriteTransferCount", ctypes.c_uint64),
            ("OtherTransferCount", ctypes.c_uint64),
        ]

    class JOBOBJECT_EXTENDED_LIMIT_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("BasicLimitInformation", JOBOBJECT_BASIC_LIMIT_INFORMATION),
            ("IoInfo", IO_COUNTERS),
            ("ProcessMemoryLimit", ctypes.c_size_t),
            ("JobMemoryLimit", ctypes.c_size_t),
            ("PeakProcessMemoryUsed", ctypes.c_size_t),
            ("PeakJobMemoryUsed", ctypes.c_size_t),
        ]

    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000
    JobObjectExtendedLimitInformation = 9

    info = JOBOBJECT_EXTENDED_LIMIT_INFORMATION()
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE

    kernel32.SetInformationJobObject(
        job,
        JobObjectExtendedLimitInformation,
        ctypes.byref(info),
        ctypes.sizeof(info),
    )

    _job_handle = job


def _assign_to_job(proc):
    if sys.platform != "win32" or _job_handle is None:
        return

    kernel32 = ctypes.windll.kernel32
    handle = kernel32.OpenProcess(0x1FFFFF, False, proc.pid)
    if handle:
        kernel32.AssignProcessToJobObject(_job_handle, handle)
        kernel32.CloseHandle(handle)


def _find_python():
    fc_bin_dir = os.path.dirname(os.path.abspath(sys.executable))
    for name in ("python.exe", "python3.exe", "python", "python3"):
        candidate = os.path.join(fc_bin_dir, name)
        if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate

    for name in ("python3", "python3.exe", "python", "python.exe"):
        for d in os.environ.get("PATH", "").split(os.pathsep):
            full = os.path.join(d, name)
            if os.path.isfile(full) and os.access(full, os.X_OK):
                if "WindowsApps" not in full:
                    return full

    return None


def _plugin_dir():
    return os.path.dirname(os.path.abspath(__file__))


def is_running():
    global _process
    with _lock:
        if _process is None:
            return False
        if _process.poll() is not None:
            _process = None
            return False
        return True


def _register_shutdown_hooks():
    global _atexit_registered
    if _atexit_registered:
        return
    _atexit_registered = True

    atexit.register(stop)

    try:
        from PySide6.QtWidgets import QApplication

        app = QApplication.instance()
        if app is not None:
            app.aboutToQuit.connect(stop)
    except Exception:
        pass


def start():
    global _process
    with _lock:
        if _process is not None and _process.poll() is None:
            FreeCAD.Console.PrintMessage(
                "Parashell RPC: Bridge server already running.\n"
            )
            return True

        python = _find_python()
        if python is None:
            FreeCAD.Console.PrintError(
                "Parashell MCP: Python not found in PATH or plugin .venv. "
                "Install Python 3.10+ with the 'mcp' package to enable the MCP bridge.\n"
            )
            return False

        plugin_dir = _plugin_dir()

        env = os.environ.copy()
        existing_pythonpath = env.get("PYTHONPATH", "")
        if existing_pythonpath:
            env["PYTHONPATH"] = plugin_dir + os.pathsep + existing_pythonpath
        else:
            env["PYTHONPATH"] = plugin_dir

        cmd = [
            python,
            "-c",
            "import bridge; bridge.main()",
        ]

        _create_win_job()

        popen_kwargs = {
            "cwd": plugin_dir,
            "env": env,
            "stdin": subprocess.PIPE,
            "stdout": subprocess.PIPE,
            "stderr": subprocess.PIPE,
        }
        if sys.platform == "win32":
            popen_kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW

        try:
            _process = subprocess.Popen(cmd, **popen_kwargs)
        except Exception as exc:
            FreeCAD.Console.PrintError(
                f"Parashell MCP: Failed to start MCP server: {exc}\n"
            )
            return False

        _assign_to_job(_process)

    _register_shutdown_hooks()

    time.sleep(0.5)

    with _lock:
        if _process is not None and _process.poll() is not None:
            exit_code = _process.returncode
            stderr_output = ""
            try:
                stderr_output = _process.stderr.read().decode("utf-8", errors="replace")
            except Exception:
                pass
            _process = None
            FreeCAD.Console.PrintError(
                f"Parashell MCP: Server exited immediately with code {exit_code}.\n"
            )
            if stderr_output:
                FreeCAD.Console.PrintError(f"Parashell MCP stderr: {stderr_output}\n")
            return False

    FreeCAD.Console.PrintMessage(
        f"Parashell MCP: Bridge server started (PID {_process.pid}); "
        "endpoint resolved via secure discovery.\n"
    )
    return True


def get_pid():
    with _lock:
        if _process is not None and _process.poll() is None:
            return _process.pid
    return None


def stop():
    global _process
    with _lock:
        if _process is None:
            return
        try:
            if sys.platform == "win32":
                _process.terminate()
            else:
                _process.send_signal(signal.SIGTERM)
            _process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            _process.kill()
            _process.wait(timeout=5)
        except Exception:
            pass
        finally:
            _process = None
    FreeCAD.Console.PrintMessage("Parashell MCP: Bridge server stopped.\n")
