# !/bin/bash

# Bootstrap
_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pushd "${root}"> /dev/null

./linux_multiarch.sh x86-64
if [[ $? -ne 0 ]]; then
	exit 1
fi

popd > /dev/null
