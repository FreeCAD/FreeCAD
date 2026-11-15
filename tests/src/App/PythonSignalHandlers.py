# SPDX-License-Identifier: LGPL-2.1-or-later

"""Check signal delivery and crash-report access in a host Python interpreter."""


def write_report(path):
    import struct
    import zlib

    # Minimal v1 report: a 128-byte header, no frames or strings, and a CRC32 footer.
    header = bytearray(128)
    struct.pack_into("<II", header, 0, 0x52434346, 1)  # Magic and format version
    struct.pack_into("<q", header, 24, 1700000000)  # Timestamp
    struct.pack_into(
        "<10I",
        header,
        32,
        12345,  # Process ID
        0,  # Fault code
        0,  # Frame count
        132,  # File size, including footer
        0,  # Flags
        128,  # Frame table offset
        128,  # String table offset
        0xFFFFFFFF,  # No build ID
        0xFFFFFFFF,  # No version suffix
        0xFFFFFFFF,  # No minidump
    )
    path.write_bytes(header + struct.pack("<I", zlib.crc32(header)))


def main():
    import os
    import signal
    import sys
    from contextlib import ExitStack
    from pathlib import Path
    from tempfile import TemporaryDirectory

    module_dir = sys.argv[1]
    part_module_dir = sys.argv[2] if len(sys.argv) > 2 else None
    sys.path.insert(0, module_dir)
    libraries = ExitStack()
    if sys.platform == "win32":
        libraries.enter_context(os.add_dll_directory(module_dir))
        if part_module_dir:
            libraries.enter_context(os.add_dll_directory(part_module_dir))

    received = []

    def handler(signum, frame):
        received.append(signum)

    signals = [signal.SIGABRT, signal.SIGFPE, signal.SIGILL, signal.SIGSEGV]
    for name in ("SIGBUS", "SIGHUP", "SIGINT", "SIGQUIT", "SIGSYS"):
        if hasattr(signal, name):
            signals.append(getattr(signal, name))
    for signum in signals:
        signal.signal(signum, handler)

    def check_signals(stage):
        received.clear()
        # getsignal() only reports Python's cached handler: verify actual delivery.
        for signum in signals:
            print(f"Checking {signum.name} {stage}", flush=True)
            signal.raise_signal(signum)
        if received != signals:
            raise AssertionError(f"Expected {signals}, received {received} {stage}")

    with libraries, TemporaryDirectory(prefix="FreeCAD-PythonSignalHandlers-") as directory:
        os.environ["FREECAD_USER_HOME"] = directory
        os.environ["FREECAD_USER_DATA"] = directory
        reports_dir = Path(directory) / "CrashReports"
        reports_dir.mkdir()
        report_path = reports_dir / "crash-1700000000-12345.fcrash"
        write_report(report_path)

        import FreeCAD

        check_signals("after importing FreeCAD")

        if part_module_dir:
            sys.path.insert(0, part_module_dir)
            import Part

            check_signals("after importing Part")
            for iteration in range(2):
                box = Part.makeBox(10, 10, 10)
                chamfer = box.makeChamfer(1, [box.Edges[0]])
                if not chamfer.isValid() or not 0 < chamfer.Volume < box.Volume:
                    raise AssertionError("Chamfer did not produce a valid modified solid")
                check_signals(f"after chamfer {iteration}")
                cut = box.cut(Part.makeBox(5, 5, 5))
                if not cut.isValid() or abs(cut.Volume - 875) > 1e-7:
                    raise AssertionError("Cut did not produce the expected solid")
                check_signals(f"after boolean cut {iteration}")

        reports = FreeCAD.getCrashReports()
        if len(reports) != 1 or reports[0].process_id != 12345:
            raise AssertionError("Existing crash report was not loaded")
        if FreeCAD.getLastCrashReport().process_id != 12345:
            raise AssertionError("Existing crash report was not exposed as the latest report")
        if not (reports_dir / "archive" / report_path.name).is_file():
            raise AssertionError("Existing crash report was not archived")

        try:
            FreeCAD.Vector(1, 2, "invalid")
        except TypeError:
            pass
        else:
            raise AssertionError("Invalid API arguments did not raise a catchable TypeError")

        print("Host signals, crash-report access, and catchable API errors verified")


if __name__ == "__main__":
    main()
