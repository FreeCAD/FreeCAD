macro(SetupJSON)

    if(FREECAD_USE_EXTERNAL_JSON)
        find_package(nlohmann_json REQUIRED)
    else(FREECAD_USE_EXTERNAL_JSON)
        set(nlohmann_json_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/3rdParty/json/single_include)
    endif(FREECAD_USE_EXTERNAL_JSON)

endmacro(SetupJSON)
