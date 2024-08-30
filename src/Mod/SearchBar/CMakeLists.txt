set(Namespace_SRCS
    animAopt.gif
    animB2op.gif
    BuiltInSearchResults.py
    CMakeLists.txt
    GetItemGroups.py
    IndentedItemDelegate.py
    InitGui.py
    Init.py
    RefreshTools.py
    ResultsDocument.py
    ResultsPreferences.py
    ResultsRefreshTools.py
    ResultsToolbar.py
    SafeViewer.py
    screenshot.png
    SearchBoxLight.py
    SearchBox.py
    SearchResults.py
    Serialize.py
    Tango-System-search.svg
    Tango-Tools-spanner-hammer.svg
)



SOURCE_GROUP("" FILES ${Namespace_SRCS})

ADD_CUSTOM_TARGET(SEARCHBAR ALL
        SOURCES ${Namespace_SRCS}
)

fc_copy_sources(SEARCHBAR "${CMAKE_BINARY_DIR}/Mod/SearchBar" ${Namespace_SRCS})

INSTALL(
    FILES
        ${Namespace_SRCS}
    DESTINATION
        Mod/SearchBar
)
