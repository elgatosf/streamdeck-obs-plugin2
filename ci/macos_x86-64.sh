# !/bin/bash

# Bootstrap
_root=$0:A
pushd "${root}"> /dev/null

./macos_multiarch.sh x86_64
if [[ $? -ne 0 ]]; then
	exit 1
fi

popd > /dev/null
