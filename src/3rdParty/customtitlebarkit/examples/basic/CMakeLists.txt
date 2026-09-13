add_executable(basic_example main.cpp)
target_link_libraries(basic_example PRIVATE customtitlebarkit Qt6::Widgets)

add_custom_command(TARGET basic_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../default.qss
        $<TARGET_FILE_DIR:basic_example>/default.qss
)
