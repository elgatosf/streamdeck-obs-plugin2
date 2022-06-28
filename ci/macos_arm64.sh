# !/bin/bash

# Bootstrap
_root=$0:A
pushd "${root}"> /dev/null

./macos_multiarch.sh arm64

popd > /dev/null
