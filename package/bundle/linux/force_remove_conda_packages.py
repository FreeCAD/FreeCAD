#!/usr/bin/env python3
# pylint: disable=missing-class-docstring,missing-function-docstring,import-outside-toplevel,abstract-class-instantiated
"""
This is the Arcane Scroll of Conda Package Removal, you need to be a level 92 binary sorcerer to
use it. Trying to understand it fully is likely to deplete your mana. Keep potions at the ready.

This script serves double duty: it's a more or less regular Conda package cleanup tool (without
having to actually invoke Conda tooling from pixi), and a tool to forcefully crowbar dependencies
out of a target conda environment while patching those dependencies out at the binary level.

It reads the manifest JSON files in the `conda-meta` directory and builds an in-memory
representation of the installed packages, forward and reverse dependencies between them, file lists
and size totals, as well as what files are executables, be they runnable or library.

Removing a package marks dependent packages as broken for later patching, and depended-on packages
as orphan if nothing refers to them any more. Those explicitly requested on the command line are
removed first, then waves of orphan removals happen until only broken packages are left.

Broken packages are scanned for references to symbols (functions and objects) that came from
removed packages, and are patched (`Executable.patch()`) to redirect these symbols to a
substitution library so that they exist and calling them will either return some init failure/error
code, or crash the process outright according to the ruleset passed on the CLI.
This is done to satisfy dynamic linkers which would otherwise complain about the missing symbols,
as those cannot be provided by the bundle (AppImage, ...) and are not guaranteed to exist on the
host system.

There is a provision to leave some libraries referenced as broken from the perspective of the Conda
environment, where those are expected to be available on the host; most notably Mesa, libdrm &
friends, or any library whose implementation is dispatched to a dynamically loaded lib who always
favors the host (Vulkan, OpenCL, ...).

While this script was made for Linux AppImage bundling in mind, it is actually pretty
platform-agnostic and has support for detecting Windows .exe & .dlls as well as macOS .dylib.
All that's missing is an implementation of `PEExecutable` and `MachOExecutable` information &
patching classes respectively, and compiler support in `build_substitution_library()`.
"""

import argparse
import enum
import json
import os
import platform
import re
import subprocess
from abc import ABC, abstractmethod
from collections import defaultdict
from collections.abc import Iterable, Iterator
from dataclasses import dataclass, field
from datetime import datetime
from io import StringIO
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import Dict, Set, List, Union, Any, Optional, Tuple, Callable, TypeAlias

import yaml

PackageName: TypeAlias = str


@dataclass(frozen=True)
class Package:
    meta_path: Path
    name: PackageName
    depends: List[str]
    files: List[str]
    size_in_bytes: int
    executable_paths: List[Path]


class SymbolType(enum.Enum):
    FUNCTION = "func"
    OBJECT = "object"


@dataclass(frozen=True, order=True)
class Symbol:
    name: str
    type: SymbolType
    size: int
    origin_library: str = ""

    @property
    def demangled_name(self) -> str:
        """Display name of the symbol."""
        if self.name[0:2] == "_Z":
            import cxxfilt

            return cxxfilt.demangle(self.name, external_only=False)
        return self.name


class SymbolSet(Iterable[Symbol]):
    def __init__(self, symbols: Optional[Iterable[Symbol]] = None, /):
        self.symbols: Dict[str, Symbol] = {sym.name: sym for sym in symbols or []}

    @property
    def names(self) -> Set[str]:
        """Set of all symbol names."""
        return set(self.symbols.keys())

    def __getitem__(self, item):
        if isinstance(item, str):
            return self.symbols[item]
        raise ValueError

    def __iter__(self) -> Iterator[Symbol]:
        return iter(self.symbols.values())

    def __repr__(self):
        if len(self.symbols) == 0:
            return "<empty SymbolSet>"
        return f"<SymbolSet of {' '.join(sorted(self.symbols.keys()))}>"

    def union(self, *sets: Iterable[Symbol]) -> "SymbolSet":
        """
        Merges two ``SymbolSet``s together.
        For any duplicate ``Symbol``, keeps the one with the most information.
        """
        symbols = dict(self.symbols)
        for new_set in sets:
            for new_sym in new_set:
                try:
                    if new_sym.size > symbols[new_sym.name].size:
                        symbols[new_sym.name] = new_sym
                except KeyError:
                    symbols[new_sym.name] = new_sym
        return SymbolSet(symbols.values())

    def intersection(self, *sets: Iterable[Symbol]) -> "SymbolSet":
        """
        Extracts the common set of ``Symbol``s in two ``SymbolSet``s.
        For any common ``Symbol``, keeps the one with the most information.
        """
        symbols = dict(self.symbols)
        for new_set in sets:
            for extra_sym in symbols.keys() - set(sym.name for sym in new_set):
                del symbols[extra_sym]
            for new_sym in new_set:
                try:
                    if new_sym.size > symbols[new_sym.name].size:
                        symbols[new_sym.name] = new_sym
                except KeyError:
                    # Not in ``self`` therefore not shared.
                    pass
        return SymbolSet(symbols.values())


def executables_in_file_list(root: Path, files: Iterable[str]) -> List[Path]:
    executable_paths: List[Path] = []
    for file_path_str in files:
        # Use heuristics to speed up discrimination of executable and non-executable files
        definitely_executable = (
            file_path_str.endswith((".so", ".dylib", ".exe", ".dll"))
            or ".so." in file_path_str
        )
        maybe_executable = not definitely_executable and "." not in file_path_str
        # Skip files whose names don't indicate an executable
        if not (definitely_executable or maybe_executable):
            continue
        # Skip files that are gone
        executable_path = root / file_path_str
        if not executable_path.is_file():
            continue
        if definitely_executable:
            executable_paths.append(executable_path)
        if maybe_executable:
            try:
                Executable(executable_path)
                executable_paths.append(executable_path)
            except NotAnExecutableError:
                # Skip adding to executable paths.
                pass
    return executable_paths


def is_executable_a_library(path: Path) -> bool:
    return path.suffix in ("so", "dylib", "dll") or ".so." in path.name


# pylint: disable-next=too-many-instance-attributes
class Environment:
    def __init__(self, root: Path):
        self.root = root
        conda_meta_path: Path = root / "conda-meta"
        if not conda_meta_path.is_dir():
            raise FileNotFoundError("conda-meta not in environment")

        self.packages: Dict[PackageName, Package] = {}
        """Packages installed in this environment. Removed packages are moved to ``removed``."""
        for meta_path in sorted(conda_meta_path.glob("*.json")):
            with meta_path.open("r") as meta_file:
                meta = json.load(meta_file)
            package = Package(
                meta_path=meta_path,
                name=meta["name"],
                depends=[d.split(" ", maxsplit=1)[0] for d in meta["depends"]],
                files=meta["files"],
                size_in_bytes=sum(
                    f["size_in_bytes"]
                    for f in meta["paths_data"]["paths"]
                    if f["path_type"] == "hardlink"
                ),
                executable_paths=executables_in_file_list(self.root, meta["files"]),
            )
            self.packages[package.name] = package

        self.rdepends: Dict[PackageName, Set[PackageName]] = defaultdict(set)
        for package in self.packages.values():
            for dependency_name in package.depends:
                self.rdepends.setdefault(dependency_name, set()).add(package.name)

        self.broken: Set[PackageName] = set()
        self.removed: Dict[PackageName, Package] = {}

        # Definition of self.orphans depends on leaf_packages
        self.leaf_packages = set()
        self.leaf_packages = set(self.orphans)

        self.library_paths: Dict[str, Path] = {}
        for package in self.packages.values():
            for file in package.files:
                if ".so" in file or ".dylib" in file or ".dll" in file:
                    path = self.root / file
                    self.library_paths[path.name] = path
        self._library_exported_symbols_cache: Dict[str, SymbolSet] = {}
        self.referenced_removed_symbols = SymbolSet()

    @property
    def orphans(self) -> Set[PackageName]:
        return set(self.packages.keys()) - self.rdepends.keys() - self.leaf_packages

    def packages_matching_name_globs(self, globs: Iterable[str]) -> List[PackageName]:
        # glob.translate is not available prior to Python 3.13,
        # implement a super basic version of "globs" instead.
        package_name_re = re.compile(
            "|".join(re.escape(str(pg)).replace("\\*", ".*") for pg in globs)
        )
        return [p for p in self.packages if package_name_re.match(p)]

    def remove_package(self, package_name: PackageName):
        package = self.packages[package_name]
        # Mark any package that depends on the one being removed as broken.
        self.broken.update(self.rdepends[package_name])
        # Update reverse dependency map to remove the package.
        for dependency in package.depends:
            try:
                self.rdepends[dependency].remove(package_name)
                if len(self.rdepends[dependency]) == 0:
                    del self.rdepends[dependency]
            except KeyError:
                pass
        # Unmark broken on package being deleted, if applicable.
        try:
            self.broken.remove(package_name)
        except KeyError:
            pass
        self.removed[package_name] = package
        del self.packages[package_name]

    def patch_package(
        self,
        package_name: PackageName,
        *,
        dry_run: bool,
        substitution_library_name: str,
        substituted_symbol_namer: Callable[[str], str],
        keep_needed_libs_pattern: Optional[re.Pattern[str]],
    ):
        package = self.packages[package_name]
        for executable_path in package.executable_paths:
            if executable_path.is_symlink():
                continue
            patch_result = Executable(executable_path).patch(
                self,
                dry_run=dry_run,
                substitution_library_name=substitution_library_name,
                substituted_symbol_namer=substituted_symbol_namer,
                keep_needed_libs_pattern=keep_needed_libs_pattern,
            )
            self.referenced_removed_symbols = self.referenced_removed_symbols.union(
                patch_result.referenced_removed_symbols
            )

    def get_library_exported_symbols(self, library_name: str) -> SymbolSet:
        try:
            return self._library_exported_symbols_cache[library_name]
        except KeyError:
            exe = Executable(self.library_paths[library_name])
            symbols = exe.exported_symbols
            self._library_exported_symbols_cache[library_name] = symbols
            return symbols


class NotAnExecutableError(RuntimeError):
    pass


@dataclass(frozen=True)
class PatchResult:
    referenced_removed_symbols: SymbolSet = field(default_factory=SymbolSet)


class Executable(ABC):
    """
    Wrapper for an executable binary, whether runnable or library.
    Instantiating with ``Executable(...)`` will return the appropriate `Executable`` subclass
    for the binary format of the file that's been passed.
    """

    def __new__(cls, path: Path):
        if cls is not Executable:
            return super(Executable, cls).__new__(cls)
        with path.open("rb") as io:
            magic = io.read(4)
            if magic == b"\x7fELF":
                return ELFExecutable.__new__(ELFExecutable, path)
            if magic == b"\xfe\xed\xfa\xcf":
                return MachOExecutable.__new__(ELFExecutable, path)
            if magic[:2] == b"MZ":
                return PEExecutable.__new__(ELFExecutable, path)
            if magic == b"INPU":  # No, not "INPUT", we only read 4 bytes.
                raise NotAnExecutableError(
                    f"{path} is a linker INPUT() forwarder, remove it prior to running this script"
                )
            raise NotAnExecutableError(f"{path} is not an executable")

    def __init__(self, path: Path):
        self.path = path

    @property
    @abstractmethod
    def exported_symbols(self) -> SymbolSet: ...

    @property
    @abstractmethod
    def imported_symbols(self) -> SymbolSet: ...

    @property
    @abstractmethod
    def needed_library_names(self) -> Set[str]: ...

    @abstractmethod
    def patch_library(
        self,
        *,
        add_needed: Optional[Iterable[str]] = None,
        remove_needed: Optional[Iterable[str]] = None,
        replace_needed: Optional[Iterable[Tuple[str, str]]] = None,
        replace_symbol: Optional[Iterable[Tuple[str, str]]] = None,
    ): ...

    def patch(
        self,
        env: Environment,
        *,
        dry_run: bool,
        substitution_library_name: str,
        substituted_symbol_namer: Callable[[str], str],
        keep_needed_libs_pattern: Optional[re.Pattern[str]],
    ) -> PatchResult:
        removed_library_names = set(
            Path(f).name
            for removed in env.removed.values()
            for f in removed.executable_paths
            if is_executable_a_library(f)
        )
        needed_library_names = self.needed_library_names
        libraries_to_patch_out = needed_library_names.intersection(
            removed_library_names
        )
        libraries_to_keep: Set[str] = set()
        if keep_needed_libs_pattern:
            for lib in libraries_to_patch_out:
                if keep_needed_libs_pattern.match(lib):
                    libraries_to_keep.add(lib)
            libraries_to_patch_out -= libraries_to_keep
        if len(libraries_to_patch_out) == 0:
            return PatchResult()
        print(
            f"  - {self.path.name}: {' '.join(sorted(libraries_to_patch_out))}"
            + (
                f' (keeping {" ".join(sorted(libraries_to_keep))} as requested)'
                if len(libraries_to_keep) > 0
                else ""
            )
        )

        exported_from_removed_libs = SymbolSet().union(
            *(
                env.get_library_exported_symbols(library_name)
                for library_name in libraries_to_patch_out
            )
        )
        imported = self.imported_symbols
        referenced_removed_symbols = exported_from_removed_libs.intersection(imported)

        if not dry_run:
            self.patch_library(
                add_needed=(substitution_library_name,),
                remove_needed=libraries_to_patch_out,
                replace_symbol=[
                    (sym.name, substituted_symbol_namer(sym.name))
                    for sym in referenced_removed_symbols
                ],
            )

        return PatchResult(referenced_removed_symbols=referenced_removed_symbols)


class ELFExecutable(Executable):
    ST_TYPE_TO_SYMBOLTYPE = {
        "STT_OBJECT": SymbolType.OBJECT,
        "STT_FUNC": SymbolType.FUNCTION,
    }
    EXCLUDED_SYMBOLS = {"_init", "_fini"}

    @classmethod
    def _symbol_from_elftools(cls, symbol, origin_library: str = "") -> Symbol:
        return Symbol(
            name=symbol.name,
            type=cls.ST_TYPE_TO_SYMBOLTYPE[symbol.entry.st_info.type],
            size=symbol.entry.st_size,
            origin_library=origin_library,
        )

    @property
    def exported_symbols(self) -> SymbolSet:
        with self.path.open("rb") as io:
            from elftools.elf.elffile import ELFFile

            elf = ELFFile(io)
            dynsym = elf.get_section_by_name(".dynsym")
            return SymbolSet(
                self._symbol_from_elftools(sym, self.path.name)
                for sym in dynsym.iter_symbols()
                if sym.entry.st_value != 0
                and sym.entry.st_shndx != "SHN_UNDEF"
                and sym.entry.st_info.bind in ("STB_GLOBAL", "STB_WEAK")
                and sym.entry.st_info.type not in ("STT_NOTYPE", "STT_TLS")
                and sym.name != ""
                and sym.name not in self.EXCLUDED_SYMBOLS
            )

    @property
    def imported_symbols(self) -> SymbolSet:
        with self.path.open("rb") as io:
            from elftools.elf.elffile import ELFFile

            elf = ELFFile(io)
            dynsym = elf.get_section_by_name(".dynsym")
            return SymbolSet(
                self._symbol_from_elftools(sym)
                for sym in dynsym.iter_symbols()
                if sym.entry.st_value == 0
                and sym.entry.st_shndx == "SHN_UNDEF"
                and sym.entry.st_info.bind in ("STB_GLOBAL", "STB_WEAK")
                and sym.entry.st_info.type not in ("STT_NOTYPE", "STT_TLS")
                and sym.name != ""
                and sym.name not in self.EXCLUDED_SYMBOLS
            )

    @property
    def needed_library_names(self) -> Set[str]:
        needed_sonames: Set[str] = set()
        with self.path.open("rb") as io:
            from elftools.elf.elffile import ELFFile

            elf = ELFFile(io)
            try:
                dynamic = next(elf.iter_segments(type="PT_DYNAMIC"))
            except StopIteration:
                # ELF has no dynamic section
                return needed_sonames
            for tag in dynamic.iter_tags():
                if tag.entry.d_tag == "DT_NEEDED":
                    needed_sonames.add(tag.needed)
        return needed_sonames

    def patch_library(
        self,
        *,
        add_needed: Optional[Iterable[str]] = None,
        remove_needed: Optional[Iterable[str]] = None,
        replace_needed: Optional[Iterable[Tuple[str, str]]] = None,
        replace_symbol: Optional[Iterable[Tuple[str, str]]] = None,
    ):
        cmdline: List[Union[str, Path]] = ["patchelf"]
        for soname in add_needed or []:
            cmdline.extend(("--add-needed", soname))
        for soname in remove_needed or []:
            cmdline.extend(("--remove-needed", soname))
        for old, new in replace_needed or []:
            cmdline.extend(("--replace-needed", old, new))
        rename_file_path = ""
        if replace_symbol:
            with NamedTemporaryFile(
                "w", prefix="FreeCAD_subst_", delete=False
            ) as rename_file:
                rename_file_path = rename_file.name
                for old, new in replace_symbol:
                    rename_file.write(f"{old} {new}\n")
                    # print(f"{self.path} : {rename_file.name}: {old} → {new}")
            cmdline.extend(("--rename-dynamic-symbols", rename_file_path))
        cmdline.append(self.path)
        try:
            subprocess.run(cmdline, check=True)
        finally:
            if rename_file_path:
                os.unlink(rename_file_path)


class MachOExecutable(Executable, ABC):
    # Implement if necessary
    pass


class PEExecutable(Executable, ABC):
    # Implement if necessary
    pass


SUBSTITUTION_LIBRARY_SOURCE_PREAMBLE = r"""
#include <cstdint>
#include <cstdio>

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(__CYGWIN__)
#  include <atomic>
#  define EXPORT __declspec(dllexport)
#  define NOINLINE __declspec(noinline)
#  define fence() std::atomic_signal_fence(std::memory_order_seq_cst)
#  define unreachable __assume(0)
#else
#  define EXPORT __attribute__((weak))
#  define NOINLINE __attribute__((noinline))
#  define fence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#  define unreachable __builtin_unreachable()
#endif

static int* volatile FreeCADSymbolSubst_deliberate_null = nullptr;
static const char constzeroes[1024] = {0};

NOINLINE [[noreturn]] static void FreeCADSymbolSubst_crash(const char* symbol) {
  fprintf(stderr,
    "%s() called yet it is removed from FreeCAD distribution.\n"
    "This is a packaging problem, please report the issue and include this error message at "
    "https://github.com/FreeCAD/FreeCAD/issues\n",
    symbol
  );
  fflush(stderr);
  fence();
  volatile int* p = FreeCADSymbolSubst_deliberate_null;
  *p = 42;
  fence();
  unreachable;
}

NOINLINE static void FreeCADSymbolSubst_log(const char* symbol) {
  fprintf(stderr, "%s() called but stubbed in FreeCAD distribution.\n", symbol);
}
"""


# Compiles a library file (.so/.dylib/.dll) to substitute for the missing symbols
# introduced as a result of removing some packages.
# Intentionally keeps duplicate crash functions as separate funcs instead
# of using symbol aliasing in order to preserve visible stack traces.
def build_substitution_library(
    *,
    substitution_rules: Dict[str, Any],
    referenced_removed_symbols: SymbolSet,
    substituted_symbol_namer: Callable[[str], str],
    output_path: Path,
):
    cxx = os.environ.get("CXX", "clang++" if platform.system() == "Darwin" else "g++")
    is_msvc = cxx.lower() in ("cl", "cl.exe", "clang-cl", "clang-cl.exe")

    for r in [r for r in substitution_rules.keys() if r[0] == "$"]:
        del substitution_rules[r]
    rules_names = [*substitution_rules.keys()]
    rule_re = re.compile(f"^{'|'.join(f'({r})' for r in substitution_rules.keys())}$")
    source = StringIO()
    source.write(SUBSTITUTION_LIBRARY_SOURCE_PREAMBLE)

    def write_func(
        return_type: str,
        name: str,
        *,
        body: str = "",
        return_value: str = "",
        attr: str = "",
    ):
        source.write(f'extern "C" EXPORT {attr} {return_type} {name}() {{\n')
        if body:
            source.write(f"  {body}\n")
        if return_value != "":
            source.write(f"  return {return_value};\n")
        source.write("}\n\n")

    for symbol in referenced_removed_symbols:
        match = rule_re.match(symbol.name)
        if not match or match.lastindex is None or match.lastindex <= 0:
            raise ValueError(
                f'No substitution rule for symbol "{symbol.name}" from {symbol.origin_library}'
            )
        rule = substitution_rules[rules_names[match.lastindex - 1]]
        if rule.get("crash", False) is True:
            write_func(
                "void",
                substituted_symbol_namer(symbol.name),
                attr="[[noreturn]]",
                body=f'FreeCADSymbolSubst_crash("{symbol.demangled_name}");',
            )
        elif "return" in rule:
            if "value" in rule["return"]:
                write_func(
                    rule["return"]["type"],
                    substituted_symbol_namer(symbol.name),
                    body=f'FreeCADSymbolSubst_log("{symbol.name}");',
                    return_value=rule["return"]["value"],
                )
            else:
                write_func(
                    rule["return"]["type"], substituted_symbol_namer(symbol.name)
                )
        elif "object" in rule:
            if symbol.type != SymbolType.OBJECT:
                raise ValueError(
                    f'Symbol "{symbol.name}" is defined as object substitution '
                    f"but the symbol is a {symbol.type}"
                )
            if rule["object"].get("array", False):
                if symbol.size == 0:
                    raise ValueError(
                        f'Symbol "{symbol.name}" wants to be a {rule["object"]["type"]} array '
                        f"but the symbol has no size"
                    )
                source.write(
                    f'{rule["object"]["type"]} {substituted_symbol_namer(symbol.name)}'
                    f'[{symbol.size}] = {rule["object"]["init"]};\n'
                )
    start = datetime.now()
    if is_msvc:  # pylint: disable=no-else-raise
        raise NotImplementedError(
            "Compiling the substitution library with MSVC not yet implemented"
        )
    else:
        subprocess.run(
            [
                cxx,
                "-fno-optimize-sibling-calls",
                "-fno-omit-frame-pointer",
                "-fno-exceptions",
                "-Wl,--build-id=none",
                "-shared",
                "-fPIC",
                "-O2",
                "-x",
                "c++",
                "-o",
                output_path,
                "-",
            ],
            check=True,
            input=source.getvalue().encode(),
        )
    print(
        f"Compiled {output_path.name} in {(datetime.now() - start).total_seconds():.3f} s"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--dry-run", action="store_true")
    parser.add_argument(
        "-e",
        "--environment",
        dest="environment_path",
        type=Path,
        required=True,
        help="Environment directory to work in (containing conda-meta).",
    )
    parser.add_argument(
        "--keep-needed-libs",
        dest="keep_needed_libs",
        type=re.compile,
        nargs="?",
        help="Regex of needed libraries (DT_NEEDED, ...) to keep.",
    )
    parser.add_argument(
        "-s",
        "--substitution-rules",
        dest="substitution_rules_path",
        type=Path,
        required=True,
        help="YAML file containing rules for building the symbol substitution library",
    )
    parser.add_argument("--print-rdeps-before-removal", action="store_true")
    parser.add_argument("package_globs", type=str, nargs="+")
    return parser.parse_args()


def get_substitution_library_path(env_path: Path) -> Path:
    system = platform.system()
    if system in ("Linux", "FreeBSD", "OpenBSD"):
        return env_path / "lib" / "libFreeCADSymbolSubst.so"
    if system == "Darwin":
        return env_path / "lib" / "libFreeCADSymbolSubst.dylib"
    if system == "Windows":
        return env_path / "bin" / "FreeCADSymbolSubst.dll"
    raise NotImplementedError(f'OS "{system}" not supported')


def print_removal_summary(env: Environment, all_orphans: Set[PackageName]):
    bytes_untouched = sum(r.size_in_bytes for r in env.packages.values())
    bytes_to_remove = sum(r.size_in_bytes for r in env.removed.values())
    bytes_total = bytes_untouched + bytes_to_remove
    print(
        f"\nWill remove ({bytes_to_remove / 1024 / 1024:.1f}/{bytes_total / 1024 / 1024:.1f} MiB, "
        f"{bytes_to_remove / bytes_total * 100:.1f}%):"
    )
    for removed in sorted(
        env.removed.values(), key=lambda r: r.size_in_bytes, reverse=True
    ):
        print(
            f"remove: {removed.name} ({removed.size_in_bytes / 1024 / 1024:.2f} MiB"
            f"{', orphaned' if removed.name in all_orphans else ''})"
        )
        for removed_library_name in sorted(
            set(
                Path(f).name
                for f in removed.executable_paths
                if is_executable_a_library(f)
            )
        ):
            print(f"  - {removed_library_name}")


def freecad_substituted_symbol_namer(name: str):
    if name[:2] == "_Z":
        # If it's a C++ Itanium ABI mangled name, prepend a FCSub:: namespace.
        insert_pos = min(name.find(d) for d in "0123456789" if d in name)
        return f"{name[:insert_pos]}5FCSub{name[insert_pos:]}"
    return f"FCSub_{name}"


def main():
    args = parse_args()
    substitution_library_path = get_substitution_library_path(args.environment_path)
    env = Environment(args.environment_path)

    if args.print_rdeps_before_removal:
        name_max_len = max(len(name) for name in env.rdepends.keys())
        for package_name, rdeps in sorted(env.rdepends.items()):
            print(f"{package_name:>{name_max_len}} ← {' '.join(rdeps)}")

    packages_to_remove = env.packages_matching_name_globs(args.package_globs)

    # Skip the packages requested for removal from the leaf packages that must be kept in the env
    env.leaf_packages -= set(packages_to_remove)
    print("\nKeeping these leaf packages:")
    for leaf in sorted(env.leaf_packages):
        print(f"leaf: {leaf}")

    # Remove the requested packages
    for package_name in packages_to_remove:
        env.remove_package(package_name)

    # Remove orphans until the used package set settles
    all_orphans: Set[PackageName] = set()
    while len(orphans := env.orphans) > 0:
        all_orphans |= orphans
        for orphan in orphans:
            env.remove_package(orphan)

    print_removal_summary(env, all_orphans)

    print("\nPatching:")
    for broken in sorted(env.broken):
        print(
            f"patch: {broken} (wants "
            f"{' '.join(d for d in env.packages[broken].depends if d in env.removed)})"
        )
        env.patch_package(
            broken,
            dry_run=args.dry_run,
            substitution_library_name=substitution_library_path.name,
            substituted_symbol_namer=freecad_substituted_symbol_namer,
            keep_needed_libs_pattern=args.keep_needed_libs,
        )

    print("\nRemoving files", end="")
    removed_file_count = 0
    if not args.dry_run:
        for package in env.removed.values():
            removed_file_count += len(package.files)
            for file_path_str in package.files:
                file_path = env.root / file_path_str
                file_path.unlink(missing_ok=True)
            package.meta_path.unlink()
    print(f" ({removed_file_count} removed)")

    print("\nRemoved symbols still referenced to include in substitution library:")
    for removed in sorted(env.referenced_removed_symbols):
        print(f"- {removed.name}")

    with args.substitution_rules_path.open("r") as io:
        substitution_rules = yaml.safe_load(io)
    build_substitution_library(
        substitution_rules=substitution_rules,
        referenced_removed_symbols=env.referenced_removed_symbols,
        substituted_symbol_namer=freecad_substituted_symbol_namer,
        output_path=substitution_library_path,
    )


if __name__ == "__main__":
    main()
