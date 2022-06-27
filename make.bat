@ECHO OFF
if "%~1"=="" goto BLANK
if "%~1"=="22" goto 22
if "%~1"=="install" goto install
if "%~1"=="clean" goto CLEAN
@ECHO ON

:BLANK
cmake -H. -B_project -G  "Visual Studio 16 2019" -A "x64"
GOTO DONE

:22
cmake -H. -B_project -G  "Visual Studio 17 2022" -A "x64"
GOTO DONE

:INSTALL
cmake -H. -B_project -G  "Visual Studio 16 2019" -A "x64" -DCMAKE_INSTALL_PREFIX=%2 
cmake --build _project --config Release --target INSTALL
GOTO DONE

:CLEAN
rmdir /Q /S _project
rmdir /Q /S _bin
GOTO DONE

:DONE