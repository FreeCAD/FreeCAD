
# install GCC
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install -qq g++-8
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 90
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 90
sudo update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-8 90
g++ --version

# install cmake
CMAKE_VERSION=3.12.1
CMAKE_VERSION_DIR=v3.12
CMAKE_OS=Linux-x86_64
CMAKE_TAR=cmake-$CMAKE_VERSION-$CMAKE_OS.tar.gz
CMAKE_URL=http://www.cmake.org/files/$CMAKE_VERSION_DIR/$CMAKE_TAR
CMAKE_DIR=$(pwd)/cmake-$CMAKE_VERSION
wget --quiet $CMAKE_URL
mkdir -p $CMAKE_DIR
tar --strip-components=1 -xzf $CMAKE_TAR -C $CMAKE_DIR
export PATH=$CMAKE_DIR/bin:$PATH
cmake --version