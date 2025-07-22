SET(lazy_loader
    lazy_loader.py
    __init__.py
)
add_custom_target(lazy_loader ALL SOURCES
    ${lazy_loader}
)
SET_PYTHON_PREFIX_SUFFIX(lazy_loader)

fc_copy_sources(lazy_loader "${CMAKE_BINARY_DIR}/Ext/lazy_loader" ${lazy_loader})
install (FILES ${lazy_loader} DESTINATION "Ext/lazy_loader")

