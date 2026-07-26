set(types_file "${BRLOADER_SOURCE_DIR}/include/types.h")
file(READ "${types_file}" types_content)

if(NOT types_content MATCHES "TopTools_ShapeMapHasher")
  string(REPLACE
    "#include <TopoDS_Shape.hxx>\n#include <TopAbs_ShapeEnum.hxx>"
    "#include <TopoDS_Shape.hxx>\n#include <Standard_Version.hxx>\n#if OCC_VERSION_HEX >= 0x070800\n#include <TopTools_ShapeMapHasher.hxx>\n#endif\n#include <TopAbs_ShapeEnum.hxx>"
    types_content "${types_content}")
  string(REPLACE
    "        return shape.HashCode(INT_MAX);"
    "#if OCC_VERSION_HEX >= 0x070800\n        return TopTools_ShapeMapHasher{}(shape);\n#else\n        return shape.HashCode(INT_MAX);\n#endif"
    types_content "${types_content}")
  file(WRITE "${types_file}" "${types_content}")
endif()

set(brloader_cmake "${BRLOADER_SOURCE_DIR}/CMakeLists.txt")
file(READ "${brloader_cmake}" cmake_content)

if(NOT cmake_content MATCHES "BRLOADER_OCCT_STEP_LIBRARY")
  string(REPLACE
    "find_package(OpenCASCADE REQUIRED)\n"
    "find_package(OpenCASCADE REQUIRED)\n\nfind_library(BRLOADER_OCCT_STEP_LIBRARY NAMES TKDESTEP TKSTEP\n  HINTS \${OpenCASCADE_LIBRARY_DIR})\nif(NOT BRLOADER_OCCT_STEP_LIBRARY)\n  message(FATAL_ERROR \"Could not find the OpenCascade STEP library\")\nendif()\n"
    cmake_content "${cmake_content}")
  string(REPLACE "  TKSTEP \n" "  \${BRLOADER_OCCT_STEP_LIBRARY}\n"
    cmake_content "${cmake_content}")
  file(WRITE "${brloader_cmake}" "${cmake_content}")
endif()
