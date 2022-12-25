echo on
echo ------------------- APPVEYOR 15 -----------------------------------
echo ------------------- APPVEYOR 16 -----------------------------------
echo ------------------- APPVEYOR 17 -----------------------------------
echo ------------------- APPVEYOR 18 -----------------------------------
echo ------------------- APPVEYOR 19 -----------------------------------
cd %BUILD_DIR%
if defined GITREV (
    set REVISION=%GITREV%
) else (
    set REVISION=r%SVNVERSION%
)
set ZIP_FILE=snapshot-%VERSION%-%REVISION%-githubactions-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-%REVISION%-githubactions-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-%REVISION%-githubactions-%COMPILER_FRIENDLY%
echo "%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
"%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE% || exit /b !ERRORLEVEL!
