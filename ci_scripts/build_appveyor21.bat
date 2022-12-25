echo on
echo ------------------- APPVEYOR 21 -----------------------------------
cd %BUILD_DIR%
if defined GITREV (
    set REVISION=%GITREV%
) else (
    set REVISION=r%SVNVERSION%
)
set ZIP_FILE=snapshot-%VERSION%-%REVISION%-githubactions-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-%REVISION%-githubactions-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-%REVISION%-githubactions-%COMPILER_FRIENDLY%

echo "%CMAKE_COMMAND%" --build . --target zip
"%CMAKE_COMMAND%" --build . --target zip                          || exit /b !ERRORLEVEL!
