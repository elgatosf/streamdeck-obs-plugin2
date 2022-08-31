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

echo "Qt6/OBS 28"

../obs-scripts/scripts/build-macos.zsh --target macos-universal

echo "Qt5"

# A hack to get around dylib issues
pushd /tmp
curl -L "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/qt-5.15.2-macos-x86_64-2021-03-25.tar.gz" -o /tmp/obsdeps.tar.gz
tar -xzf obsdeps.tar.gz
popd

# Configure Qt5
cmake -H.. -B"${_build}" \
  -DCMAKE_OSX_ARCHITECTURES="$1" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
  -DCMAKE_INSTALL_PREFIX=../build/install \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DWITH_QT6=OFF \
  -DBUILD_OLD=YES \
  -DDOWNLOAD_QT=ON \
  -DDOWNLOAD_QT_URL="https://github.com/Xaymar/obs-studio/releases/download/27.0.0/qt-5.15.2-macos-x86_64-2021-03-25.tar.gz" \
  -DDOWNLOAD_QT_HASH="SHA256=FFABB54624B931EA3FCC06BED244895F50CEFC95DE09D792D280C46D4F91D4C5" \
  -DDOWNLOAD_OBS_URL="https://github.com/Xaymar/obs-studio/releases/download/27.0.0-ci/obs-studio-x64-0.0.0.0-macos-x86-64.7z" \
  -DDOWNLOAD_OBS_HASH="SHA256=F15BC4CA8EB3F581A94372759CFE554E30D202B604B541445A5756B878E4E799" \
  -DDOWNLOAD_OBSDEPS_URL="https://github.com/Xaymar/obs-studio/releases/download/27.0.0/deps-macos-x86_64-2021-03-25.tar.gz" \
  -DDOWNLOAD_OBSDEPS_HASH="SHA256=1C409374BCAB9D5CEEAFC121AA327E13AB222096718AF62F2648302DF62898D6" \
  -DENABLE_CLANG=OFF \
  -DBUILD_LOADER=no \
  -DPROJECT_SUFFIX=Qt5
if [[ $? -ne 0 ]]; then
	exit 1
fi


# Compile Qt5
cmake --build "${_build}" --config RelWithDebInfo --target install
if [[ $? -ne 0 ]]; then
	exit 1
fi


echo "Loader"

# Configure Loader
cmake -H.. -B"${_build}" \
  -DCMAKE_OSX_ARCHITECTURES="x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
  -DCMAKE_INSTALL_PREFIX=../build/install \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DWITH_QT6=OFF \
  -DDOWNLOAD_QT=OFF \
  -DBUILD_OLD=YES \
  -DENABLE_CLANG=OFF \
  -DBUILD_LOADER=yes \
  -DPROJECT_SUFFIX=
if [[ $? -ne 0 ]]; then
	exit 1
fi

# Compile Loader
cmake --build "${_build}" --config RelWithDebInfo --target install
if [[ $? -ne 0 ]]; then
	exit 1
fi

popd > /dev/null
