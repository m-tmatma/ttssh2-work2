echo on
echo ------------------- APPVEYOR 22 -----------------------------------
cd %BUILD_DIR%
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
echo "%CMAKE_COMMAND%" --build . --target inno_setup
"%CMAKE_COMMAND%" --build . --target inno_setup                   || exit /b !ERRORLEVEL!

copy /Y Release\cygterm.exe                                                  %~dp0\..\cygwin\cygterm\cygterm+-i686\cygterm.exe
copy /Y cygwin\cygterm_build\cygterm_build\build_cygterm_x86_64\cygterm.exe  %~dp0\..\cygwin\cygterm\cygterm+-x86_64\cygterm.exe
