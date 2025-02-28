# FreeCAD C++ to Python Binding System Manual

Welcome to the new Python-based binding system for exposing FreeCAD C++ APIs to Python. This system replaces the previous XML-based approach with a more direct and flexible Python interface, allowing C++ developers to define Python bindings using native Python syntax, type annotations, and decorators.

* * *

## Table of Contents

* [Overview](#overview)
* [Key Features](#key-features)
* [Core Components](#core-components)
* [Defining Bindings in Python](#defining-bindings-in-python)
    * [Metadata Decorators](#metadata-decorators)
    * [Class Definitions](#class-definitions)
    * [Method Overloading](#method-overloading)
    * [Attributes and Read-Only Properties](#attributes-and-read-only-properties)
* [Example](#example)
* [Getting Started](#getting-started)
* [Advanced Topics](#advanced-topics)
* [Conclusion](#conclusion)

* * *

## Overview

The new Python-based binding system allows you to create bindings between FreeCAD’s C++ APIs and Python directly within Python source files. By leveraging Python’s native features—such as type annotations, decorators, and overloads—you can produce well-documented, type-safe Python interfaces that closely mirror your C++ classes.

This system is designed to be fully compatible and backwards-compatible with the previous XML-based system but offers the following advantages:

* **Direct Python Syntax:** Write bindings directly in Python without an intermediary XML representation.
* **Enhanced Readability:** Preserve detailed documentation and formatting in docstrings.
* **Type Safety:** Use Python type hints to ensure that the Python interface accurately reflects the C++ API.
* **Decorator-Based Metadata:** Attach metadata to classes and methods to control binding behavior.
* **Method Overloads:** Define multiple method signatures using `@overload` for improved clarity and support of type hinting for Python overloads.
* **Comprehensive Documentation:** Maintain detailed developer and user documentation directly in the Python stubs.

* * *

## Core Components

The binding system is built around a few core components:

* **Metadata Decorators:**
    A set of decorators (e.g., `@export`, `@constmethod`, `@sequence_protocol`) to annotate classes and methods with necessary metadata for the binding process. These decorators help bridge the gap between the C++ definitions and the Python interface.

* **C++ Python Stub Generation:**
    The system generates C++ Python stubs that act as a direct mapping to the corresponding C++ classes. These stubs include method signatures, attributes, and detailed docstrings and uses the same code
    as the previous XML-based system.

* **Type Annotations and Overloads:**
    Utilize Python's type hints and the `@overload` decorator from the `typing` module to accurately represent C++ method signatures, including support for overloaded methods.

* * *

## Defining Bindings in Python

### Metadata Decorators

The core decorator, `@export`, is used to attach binding-related metadata to a class. This metadata includes information such as the C++ class name, header files, namespaces, and more.

**Example:**

```python
from Metadata import export, constmethod
from PyObjectBase import PyObjectBase

@export(
    Father="PyObjectBase",
    Name="PrecisionPy",
    Twin="Precision",
    TwinPointer="Precision",
    Include="Base/Precision.h",
    Namespace="Base",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
)
class PrecisionPy(PyObjectBase):
    """
    Base.Precision class.

    This class provides precision values for various numerical operations
    in the FreeCAD environment.
    """
    ...
```

### Class Definitions

Classes are defined in a way that closely mirrors the C++ counterparts. The Python classes use decorators to attach metadata and include docstrings that retain original formatting.

### Method Overloading

For methods that require multiple signatures (overloads), use the `@overload` decorator. A final implementation that handles variable arguments (`*args`, `**kwargs`) is provided as a placeholder.

**Example:**

```python
from typing import overload

class QuantityPy(PyObjectBase):
    @overload
    def toStr(self) -> str: ...

    @overload
    def toStr(self, decimals: int) -> str: ...

    def toStr(self, decimals: int = ...) -> str:
        """
        toStr([decimals])
        Returns a string representation of the quantity, rounded to the specified number of decimals.
        """
        ...
```

The `@overload` variants are not actually used by the generator, but solely for the purpose of
providing Python type hinting to be used by type checkers like mypy.

### Attributes and Read-Only Properties

Attributes defined as read-only are annotated with `Final` from Python’s `typing` module to indicate immutability.

**Example:**

```python
from typing import Final, Tuple

class UnitPy(PyObjectBase):
    # holds the unit type as a string, e.g. 'Area'.
    Type: Final[str] = ...

    # Returns the signature.
    Signature: Final[Tuple] = ...
```

* * *

## Metadata Decorators Reference

This section details each metadata decorator and helper function used to attach auxiliary binding information to your Python classes and methods.

* * *

### 1. `metadata`

#### **Purpose**

The `metadata` decorator attaches a set of key-value pairs as metadata to a class. This metadata informs the binding generator about various aspects of the corresponding C++ API, such as its name, header file, namespace, inheritance, and twin (or native) types.

#### **Usage**

```python
from Metadata import export

@export(
    Father="PyObjectBase",
    Name="PrecisionPy",
    Twin="Precision",
    TwinPointer="Precision",
    Include="Base/Precision.h",
    Namespace="Base",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
)
class PrecisionPy(PyObjectBase):
    """
    Base.Precision class.

    This class provides precision values for various numerical operations
    in the FreeCAD environment.
    """
    # Class implementation goes here...
```

#### **Parameters**

* **Arbitrary Keyword Arguments (`**kwargs`):**
    These may include:

    * `Father`: The name of the parent class in Python.
    * `Name`: The name of the current Python binding C++ class.
    * `Twin`: The name of the corresponding C++ class.
    * `TwinPointer`: The pointer type of the twin C++ class.
    * `Include`: The header file where the C++ class is declared.
    * `Namespace`: The C++ namespace of the class.
    * `FatherInclude`: The header file for the parent class.
    * `FatherNamespace`: The C++ namespace for the parent class.

    _(Additional keys can be added as required by the binding generator.)_

* * *

### 2. `constmethod`

#### **Purpose**

The `constmethod` decorator marks a method as a constant method. In C++ bindings, this means that the method does not modify the state of the object and should be treated as `const`. This can affect the generated C++ method signature and enforce read-only behavior in Python where applicable.

#### **Usage**

```python
from Metadata import constmethod

class ExamplePy(PyObjectBase):

    @constmethod()
    def getValue(self) -> int:
        """
        Returns an integer value without modifying the object.
        """
        # Actual implementation goes here...
```

* * *

### 3. `no_args`

#### **Purpose**

The `no_args` decorator is used to indicate that a method should be called without any arguments. This is
to signal that fact to the generator so it knows to generate the correct C++ API signature.

#### **Usage**

```python
from Metadata import no_args

class ExamplePy(PyObjectBase):

    @no_args()
    def reset(self) -> None:
        """
        Resets the state of the object.
        """
        # Implementation goes here...
```

#### **Parameters**

* **None:**
    This decorator acts as a marker. It does not modify the method behavior at runtime but provides metadata to the binding generator.

#### **Behavior**

When the binding generator encounters the `no_args` decorator, it ensures that the generated Python stub does not expect any parameters beyond the implicit `self`, matching the no-argument signature of the underlying C++ method.

* * *

### 4. `forward_declarations`

#### **Purpose**

The `forward_declarations` decorator allows you to attach a snippet of source code containing forward declarations to a class. Forward declarations are useful when the binding process requires awareness of other classes or types before their full definitions are encountered.

#### **Usage**

```python
from Metadata import forward_declarations

@forward_declarations("""
class OtherType;
struct HelperStruct;
""")
class ExamplePy(PyObjectBase):
    """
    Example class that depends on OtherType and HelperStruct.
    """
    # Class implementation goes here...
```

#### **Parameters**

* **`source_code` (str):**
    A string containing the forward declarations in C++ syntax.

#### **Behavior**

This decorator attaches the provided forward declarations to the class (typically in a `__forward_declarations__` attribute). During stub generation or C++ header generation, these declarations are inserted at the appropriate location.

* * *

### 5. `class_declarations`

#### **Purpose**

The `class_declarations` decorator is similar to `forward_declarations` but is used for attaching additional class declarations. This may include extra helper classes, enums, or typedefs that are needed for the proper functioning of the bindings.

#### **Usage**

```python
from Metadata import class_declarations

@class_declarations("""
enum Status {
    SUCCESS,
    FAILURE
};

typedef std::vector<int> IntVector;
""")
class ExamplePy(PyObjectBase):
    """
    Example class with extra class declarations.
    """
    # Class implementation goes here...
```

#### **Parameters**

* **`source_code` (str):**
    A string containing extra class or type declarations that supplement the binding.

#### **Behavior**

The decorator stores the provided declarations in an attribute (e.g., `__class_declarations__`) so that the binding generator can include these in the final generated files.

* * *

### 6. `sequence_protocol`

#### **Purpose**

The `sequence_protocol` decorator is used to declare that a class implements Python’s sequence protocol. This includes support for operations like indexing, slicing, iteration, and length retrieval. By attaching protocol metadata, you can control how the binding system exposes these behaviors.

#### **Usage**

```python
from Metadata import sequence_protocol

@sequence_protocol(
    sq_length=True,
    sq_concat=False,
    sq_repeat=False,
    sq_item=True,
    mp_subscript=True,
    sq_ass_item=True,
    mp_ass_subscript=False,
    sq_contains=False,
    sq_inplace_concat=False,
    sq_inplace_repeat=False,
)
class ContainerPy(PyObjectBase):
    """
    A container class that implements Python's sequence protocol.
    """
    ...
```

#### **Parameters**

* **Arbitrary Keyword Arguments (`**kwargs`):**
    * `sq_length` (bool): Whether the class supports element access via indexing.
    * `sq_concat` (bool): Whether the class is iterable.
    * `sq_repeat` (bool): Whether slicing operations are supported.
    * `sq_item` (bool): Whether the class supports element access via indexing.
    * `mp_subscript` (bool): Whether the class is iterable.
    * `sq_ass_item` (bool): Whether slicing operations are supported.
    * `sq_ass_item` (bool):
    * `mp_ass_subscript` (bool):
    * `sq_contains` (bool):
    * `sq_inplace_concat` (bool):
    * `sq_inplace_repeat` (bool):

#### **Behavior**

The decorator attaches a `__sequence_protocol__` attribute to the class with the provided dictionary. This metadata is later used to generate the appropriate sequence operations in the Python API stubs.

#### Sequence Protocol Callbacks

1. **`sq_length` → `static Py_ssize_t sequence_length(PyObject *)`**

    * **Purpose:**
        Implements the “length” function for the object.
    * **Usage in Python:**
        When you call `len(obj)`, this function is invoked to determine how many items are in the sequence.
    * **C API Mapping:**
        This function fills the `sq_length` slot in the `PySequenceMethods` structure.
2. **`sq_concat` → `static PyObject* sequence_concat(PyObject *, PyObject *)`**

    * **Purpose:**
        Implements the concatenation operation for sequences.
    * **Usage in Python:**
        This is called when two sequence objects are added together using the `+` operator (e.g., `a + b`).
    * **C API Mapping:**
        This function is assigned to the `sq_concat` slot in `PySequenceMethods`.
3. **`sq_repeat` → `static PyObject * sequence_repeat(PyObject *, Py_ssize_t)`**

    * **Purpose:**
        Implements the repetition operation for sequences.
    * **Usage in Python:**
        It is invoked when a sequence is multiplied by an integer using the `*` operator (e.g., `a * n`), creating a new sequence with repeated elements.
    * **C API Mapping:**
        This function is installed in the `sq_repeat` slot of `PySequenceMethods`.
4. **`sq_item` → `static PyObject * sequence_item(PyObject *, Py_ssize_t)`**

    * **Purpose:**
        Implements element access via integer indexing.
    * **Usage in Python:**
        When you access an element using `obj[index]`, this function is called to retrieve the item.
    * **C API Mapping:**
        It fills the `sq_item` slot in `PySequenceMethods`.
5. **`sq_ass_item` → `static int sequence_ass_item(PyObject *, Py_ssize_t, PyObject *)`**

    * **Purpose:**
        Implements assignment (or deletion) of an element via an integer index.
    * **Usage in Python:**
        This function is used when an item is assigned (e.g., `obj[index] = value`) or deleted (`del obj[index]`).
    * **C API Mapping:**
        It is set into the `sq_ass_item` slot of the `PySequenceMethods` structure.
6. **`sq_contains` → `static int sequence_contains(PyObject *, PyObject *)`**

    * **Purpose:**
        Implements the membership test operation (the `in` operator).
    * **Usage in Python:**
        When evaluating `value in obj`, this function is used to determine if `value` is present in the sequence.
    * **C API Mapping:**
        This function populates the `sq_contains` slot in `PySequenceMethods`.
7. **`sq_inplace_concat` → `static PyObject* sequence_inplace_concat(PyObject *, PyObject *)`**

    * **Purpose:**
        Implements in-place concatenation of sequences.
    * **Usage in Python:**
        This is invoked when using the `+=` operator on sequences, modifying the sequence in place.
    * **C API Mapping:**
        It goes into the `sq_inplace_concat` slot of `PySequenceMethods`.
8. **`sq_inplace_repeat` → `static PyObject * sequence_inplace_repeat(PyObject *, Py_ssize_t)`**

    * **Purpose:**
        Implements in-place repetition of sequences.
    * **Usage in Python:**
        This function handles in-place multiplication (using `*=`) to repeat the sequence.
    * **C API Mapping:**
        It fills the `sq_inplace_repeat` slot in `PySequenceMethods`.

* * *

#### Mapping Protocol Callback

Although the code is primarily about the sequence protocol, it also provides functions for handling more general subscript operations via the mapping protocol:

1. **`mp_subscript` → `static PyObject * mapping_subscript(PyObject *, PyObject *)`**

    * **Purpose:**
        Provides generalized subscript access.
    * **Usage in Python:**
        This function is used when the object is accessed with a subscript that is not necessarily an integer (for example, a slice or another key) using the `obj[key]` syntax.
    * **C API Mapping:**
        It fills the `mp_subscript` slot in the `PyMappingMethods` structure.
2. **`mp_ass_subscript` → `static int mapping_ass_subscript(PyObject *, PyObject *, PyObject *)`**

    * **Purpose:**
        Implements assignment (or deletion) via subscripting through the mapping protocol.
    * **Usage in Python:**
        When performing operations like `obj[key] = value` or `del obj[key]`, this function is called.
    * **C API Mapping:**
        This function is assigned to the `mp_ass_subscript` slot in the `PyMappingMethods` structure.

* * *
