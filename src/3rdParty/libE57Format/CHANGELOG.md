# Changelog

All notable changes to this project will be documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 3.3.0 - (in progress)

### Added

- {cmake} Generate a package version file ([#316](https://github.com/asmaloney/libE57Format/pull/316)) (Thanks SunBlack!)

### Changed

- {cmake} Require XercesC 3.2 and remove `USING_STATIC_XERCES` ([#317](https://github.com/asmaloney/libE57Format/pull/317)) (Thanks SunBlack!)

### Fixed

- {cmake} Added missing include for GNUInstallDirs ([#315](https://github.com/asmaloney/libE57Format/pull/315)) (Thanks Adrian!)

  If you built without testing on, the cmake files were not installed to the correct location.

## [3.2.0](https://github.com/asmaloney/libE57Format/releases/tag/v3.2.0) - 2024-06-27

### Added

- {cmake} Added `E57_INSTALL_CMAKEDIR` option to override where the cmake files are installed. ([#305](https://github.com/asmaloney/libE57Format/pull/305))

  This defaults to `lib/cmake/E57Format` and the path is relative to `CMAKE_INSTALL_PREFIX`.

### Changed

- {cmake} If building as a shared library, set [VERSION](https://cmake.org/cmake/help/v3.15/prop_tgt/VERSION.html) & [SOVERSION](https://cmake.org/cmake/help/v3.15/prop_tgt/SOVERSION.html). ([#304](https://github.com/asmaloney/libE57Format/pull/304))
- {cmake} Don't force warnings as errors when building self. ([#299](https://github.com/asmaloney/libE57Format/pull/299))
- {cmake} Use git tag in library version for more precision. ([#298](https://github.com/asmaloney/libE57Format/pull/298))
- {license} Re-license all my (asmaloney) MIT files to BSL-1.0. It is just easier to have it all under one license. ([#296](https://github.com/asmaloney/libE57Format/pull/296))
- {format} Update to clang-format 18 & reformat code. ([#286](https://github.com/asmaloney/libE57Format/pull/286))
- Add "E57\_" to macros in E57Exception.h. ([#285](https://github.com/asmaloney/libE57Format/pull/285))
- "De-deprecate" methods in **E57SimpleWriter**. These methods can be useful when writing batches. ([#284](https://github.com/asmaloney/libE57Format/pull/284))

### Fixed

- Update CRCpp to fix MSVC and MinGW warnings. ([#302](https://github.com/asmaloney/libE57Format/pull/302))
- Fix warnings about redefinition of `_LARGEFILE64_SOURCE` and `__LARGE64_FILES` ([#301](https://github.com/asmaloney/libE57Format/pull/301)) (Thanks Niklas!)
- Fix building with emscripten. (Note that the project doesn't officially support emscripten.) ([#288](https://github.com/asmaloney/libE57Format/pull/288)) (Thanks maz-1!)
- {standard conformance} CompressedVectors always write an index packet. This is required by the standard (9.3.5). ([#295](https://github.com/asmaloney/libE57Format/pull/295))
- {standard conformance} **E57SimpleReader** accepts files containing zero scans. ([#283](https://github.com/asmaloney/libE57Format/pull/283))
- {cmake} Replace deprecated "exec_program" with "execute_process". ([#282](https://github.com/asmaloney/libE57Format/pull/282))
- Fix potential invalid range exceptions when reading integer nodes. ([#278](https://github.com/asmaloney/libE57Format/pull/278))

## [3.1.1](https://github.com/asmaloney/libE57Format/releases/tag/v3.1.1) - 2023-11-28

### Fixed

- Fix library version in CMakeLists.txt. (Thanks Olli!)

## [3.1.0](https://github.com/asmaloney/libE57Format/releases/tag/v3.1.0) - 2023-11-20

### Added

- {cmake} New option `E57_RELEASE_LTO` controls whether link-time optimization is on for release builds. It defaults to `ON`. ([#254](https://github.com/asmaloney/libE57Format/pull/254))

  CMake forces "thin" LTO (see [this issue](https://gitlab.kitware.com/cmake/cmake/-/issues/23136)) which is a problem if compiling statically for distribution (e.g. in a package manager). Generally you will only want to turn this off for distributing static release builds.

### Changed

- Trying to read a Data3D with zero records which has an ill-formed header will now throw an `ErrorData3DReadInvalidZeroRecords` exception instead of `ErrorInternal`. ([#264](https://github.com/asmaloney/libE57Format/pull/264))
  > Note that previous versions of this library (and E57RefImpl itself) could write these headers incorrectly. This was also fixed in this release (see below).
- {cmake} Remove `E57_VISIBILITY_HIDDEN` option. ([#259](https://github.com/asmaloney/libE57Format/pull/259))

  I cannot get extern templates to work across all of gcc, clang, apple clang, and MSVC when using "hidden" visibility with shared libraries.

### Fixed

- Fix #include to avoid MSVC compilation error with Visual Studio 2017 ([#268](https://github.com/asmaloney/libE57Format/pull/268)) (Thanks Thomas!)
- {standard conformance} A compressed vector with 0 records must still write a data packet. ([#266](https://github.com/asmaloney/libE57Format/pull/266))
- {standard conformance} Fix reading a compressed vector with 0 points which has an empty data packet. ([#267](https://github.com/asmaloney/libE57Format/pull/267))
- {standard conformance} Compressed vectors with an invalid section ID now throw an `ErrorBadCVPacket` exception if `E57_VALIDATION_LEVEL` > 0. ([#265](https://github.com/asmaloney/libE57Format/pull/265))
- Fix clang warnings about implicit conversions in _SourceDestBufferImpl.cpp_. Apple's clang doesn't warn about these, but it looks like the official clang releases do. ([#257](https://github.com/asmaloney/libE57Format/pull/257)) (Thanks Martin!)

## [3.0.2](https://github.com/asmaloney/libE57Format/releases/tag/v3.0.2) - 2023-07-22

### Fixed

- Fix `E57_VERBOSE` build. ([#251](https://github.com/asmaloney/libE57Format/pull/251)) (Thanks Jia!)
- {standard conformance} Fix invalid range exception in FloatNode implementation. ([#250](https://github.com/asmaloney/libE57Format/pull/250))
- Fix reading of index packets. ([#249](https://github.com/asmaloney/libE57Format/pull/249))
- Fix several places where we should be checking for MSVC, not WIN32. ([#248](https://github.com/asmaloney/libE57Format/pull/248))
- Fix "unnecessary semicolons" warnings which prevented building with GCC <= 10. ([#241](https://github.com/asmaloney/libE57Format/pull/241)) (Thanks Andre!)

## [3.0.1](https://github.com/asmaloney/libE57Format/releases/tag/v3.0.1) - 2023-03-15

### Added

- {ci} Added an MSVC 32-bit build. ([#235](https://github.com/asmaloney/libE57Format/pull/235))

### Fixed

- {cmake} Turn off inter-procedural optimization in debug builds. Link-time optimization can make debugging more difficult. ([#240](https://github.com/asmaloney/libE57Format/pull/240))
- {cmake} Don't force a debug postfix if `CMAKE_DEBUG_POSTFIX` is defined.([#239](https://github.com/asmaloney/libE57Format/pull/239))
- {cmake} Don't force install locations. This prevents overriding them using `CMAKE_INSTALL_BINDIR` & `CMAKE_INSTALL_LIBDIR`.([#237](https://github.com/asmaloney/libE57Format/pull/237))
- Fix warnings which prevented building on 32-bit systems. ([#233](https://github.com/asmaloney/libE57Format/pull/233), [#234](https://github.com/asmaloney/libE57Format/pull/234))

## [3.0.0](https://github.com/asmaloney/libE57Format/releases/tag/v3.0.0) - 2023-02-23

There have been _many_ changes around the `Simple API` in this release to fix some problems, avoid potential errors, and simplify the use of the API. Where possible these changes are marked as `deprecated` to be removed later. The compiler will produce warnings for these indicating what needs to change. Other changes could not be marked deprecated but will break the API, requiring code changes. The notable ones are marked with 🚧 below.

### Added

- Add address (ASan) and undefined behaviour (UBSan) sanitizers. These are controlled by `E57FORMAT_SANITIZE_ALL`, `E57FORMAT_SANITIZE_ADDRESS` and `E57FORMAT_SANITIZE_ADDRESS`. They are not included when building with MSVC.
- Add new `E57Version.h` header for more convenient access to version information. ([#197](https://github.com/asmaloney/libE57Format/pull/197)).
- Add `ImageFile::extensionsLookupPrefix()` overload for more concise user code. ([#161](https://github.com/asmaloney/libE57Format/pull/161))
- Added a constructor & destructor for **E57SimpleData**'s `Data3DPointsData_t`. This will create all the required buffers based on an `e57::Data3D` struct and handle their cleanup. See the `SimpleWriter` tests for examples. ([#149](https://github.com/asmaloney/libE57Format/pull/149))

  > **Note:** I strongly recommend these new constructors be used to simplify your code and help prevent errors.

- A new **E57SimpleReader** constructor takes a `ReaderOptions` struct which allows setting the checksum policy.
  ```cpp
  Reader( const ustring &filePath, const ReaderOptions &options );
  ```
- {test} Added testing using [GoogleTest](https://github.com/google/googletest). For details, please see [test/README.md](test/README.md) ([#121](https://github.com/asmaloney/libE57Format/pull/121))
- Added `E57Exception::errorStr()` to get the error string directly. ([#128](https://github.com/asmaloney/libE57Format/pull/128))
- {cmake} Use [ccache](https://ccache.dev/) if available. ([#129](https://github.com/asmaloney/libE57Format/pull/129))
- {ci} Added a CI check for proper clang-formatted code. ([#125](https://github.com/asmaloney/libE57Format/pull/125))

### Changed

- Now requires a **[C++14](https://en.cppreference.com/w/cpp/14)** compatible compiler.
- Now requires **[CMake](https://cmake.org/) >= 3.15**. ([#205](https://github.com/asmaloney/libE57Format/pull/205))
- 🚧 **DEBUG** and **VERBOSE** macros were changed to simplify and clarify:
  - New `E57_ENABLE_DIAGNOSTIC_OUTPUT` controls the inclusion of the code for the `dump()` functions on nodes. I don't see any real reason to turn this off, but I left the capability in for compatibility.
  - New `E57_VALIDATION_LEVEL=N` replaces `E57_DEBUG` (N=1) and `E57_MAX_DEBUG` (N=2).
  - `E57_MAX_VERBOSE` was consolidated with `E57_VERBOSE` since they were essentially the same.
- When building itself, warnings are now treated as errors. ([#205](https://github.com/asmaloney/libE57Format/pull/205), [#211](https://github.com/asmaloney/libE57Format/pull/211))
- Clean up global const and enum names to use the `e57` namespace and avoid repetition. ([#176](https://github.com/asmaloney/libE57Format/pull/176))
  - i.e. instead of `e57::E57_STRUCTURE`, we now use `e57::TypeStructure`
- {format} Update clang-format rules for clang-format 15. ([#168](https://github.com/asmaloney/libE57Format/pull/168), [#179](https://github.com/asmaloney/libE57Format/pull/179))
- Change default checksum policies to an enum. ([#166](https://github.com/asmaloney/libE57Format/pull/166))
  Old | New
  --|--
  CHECKSUM_POLICY_NONE | ChecksumPolicy::None
  CHECKSUM_POLICY_SPARSE | ChecksumPolicy::Sparse
  CHECKSUM_POLICY_HALF | ChecksumPolicy::Half
  CHECKSUM_POLICY_ALL | ChecksumPolicy::All
- Avoid implicit conversion in constructors. ([#135](https://github.com/asmaloney/libE57Format/pull/135))
- Update [CRCpp](https://github.com/d-bahr/CRCpp) to 1.2. ([#130](https://github.com/asmaloney/libE57Format/pull/130))
- **E57Exception** changes ([#118](https://github.com/asmaloney/libE57Format/pull/118)):
  - mark methods as `noexcept`
  - use `private` instead of `protected`
- Rename **E57Simple**'s `Data3DPointsData` and `Data3DPointsData_d` structs to `Data3DPointsFloat` and `Data3DPointsDouble` respectively. ([#180](https://github.com/asmaloney/libE57Format/pull/180))
- 🚧 **E57Simple:** Specifying the node type for cartesian & spherical points, time stamp, and intensity is now explicit using new fields (`pointRangeNodeType`, `angleNodeType`, `timeNodeType`, and `intensityNodeType`) and a new enum (`NumericalNodeType`). ([#178](https://github.com/asmaloney/libE57Format/pull/178))
  - For examples, please see _test/src/testSimpleWriter.cpp_.
- Simplify the **E57SimpleWriter** API with `WriteImage2DData()` for images and `WriteData3DData()` for 3D data. This reduces code, hides complexity, and avoids potential errors. ([#171](https://github.com/asmaloney/libE57Format/pull/171))
  - As part of this simplification, `WriteData3DData()` will now fill in any missing min/max values for cartesian & spherical points, intensity, and time stamps by looking at the data.([#175](https://github.com/asmaloney/libE57Format/pull/175))
- 🚧 **E57Simple:** Intensity now uses `double` instead of `float`. ([#178](https://github.com/asmaloney/libE57Format/pull/178))
- 🚧 **E57Simple:** Colours now use `uint16_t` instead of `uint8_t`. ([#167](https://github.com/asmaloney/libE57Format/pull/167))
- 🚧 Change **E57SimpleData**'s Data3D field name from `pointsSize` to `pointCount`. ([#164](https://github.com/asmaloney/libE57Format/pull/164))
- 🚧 Min/max fields in **E57SimpleData**'s Data3D's point fields were a mix of floats and doubles. Since the fields are doubles, set them all to doubles and use the new `Data3DPointsData_t` constructor to set them properly for floats. ([#153](https://github.com/asmaloney/libE57Format/pull/153))
  > **Note:** If you were previously relying on these to be floats and are not using the new `Data3DPointsData_t` constructor, you will need to set these.
- 🚧 Renamed the [E57_EXT_surface_normals](http://www.libe57.org/E57_EXT_surface_normals.txt) extension's fields in **E57SimpleData**'s `PointStandardizedFieldsAvailable` to be in line with existing code. ([#149](https://github.com/asmaloney/libE57Format/pull/149))
  - `normalX` renamed to `normalXField`
  - `normalY` renamed to `normalYField`
  - `normalZ` renamed to `normalZField`

### Deprecated

- `e57::Utilities::getVersions()`. ([#197](https://github.com/asmaloney/libE57Format/pull/197))
- `e57::Data3DPointsData` and `e57::Data3DPointsData_d` types. ([#180](https://github.com/asmaloney/libE57Format/pull/180))
- Many global const and enum names. The compiler will produce warnings including the replacement symbols (note that enumerators will not produce warnings on C++14, but they will on C++17). ([#176](https://github.com/asmaloney/libE57Format/pull/176))
- `e57::Writer::NewImage2D`and `e57::Writer::SetUpData3DPointsData`. ([#171](https://github.com/asmaloney/libE57Format/pull/171))
- Old default checksum policies (`CHECKSUM_POLICY_NONE`, `CHECKSUM_POLICY_SPARSE`, `CHECKSUM_POLICY_HALF`, and `CHECKSUM_POLICY_ALL`). ([#166](https://github.com/asmaloney/libE57Format/pull/166))
- The old `e57::Reader` constructor taking only `filePath`. ([#139](https://github.com/asmaloney/libE57Format/pull/139))
- The old `e57::Writer` constructor taking only `filePath`. ([#117](https://github.com/asmaloney/libE57Format/pull/117))

### Fixed

- Fix several potential divide-by-zero cases. ([#224](https://github.com/asmaloney/libE57Format/pull/224))
- {ci} Now builds shared library versions as well. ([#219](https://github.com/asmaloney/libE57Format/pull/219))
- {win} Fixes shared library build warnings. ([#215](https://github.com/asmaloney/libE57Format/pull/215), [#216](https://github.com/asmaloney/libE57Format/pull/216), [#217](https://github.com/asmaloney/libE57Format/pull/217))
- Fix the code which shortens floating point numbers when converted to strings. The impact of it being incorrect was negligible since it's just the floating point representation in the XML portion of the file. ([#214](https://github.com/asmaloney/libE57Format/pull/214))
- Turned on a lot of compiler warnings and fixed them. ([#201](https://github.com/asmaloney/libE57Format/pull/201), [#202](https://github.com/asmaloney/libE57Format/pull/202), [#203](https://github.com/asmaloney/libE57Format/pull/203), [#204](https://github.com/asmaloney/libE57Format/pull/204), [#205](https://github.com/asmaloney/libE57Format/pull/205), [#207](https://github.com/asmaloney/libE57Format/pull/207), [#209](https://github.com/asmaloney/libE57Format/pull/209))
- Fix writing floating point numbers when `std::locale::global` is set. ([#174](https://github.com/asmaloney/libE57Format/pull/174))
- E57XmlParser: Parse doubles in a locale-independent way. ([#173](https://github.com/asmaloney/libE57Format/pull/173)) (Thanks Hugal31!)
- E57SimpleReader: Ensure scaled integer fields are set as best we can when reading. ([#158](https://github.com/asmaloney/libE57Format/pull/158))
- Fix the [E57_EXT_surface_normals](http://www.libe57.org/E57_EXT_surface_normals.txt) extension's URI in **E57SimpleWriter**. ([#143](https://github.com/asmaloney/libE57Format/pull/143))
- {win} Fix conversion warning when compiling with debug on. ([#124](https://github.com/asmaloney/libE57Format/pull/124))
- Add errno detail to `E57_ERROR_OPEN_FAILED` exception. ([#119](https://github.com/asmaloney/libE57Format/pull/119), [#120](https://github.com/asmaloney/libE57Format/pull/120))

## [2.3.0](https://github.com/asmaloney/libE57Format/releases/tag/v2.3.0) - 2022-10-04

### Added

- {cmake} Added `E57_VISIBILITY_HIDDEN` option to control [CXX_VISIBILITY_PRESET](https://cmake.org/cmake/help/latest/prop_tgt/LANG_VISIBILITY_PRESET.html). Defaults to `ON`. ([#104](https://github.com/asmaloney/libE57Format/pull/104)) (Thanks Nigel!)
- Added BSD support. ([#99](https://github.com/asmaloney/libE57Format/pull/99)) (Thanks Christophe!)

### Changed

- Updated &amp; reorganized the [online API docs](https://asmaloney.github.io/libE57Format-docs/) and changed to a [cleaner theme](https://github.com/jothepro/doxygen-awesome-css).
- Change file creation to use _0666_ permissions on [POSIX](https://en.wikipedia.org/wiki/POSIX) systems. ([#105](https://github.com/asmaloney/libE57Format/pull/105)) (Thanks Nigel!)
- {cmake} [CXX_VISIBILITY_PRESET](https://cmake.org/cmake/help/latest/prop_tgt/LANG_VISIBILITY_PRESET.html) is now set and `ON` by default. ([#104](https://github.com/asmaloney/libE57Format/pull/104)) (Thanks Nigel!)
- A new **E57SimpleWriter** constructor takes a `WriterOptions` struct which allows setting the file's GUID.
  ```cpp
  Writer( const ustring &filePath, const WriterOptions &options );
  ```
  The old constructor taking only `coordinateMetadata` is deprecated and will be removed in the future. ([#96](https://github.com/asmaloney/libE57Format/pull/96)) (Thanks Nigel!)
- Change `E57_DEBUG`, `E57_MAX_DEBUG`, `E57_VERBOSE`, `E57_MAX_VERBOSE`, `E57_WRITE_CRAZY_PACKET_MODE` from **#defines** to cmake options. ([#80](https://github.com/asmaloney/libE57Format/pull/80)) (Thanks Nigel!)

### Fixed

- Fix **E57SimpleWriter**'s writing of invalid quaternions. It now defaults to the identity quaternion. ([#108](https://github.com/asmaloney/libE57Format/pull/108)) (Thanks Nigel!)
- Fix **E57SimpleReader** to handle missing `images2D` and `isAtomicClockReferenced` nodes. ([#90](https://github.com/asmaloney/libE57Format/pull/90)) (Thanks Olli!)
- Fix **BitpackIntegerDecoder** sometimes reading past end of input buffer. ([#87](https://github.com/asmaloney/libE57Format/pull/87)) (Thanks Nigel!)
- Fix compilation when some debug options are set. ([#81](https://github.com/asmaloney/libE57Format/pull/81), [#82](https://github.com/asmaloney/libE57Format/pull/82), [#84](https://github.com/asmaloney/libE57Format/pull/84)) (Thanks Nigel!)
- Fix compilation with [musl libc](https://musl.libc.org/) ([#70](https://github.com/asmaloney/libE57Format/pull/70)) (Thanks Dimitri!)
- Add missing include for [GCC 11](https://gcc.gnu.org/gcc-11/porting_to.html#header-dep-changes) ([#68](https://github.com/asmaloney/libE57Format/pull/68)) (Thanks bartoszek!)

**Note:** The next release will be 3.0 and will require a [C++14](https://en.cppreference.com/w/cpp/14) compiler.

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
- Split classes out from E57FormatImpl.[h,cpp] into their own files.

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
