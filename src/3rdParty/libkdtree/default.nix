{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  name = "libkdtree++";
  # version = "0.7.2";

  src = builtins.path { name = "libkdtree++"; path = ./.; };

  nativeBuildInputs = with pkgs; [
    cmake
    doxygen
  ];

  configurePhase = ''
    mkdir -p build && cd build
    cmake ..
  '';

  buildPhase = ''
    make
    doxygen ../doc/Doxyfile
  '';

  doCheck = true;
  checkPhase = ''
    echo Test test_kdtree
    ./examples/test_kdtree
    echo Test test_hayne
    ./examples/test_hayne
    echo Test test_find_within_range
    ./examples/test_find_within_range
  '';

  installPhase = ''
    mkdir -p $out/include/kdtree++
    cp -r $src/kdtree++/*.hpp $out/include/kdtree++/

    mkdir -p $out/share/doc/libkdtree++
    cp $src/doc/index.txt $out/share/doc/libkdtree++
    cp -r documentation $out/share/doc/libkdtree++/

    mkdir -p $out/share/doc/libkdtree++/examples
    cp -r $src/examples/*.cpp $out/share/doc/libkdtree++/examples

  '';
}
