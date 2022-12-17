echo on
echo ------------------- APPVEYOR 20 -----------------------------------
cd %BUILD_DIR%
if "%GITREV%"=="" (
    set REVISION=%GITREV%
) else (
    set REVISION=r%SVNVERSION%
)
set ZIP_FILE=snapshot-%VERSION%-%REVISION%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-%REVISION%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-%REVISION%-appveyor-%COMPILER_FRIENDLY%
echo "%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
"%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD% || exit /b !ERRORLEVEL!
