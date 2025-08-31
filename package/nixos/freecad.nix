{
  lib,
  stdenv,

  # nativeBuildInputs
  cmake,
  ninja,
  pkg-config,
  swig,
  doxygen,

  # buildInputs
  fmt,
  gts,
  zlib,
  eigen,
  xercesc,
  yaml-cpp,
  coin3d,
  libGLU,
  libXmu,
  libspnav,
  ode,
  hdf5,
  medfile,
  opencascade-occt,
  microsoft-gsl,
  qt6,
  python3Packages,

  # extra command-line utilities in PATH
  which,
  gmsh,
  libredwg,
}:
let
  pythonDeps =
    ps: with ps; [
      boost
      gitpython # for addon manager
      ifcopenshell
      matplotlib
      opencamlib
      pivy
      ply # for openSCAD file support
      py-slvs
      pycollada
      pyside6
      pyyaml # (at least for) PyrateWorkbench
      scipy
      shiboken6
      vtk
      netgen-mesher
    ];
  python-env = python3Packages.python.withPackages pythonDeps;
in
stdenv.mkDerivation (finalAttrs: {
  pname = "freecad";
  version = "1.1.0-git";

  src = lib.fileset.toSource rec {
    root = ../..;
    fileset = lib.fileset.unions (
      map (path: root + path) [
        /cMake
        /src
        /data
        /tests
        /tools
        /CMakeLists.txt
        /PRIVACY_POLICY.md
      ]
    );
  };

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    swig
    doxygen
    qt6.wrapQtAppsHook
  ];

  buildInputs = [
    fmt
    gts
    zlib
    eigen
    xercesc
    yaml-cpp
    coin3d
    libGLU
    libXmu
    libspnav
    ode
    hdf5
    medfile
    opencascade-occt
    microsoft-gsl
    qt6.qtbase
    qt6.qtsvg
    qt6.qttools
    qt6.qtwayland
    qt6.qtwebengine
    python3Packages.python
    python3Packages.pybind11
  ]
  ++ pythonDeps python3Packages;

  cmakeFlags = [
    (lib.cmakeBool "BUILD_DRAWING" true)
    (lib.cmakeBool "INSTALL_TO_SITEPACKAGES" false)

    # can be removed once sumodule OndselSolver support absolute cmake path
    (lib.cmakeFeature "CMAKE_INSTALL_BINDIR" "bin")
    (lib.cmakeFeature "CMAKE_INSTALL_LIBDIR" "lib")
    (lib.cmakeFeature "CMAKE_INSTALL_INCLUDEDIR" "include")
  ];

  qtWrapperArgs =
    let
      binPath = lib.makeBinPath [
        which
        gmsh
        libredwg
      ];
    in
    [
      "--prefix PATH : ${binPath}"
      "--add-flags"
      finalAttrs.freecadFlags
    ];

  # export freecadFlags to devShell
  # run `./bin/FreeCAD $freecadFlags` to test build result
  freecadFlags = "--python-path=${python-env}/${python3Packages.python.sitePackages}";

  postFixup = ''
    ln -s $out/bin/FreeCAD $out/bin/freecad
    ln -s $out/bin/FreeCADCmd $out/bin/freecadcmd
  '';

  meta = {
    homepage = "https://www.freecad.org";
    description = "General purpose Open Source 3D CAD/MCAD/CAx/CAE/PLM modeler";
    longDescription = ''
      FreeCAD is an open-source parametric 3D modeler made primarily to design
      real-life objects of any size. Parametric modeling allows you to easily
      modify your design by going back into your model history and changing its
      parameters.

      FreeCAD allows you to sketch geometry constrained 2D shapes and use them
      as a base to build other objects. It contains many components to adjust
      dimensions or extract design details from 3D models to create high quality
      production ready drawings.

      FreeCAD is designed to fit a wide range of uses including product design,
      mechanical engineering and architecture. Whether you are a hobbyist, a
      programmer, an experienced CAD user, a student or a teacher, you will feel
      right at home with FreeCAD.
    '';
    license = lib.licenses.lgpl2Plus;
    maintainers = with lib.maintainers; [
      srounce
      grimmauld
    ];
    platforms = lib.platforms.linux;
  };
})
