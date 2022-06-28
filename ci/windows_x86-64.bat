@ECHO OFF

REM Bootstrap
SET "ROOT=%~dp0"
PUSHD "%ROOT%" > NUL

windows_multiarch x86-64 x64

POPD > NUL
