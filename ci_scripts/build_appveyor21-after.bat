echo on
echo ------------------- APPVEYOR 21 after------------------------------
cd %BUILD_DIR%
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-appveyor-%COMPILER_FRIENDLY%
dir /b /s /a-d *.exe
