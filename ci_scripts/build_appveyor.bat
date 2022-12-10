echo on
echo ------------------- APPVEYOR 0 -----------------------------------
if exist teraterm\ttpdlg\svnversion.h del teraterm\ttpdlg\svnversion.h
if exist buildtools\svnrev\sourcetree_info.bat del buildtools\svnrev\sourcetree_info.bat
echo ------------------- APPVEYOR 1 -----------------------------------
call ci_scripts\install.bat
echo ------------------- APPVEYOR 2 -----------------------------------
call buildtools\svnrev\svnrev.bat
echo ------------------- APPVEYOR 3 -----------------------------------
call buildtools\svnrev\sourcetree_info.bat
echo ------------------- APPVEYOR 4 -----------------------------------
if exist c:\cygwin64\setup-x86_64.exe (
  c:\cygwin64\setup-x86_64.exe --quiet-mode --packages cmake
  rem c:\cygwin64\setup-x86_64.exe --quiet-mode --packages cygwin32-gcc-g++ --packages cygwin32-gcc-core || exit /b !ERRORLEVEL!
)
echo ------------------- APPVEYOR 5 -----------------------------------
if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed cmake                     || exit /b !ERRORLEVEL!
)
echo ------------------- APPVEYOR 6 -----------------------------------
if "%GENERATOR%" == "Visual Studio 8 2005" (
  cd buildtools
  call getcmake.bat nopause || exit /b !ERRORLEVEL!
  cd ..
)
echo ------------------- APPVEYOR 7 -----------------------------------
if "%COMPILER%" == "mingw"  (
  set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make     || exit /b !ERRORLEVEL!
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-i686-clang                           || exit /b !ERRORLEVEL!
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release    || exit /b !ERRORLEVEL!
)
echo ------------------- APPVEYOR 8 -----------------------------------
if "%COMPILER%" == "mingw_x64"  (
  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc make || exit /b !ERRORLEVEL!
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make     || exit /b !ERRORLEVEL!
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-x86_64-clang || exit /b !ERRORLEVEL!
    pacman -S --noconfirm --needed mingw-w64-i686-clang || exit /b !ERRORLEVEL!
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
echo ------------------- APPVEYOR 9 -----------------------------------
echo ------------------- APPVEYOR 10 -----------------------------------
cd libs
if not exist openssl11_%COMPILER% (
  "%CMAKE_COMMAND%" -DCMAKE_GENERATOR="%GENERATOR%" %CMAKE_OPTION_LIBS% -P buildall.cmake || exit /b !ERRORLEVEL!
  if exist build rmdir /s /q build
  if exist download rmdir /s /q download
  if exist openssl_%COMPILER%\html rmdir /s /q openssl_%COMPILER%\html
  if exist openssl_%COMPILER%_debug\html rmdir /s /q openssl_%COMPILER%_debug\html
  if exist ..\buildtools\perl\c rmdir /s /q ..\buildtools\perl\c
  if exist ..\buildtools\download rmdir /s /q ..\buildtools\download
)
cd ..
echo ------------------- APPVEYOR 11 -----------------------------------
echo ------------------- APPVEYOR 12 -----------------------------------
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
echo ------------------- APPVEYOR 13 -----------------------------------
echo ------------------- APPVEYOR 14 -----------------------------------
cd %BUILD_DIR%
if exist build_config.cmake del build_config.cmake
if exist cmakecache.txt del cmakecache.txt
echo ------------------- APPVEYOR 15 -----------------------------------
echo ------------------- APPVEYOR 16 -----------------------------------
echo ------------------- APPVEYOR 17 -----------------------------------
echo ------------------- APPVEYOR 18 -----------------------------------
echo ------------------- APPVEYOR 19 -----------------------------------
cd %BUILD_DIR%
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
echo "%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
"%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE% || exit /b !ERRORLEVEL!
echo ------------------- APPVEYOR 20 -----------------------------------
echo "%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
"%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD% || exit /b !ERRORLEVEL!
echo ------------------- APPVEYOR 21 -----------------------------------
echo "%CMAKE_COMMAND%" --build . --target zip
"%CMAKE_COMMAND%" --build . --target zip                          || exit /b !ERRORLEVEL!

echo ------------------- APPVEYOR 21 after------------------------------
dir /b /s /a-d *.exe
echo ------------------- APPVEYOR 22 -----------------------------------
echo "%CMAKE_COMMAND%" --build . --target inno_setup
"%CMAKE_COMMAND%" --build . --target inno_setup                   || exit /b !ERRORLEVEL!
echo ------------------- APPVEYOR 23 -----------------------------------
cd ..
