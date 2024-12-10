# Bazel support

To get [Bazel](https://bazel.build/) working with {fmt} you can copy the files `BUILD.bazel`, `WORKSPACE.bazel`, `.bazelrc`, and `.bazelversion` from this folder (`support/bazel`) to the root folder of this project. This way {fmt} gets bazelized and can be used with Bazel (e.g. doing a `bazel build //...` on {fmt}). 

## Using {fmt} as a dependency

The following minimal example shows how to use {fmt} as a dependency within a Bazel project.

The following file structure is assumed:

```
example
├── BUILD.bazel
├── main.cpp
└── WORKSPACE.bazel
```

*main.cpp*:

```c++
#include "fmt/core.h"

int main() {
  fmt::print("The answer is {}\n", 42);
}
```

The expected output of this example is `The answer is 42`.

*WORKSPACE.bazel*:

```python
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "fmt",
    branch = "master",
    remote = "https://github.com/fmtlib/fmt",
    patch_cmds = [
        "mv support/bazel/.bazelrc .bazelrc",
        "mv support/bazel/.bazelversion .bazelversion",
        "mv support/bazel/BUILD.bazel BUILD.bazel",
        "mv support/bazel/WORKSPACE.bazel WORKSPACE.bazel",
    ],
    # Windows-related patch commands are only needed in the case MSYS2 is not installed.
    # More details about the installation process of MSYS2 on Windows systems can be found here:
    # https://docs.bazel.build/versions/main/install-windows.html#installing-compilers-and-language-runtimes
    # Even if MSYS2 is installed the Windows related patch commands can still be used.
    patch_cmds_win = [
        "Move-Item -Path support/bazel/.bazelrc -Destination .bazelrc",
        "Move-Item -Path support/bazel/.bazelversion -Destination .bazelversion",
        "Move-Item -Path support/bazel/BUILD.bazel -Destination BUILD.bazel",
        "Move-Item -Path support/bazel/WORKSPACE.bazel -Destination WORKSPACE.bazel",
    ],
)
```

In the *WORKSPACE* file, the {fmt} GitHub repository is fetched. Using the attribute `patch_cmds` the  files `BUILD.bazel`, `WORKSPACE.bazel`, `.bazelrc`, and `.bazelversion` are moved to the root of the {fmt} repository. This way the {fmt} repository is recognized as a bazelized workspace. 

*BUILD.bazel*:

```python
cc_binary(
    name = "Demo",
    srcs = ["main.cpp"],
    deps = ["@fmt"],
)
```

The *BUILD* file defines a binary named `Demo` that has a dependency to {fmt}.

To execute the binary you can run `bazel run //:Demo`.

