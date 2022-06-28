# !/bin/bash

# Bootstrap
_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pushd "${root}"> /dev/null

_arch=$1
_build="../build/linux_${_arch}"
_distrib="${root}/../build/distrib"

# Make Distrib Directory
if [ ! -d "${_distrib}" ]; then
	mkdir "${_distrib}"
fi

# Configure
cmake -H.. -B"${_build}" \
  -G"Ninja" \
  -DCMAKE_INSTALL_PREFIX="${_build}_install" \
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
