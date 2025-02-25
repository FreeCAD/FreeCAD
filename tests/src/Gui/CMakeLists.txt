# Standard C++ GTest tests
target_sources(Tests_run PRIVATE
        Assistant.cpp
)

# Qt tests
setup_qt_test(QuantitySpinBox)
