mkdir -p build
cd build

cmake -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_INSTALL_PREFIX=$PREFIX \
      -D CMAKE_PREFIX_PATH=$PREFIX \
      -D CMAKE_LIBRARY_PATH=$PREFIX/lib \
      -D BUILD_QT5=ON \
      -D NETGENDATA=$PREFIX/include/netgen \
      -D NETGEN_INCLUDEDIR=$PREFIX/include/netgen \
      -D NGLIB_INCLUDE_DIR=$PREFIX/include/nglib \
      -D OCC_INCLUDE_DIR=$PREFIX/include/opencascade \
      -D OCC_LIBRARY_DIR=$PREFIX/lib \
      -D OCC_LIBRARIES=$PREFIX/lib CACHE PATH \
      -D FREECAD_USE_OCC_VARIANT="Official Version" \
      -D OCC_OCAF_LIBRARIES=$PREFIX/lib CACHE PATH \
      -D SWIG_DIR=$PREFIX/share/swig/3.0.8 \
      -D SWIG_EXECUTABLE=$PREFIX/bin/swig \
      -D PYTHON_EXECUTABLE=$PYTHON \
      -D BUILD_FEM_NETGEN=YES \
      -D USE_BOOST_PYTHON=NO \
      -D FREECAD_USE_PYBIND11=YES \
      -D BUILD_ENABLE_CXX11=ON \
      -D SMESH_INCLUDE_DIR=$PREFIX/include/smesh \
      -D FREECAD_USE_EXTERNAL_SMESH=ON \
      /source

make -j${CPU_COUNT} 2>&1 | tee output.txt
make -j${CPU_COUNT} install

rm ${PREFIX}/doc -r     # smaller size of package!