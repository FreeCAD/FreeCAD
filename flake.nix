{
  description = "FreeCAD development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems =
        f:
        nixpkgs.lib.genAttrs systems (
          system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
    in
    {
      devShells = forAllSystems (
        { pkgs }:
        let
          python = pkgs.python312;
          pythonEnv = python.withPackages (
            ps: with ps; [
              pyside6
              shiboken6
              pybind11
              lark
              ply
              pivy
              matplotlib
            ]
          );
        in
        {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              ccache
              swig
              qt6.wrapQtAppsHook
              doxygen
            ];

            buildInputs = with pkgs; [
              pythonEnv
              python.pkgs.pyside6
              python.pkgs.shiboken6
              python.pkgs.pybind11

              # Qt 6
              qt6.qtbase
              qt6.qtsvg
              qt6.qttools

              # CAD kernel & 3D
              opencascade-occt
              coin3d

              # C++ libraries
              boost
              eigen
              vtk
              icu
              xercesc
              fmt
              yaml-cpp
              hdf5
              medfile
              libGLU
            ];

            dontWrapQtApps = true;

            shellHook = ''
              _pyside6_dir=$(python3 -c "import PySide6; import os; print(os.path.dirname(PySide6.__file__))" 2>/dev/null || true)
              _shiboken6_dir=$(python3 -c "import shiboken6; import os; print(os.path.dirname(shiboken6.__file__))" 2>/dev/null || true)
              [ -n "$_pyside6_dir" ] && export CMAKE_PREFIX_PATH="$_pyside6_dir:$CMAKE_PREFIX_PATH"
              [ -n "$_shiboken6_dir" ] && export CMAKE_PREFIX_PATH="$_shiboken6_dir:$CMAKE_PREFIX_PATH"

              # Ensure Nix libraries are found before system libraries
              # (system /usr/lib may contain older Qt, minimal BLAS, etc.)
              export LD_LIBRARY_PATH="${pkgs.qt6.qtbase}/lib:${pkgs.qt6.qtsvg}/lib:${pkgs.qt6.qttools}/lib:${pkgs.openblas}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

              # Host OpenGL drivers (needed for non-NixOS systems)
              # Appended at the END so Nix libraries take priority
              for dir in /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib; do
                if [ -e "$dir/libGL.so.1" ]; then
                  export LD_LIBRARY_PATH="''${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}$dir"
                  break
                fi
              done

              echo "FreeCAD development environment loaded"
              echo ""
              echo "Build:"
              echo "  cmake -B build -G Ninja -DFREECAD_QT_VERSION=6"
              echo "  cmake --build <build filesystem path>"
              echo ""
              echo "Run:"
              echo "  ./run.sh"
            '';
          };
        }
      );
    };
}
