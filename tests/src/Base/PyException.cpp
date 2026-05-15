// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Python.h>

#include <exception>
#include <numbers>
#include <stdexcept>

#include <PyCXX/CXX/Exception.hxx>

#include "Base/Exception.h"
#include "Base/PyException.h"

#include "CXX/Python3/Objects.hxx"

/**
 * @brief Test fixture for Base::pyWrapCppExceptions and the pyThrowWrapped* helpers.
 *
 * NOTE: Constructing a Py::Exception sets the active Python error indicator, so the interpreter
 * must be initialized and the error state cleared between tests.
 */
class PyExceptionTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        Py_Initialize();
        gilState = PyGILState_Ensure();
    }

    void TearDown() override
    {
        if (PyErr_Occurred() != nullptr) {
            PyErr_Clear();
        }
        PyGILState_Release(gilState);
    }

private:
    PyGILState_STATE gilState {};
};

TEST_F(PyExceptionTest, goodCallDoesNotThrow)  // NOLINT
{
    bool goodCall {false};
    goodCall = Base::pyWrapCppExceptions([&]() { return true; }, true);
    ASSERT_TRUE(goodCall);
}

TEST_F(PyExceptionTest, pythonExceptionsPropagate)  // NOLINT
{
    EXPECT_THROW( //NOLINT
        Base::pyWrapCppExceptions([&]() {
                throw Py::IndexError();
            }, true),
            Py::IndexError
    );
}

TEST_F(PyExceptionTest, outOfRangeMapsToIndexError)  // NOLINT
{
    EXPECT_THROW( //NOLINT
        Base::pyWrapCppExceptions([&]() {
                throw std::out_of_range("Test out-of-range exception");
            }, true),
            Py::IndexError
    );
}

TEST_F(PyExceptionTest, lengthErrorMapsToIndexError)  // NOLINT
{
    EXPECT_THROW( //NOLINT
        Base::pyWrapCppExceptions([&]() {
                throw std::length_error("Test length-error exception");
            }, true),
            Py::IndexError
    );
}

TEST_F(PyExceptionTest, otherLogicErrorsMapToValueError)  // NOLINT
{
    EXPECT_THROW( //NOLINT
        Base::pyWrapCppExceptions([&]() {
                throw std::logic_error("true = false");
            }, true),
            Py::ValueError
    );
}

TEST_F(PyExceptionTest, badAllocMapsToMemoryError)  // NOLINT
{
    EXPECT_THROW( //NOLINT
        Base::pyWrapCppExceptions([&]() {
                throw std::bad_alloc();
            }, true),
            Py::MemoryError
    );
}

TEST_F(PyExceptionTest, nonExceptionTypeMapsToRuntimError)  // NOLINT
{
    using ::testing::HasSubstr;
    try {
        Base::pyWrapCppExceptions([&]() { throw std::numbers::pi_v<float>; });
        FAIL() << "expected a Py::RuntimeError";
    }
    catch (Py::RuntimeError& e) {
        // This should have gotten demangled to yield something to do with a "float" (the exact
        // message is platform-dependent)
        EXPECT_THAT(e.errorValue().as_string(), HasSubstr("float"));
    }
    catch (...) {
        FAIL() << "expected Py::RuntimeError, got something else";
    }
}
