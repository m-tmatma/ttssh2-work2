echo on
echo ------------------- APPVEYOR 21 after------------------------------
cd %BUILD_DIR%
if "%GITREV%"=="" (
    set REVISION="%GITREV%"
) else (
    set REVISION="r%SVNVERSION%"
)
set ZIP_FILE=snapshot-%VERSION%-%REVISION%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-%REVISION%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-%REVISION%-appveyor-%COMPILER_FRIENDLY%

dir /b /s /a-d *.exe
