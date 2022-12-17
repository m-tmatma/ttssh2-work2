echo on
echo ------------------- APPVEYOR 22 -----------------------------------
cd %BUILD_DIR%
if "%GITREV%"=="" (
    set REVISION=%GITREV%
) else (
    set REVISION=r%SVNVERSION%
)
set ZIP_FILE=snapshot-%VERSION%-%REVISION%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-%REVISION%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-%REVISION%-appveyor-%COMPILER_FRIENDLY%

mkdir %SNAPSHOT_DIR%\cygterm+-i686
mkdir %SNAPSHOT_DIR%\cygterm+-x86_64
copy /Y "Release\cygterm.exe"                                                  "%SNAPSHOT_DIR%\cygterm+-i686\cygterm.exe"
copy /Y "cygwin\cygterm_build\cygterm_build\build_cygterm_x86_64\cygterm.exe"  "%SNAPSHOT_DIR%\cygterm+-x86_64\cygterm.exe"

echo "%CMAKE_COMMAND%" --build . --target inno_setup
"%CMAKE_COMMAND%" --build . --target inno_setup                   || exit /b !ERRORLEVEL!
