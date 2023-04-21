# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## 2.2.1 - (in progress)

### Fixed

- Fix compilation with [musl libc](https://musl.libc.org/) ([#70](https://github.com/asmaloney/libE57Format/pull/70)) (Thanks Dimitri!)
- Add missing include for [GCC 11](https://gcc.gnu.org/gcc-11/porting_to.html#header-dep-changes) ([#68](https://github.com/asmaloney/libE57Format/pull/68)) (Thanks bartoszek!)

## [2.2.0](https://github.com/asmaloney/libE57Format/releases/tag/v2.2.0) - 2021-04-01

### Added

- Added and updated the E57Simple API from the old reference library. ([#41](https://github.com/asmaloney/libE57Format/pull/41), [#63](https://github.com/asmaloney/libE57Format/pull/63)) (Thanks Jiri & Grégoire!)
- Enabled building E57Format as a shared library. ([#40](https://github.com/asmaloney/libE57Format/pull/40)) (Thanks Amodio!)
- Added a [clang-format](https://clang.llvm.org/docs/ClangFormat.html) file, a cmake target for it ("format"), and reformatted the code.
- {doc} Added info about using [SPDX License Identifiers](https://spdx.org/ids).
- {ci} Added GitHub Actions to build macOS, Linux, and Windows. ([#35](https://github.com/asmaloney/libE57Format/pull/35))

### Changed

- `E57_V1_0_URI` was changed from a `#define` to a `constexpr`, so if you use it, it will need to be updated with a namespace: `e57::E57_V1_0_URI`.
- {doc} Moved some documentation to new repo ([libE57Format-docs](https://github.com/asmaloney/libE57Format-docs)) and generate the [docs](https://asmaloney.github.io/libE57Format-docs/).
- {cmake} Reviewed and updated cmake files. CMake minimum version was changed to 3.10.

### Fixed

- Fixed building with E57_MAX_VERBOSE defined. ([#44](https://github.com/asmaloney/libE57Format/pull/44))
- {win} Fixed MSVC warnings. ([#34](https://github.com/asmaloney/libE57Format/pull/34), [#36](https://github.com/asmaloney/libE57Format/pull/36))

### Other

- Removed all internal usage of dynamic_cast<>. ([#39](https://github.com/asmaloney/libE57Format/pull/39)) (Thanks Jiri!)
- Split classes out from E57FormatImpl.[h,cpp] intot their own files.

## [2.1.0](https://github.com/asmaloney/libE57Format/releases/tag/v2.1) - 2020-04-01

### Added

- Added support for UTF8 file names on Windows. (based on [#26](https://github.com/asmaloney/libE57Format/issues/26))
- Added support for _char\*_ input. ([#22](https://github.com/asmaloney/libE57Format/pull/22))
- {cmake} Added fallback configuration for RelWithDebInfo and MinSizeRel. ([#29](https://github.com/asmaloney/libE57Format/pull/29))
- {cmake} Added a proper install configuration. ([#28](https://github.com/asmaloney/libE57Format/pull/28))

### Changed

- {cmake} Removed unused ICU requirement for Linux.

### Fixed

- {cmake} Marked xerces-c as required.

### Other

- {cmake} Various cleanups.
- Internal code cleanups.

## [2.0.1](https://github.com/asmaloney/libE57Format/releases/tag/v2.0.1) - 2019-01-15

### Fixed

- Writing files was broken and would produce the following error:
  > Error: bad API function argument provided by user (E57_ERROR_BAD_API_ARGUMENT) (ImageFileImpl.cpp line 109)

## [2.0.0](https://github.com/asmaloney/libE57Format/releases/tag/v2.0) - 2019-01-06

Forked from [E57RefImpl](https://sourceforge.net/projects/e57-3d-imgfmt/).

### Added

- Added a checksum policy (see _ReadChecksumPolicy_ in _E57Format.h_) so the library user can decide how frequently to check them.
- {win} Added cmake option ()`USING_STATIC_XERCES`) to tell the build if you are using a static Xerces lib.

### Changed

- Now requires C++11.
- Now requires cmake 3.1+.
- No longer uses BOOST.
- Turn off `E57_MAX_DEBUG` by default.

### Removed

- Removed all but the main sources for reading and writing E57 files.
- Removed "big endian" byte swap code (not sure it was working and no way to test).

### Fixed

- Multiple fixes for compilation on macOS.
- Fixed a couple of fallthrough bugs which would result in undefined behaviour.

### Other

- Improved file read times.
- Many, many code cleanups:
  - Refactored the code into multiple files.
  - Removed unused macros and code.
  - Removed non-useful comments.
  - Added proper initialization of class and struct members.
  - Modernized using c++11.
