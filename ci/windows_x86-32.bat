@ECHO OFF

REM Bootstrap
SET "ROOT=%~dp0"
PUSHD "%ROOT%" > NUL

windows_multiarch x86-32 Win32

POPD > NUL
