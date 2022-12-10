echo on
echo ------------------- APPVEYOR 21 -----------------------------------
cd %BUILD_DIR%
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
echo "%CMAKE_COMMAND%" --build . --target zip
"%CMAKE_COMMAND%" --build . --target zip                          || exit /b !ERRORLEVEL!
