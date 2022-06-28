# !/bin/zsh

# Bootstrap
_root=$0:A
pushd "${root}"> /dev/null

# Add CMake.app to the PATH
PATH=$PATH:/Applications/CMake.app/Contents/bin

_arch=$1
_build="../build/macos_${_arch}"
_distrib="${_root}/../build/distrib"

# Make Distrib Directory
if [ ! -d "${_distrib}" ]; then
	mkdir "${_distrib}"
fi

# Configure
cmake -H.. -B"${_build}" \
  -DCMAKE_OSX_ARCHITECTURES="$1" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
  -DCMAKE_INSTALL_PREFIX=../build/install \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_CLANG=OFF
if [[ $? -ne 0 ]]; then
	exit 1
fi

# Compile
cmake --build "${_build}" --config RelWithDebInfo --target install
if [[ $? -ne 0 ]]; then
	exit 1
fi

popd > /dev/null
