{ lib
, stdenv
, cmake
, doxygen
}:

stdenv.mkDerivation (finalAttrs: {
  name = "libkdtree++";
  version = "0.7.3";

  src = builtins.path { name = "libkdtree++"; path = ./.; };

  nativeBuildInputs = [
    cmake
    doxygen
  ];

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

  meta = with lib; {
    description = "STL-like C++ template container implementatin of a kd-tree.";
    longDescription = ''
       STL-like C++ template container implementation of k-dimensional space 
       sorting, using a kd-tree.
       It sports a theoretically unlimited number of dimensions, and can store 
       any data structure.
       Fork of the project once available from http://libkdtree.alioth.debian.org/
    '';
    homepage = "https://github.com/nvmd/libkdtree/";
    license = licenses.artistic2;
    platforms = platforms.all;
  };
})
