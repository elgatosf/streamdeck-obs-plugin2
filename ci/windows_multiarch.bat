@ECHO OFF

echo hi

REM Bootstrap
SET "ROOT=%~dp0"
PUSHD "%ROOT%" > NUL

SET "ARCH=%1"
SET "BUILD=%ROOT%..\build\windows_%ARCH%"
SET "DISTRIB=%ROOT%..\build\distrib"

:: Make Distrib Directory
if NOT EXIST "%DISTRIB%" (
	mkdir "%DISTRIB%"
)

:: Configure Qt6
cmake -H.. -B"%BUILD%" ^
  -G"Visual Studio 16 2019" -A"%2" ^
  -DCMAKE_INSTALL_PREFIX="%BUILD%_install" ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.2.2\msvc2019_64" ^
  -DDOWNLOAD_QT=OFF ^
  -DENABLE_CLANG=OFF ^
  -DWITH_QT6=ON ^
  -DBUILD_OLD=YES ^
  -DBUILD_LOADER=no ^
  -DPROJECT_SUFFIX=Qt6
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

:: Compile Qt6
cmake --build "%BUILD%" --config RelWithDebInfo --target INSTALL
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

:: Configure Qt5
cmake -H.. -B"%BUILD%" ^
  -G"Visual Studio 16 2019" -A"%2" ^
  -DCMAKE_INSTALL_PREFIX="%BUILD%_install" ^
  -DDOWNLOAD_QT=ON ^
  -DWITH_QT6=OFF ^
  -DBUILD_OLD=YES ^
  -DENABLE_CLANG=OFF ^
  -DBUILD_LOADER=no ^
  -DPROJECT_SUFFIX=Qt5
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

:: Compile Qt5
cmake --build "%BUILD%" --config RelWithDebInfo --target INSTALL
if %ERRORLEVEL% NEQ 0 (
	PAUSE
) 

:: Configure Loader
cmake -H.. -B"%BUILD%" ^
  -G"Visual Studio 16 2019" -A"%2" ^
  -DCMAKE_INSTALL_PREFIX="%BUILD%_install" ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.2.2\msvc2019_64" ^
  -DDOWNLOAD_QT=OFF ^
  -DWITH_QT6=OFF ^
  -DBUILD_OLD=YES ^
  -DBUILD_LOADER=yes ^
  -DPROJECT_SUFFIX=
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

:: Compile Loader
cmake --build "%BUILD%" --config RelWithDebInfo --target INSTALL
if %ERRORLEVEL% NEQ 0 (
	PAUSE
)

POPD > NUL

:: Build Installer
PUSHD "%ROOT%../installer/Windows" > NUL

set "OBSPluginLocation=%BUILD%/RelWithDebInfo"
set BUILD_SETUP_CONFIGURATION=Setup
echo BUILD=%BUILD%
echo OBSPluginLocation=%OBSPluginLocation%
call Setup_Bamboo.bat

POPD > NUL
