@ECHO OFF

REM Bootstrap
SET "ROOT=%~dp0"
PUSHD "%ROOT%" > NUL

SET "ARCH=%1"
SET "BUILD=../build/windows_%ARCH%"
SET "DISTRIB=%ROOT%/../build/distrib"

:: Make Distrib Directory
if NOT EXIST "%DISTRIB%" (
	mkdir "%DISTRIB%"
)

:: Configure
cmake -H.. -B"%BUILD%" ^
  -G"Visual Studio 16 2019" -A"%2" ^
  -DCMAKE_INSTALL_PREFIX="%BUILD%_install" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DDOWNLOAD_QT=ON ^
  -DENABLE_CLANG=OFF
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

:: Compile
cmake --build "%BUILD%" --config RelWithDebInfo --target INSTALL
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

POPD > NUL
