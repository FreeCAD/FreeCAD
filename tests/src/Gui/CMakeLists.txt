# Standard C++ GTest tests
target_sources(Tests_run PRIVATE
        Assistant.cpp
        Camera.cpp
        StyleParameters/StyleParametersApplicationTest.cpp
        StyleParameters/ParserTest.cpp
        StyleParameters/ParameterManagerTest.cpp
)

# Qt tests
setup_qt_test(QuantitySpinBox)
