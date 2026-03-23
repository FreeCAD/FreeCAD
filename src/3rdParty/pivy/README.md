General build instructions:
---------------------------
[![Build Status](https://travis-ci.org/Coin3D/pivy.svg?branch=master)](https://travis-ci.org/Coin3D/pivy)

Starting with version 0.6.6 pivy it's possible to build pivy with cmake:

```bash
  $ cd pivy
  $ mkdir build
  $ cd build
  $ cmake .. --toolchain $VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE:STRING=Release -DPython_ROOT_DIR:PATH="path/to/python/root/dir" -DX_VCPKG_APPLOCAL_DEPS_INSTALL:BOOLEAN=ON
  $ cmake --build .
  $ cmake --install .
```

Alternative it's still possible to use [distutils][0]:

```bash
  $ python3 setup.py build
  $ python3 setup.py install
```

For older system/distros please use the `setup_old.py` script to build pivy. 

Reporting bugs:
--------------

Please submit bug reports, feature requests, etc. to the [Pivy
issue tracker][1].

Contact:
--------

If you have any questions regarding Pivy or simply want to discuss
with other Pivy users, you can do so at the general [coin-discuss
mailinglist][2].


[0]: http://www.python.org/sigs/distutils-sig/
[1]: https://github.com/Coin3D/pivy/issues
[2]: http://groups.google.com/group/coin3d-discuss
