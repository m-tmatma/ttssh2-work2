echo on
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
  rem c:\cygwin64\setup-x86_64.exe --quiet-mode --packages cygwin32-gcc-g++ --packages cygwin32-gcc-core
)
echo ------------------- APPVEYOR 5 -----------------------------------
pacman.exe  -S --noconfirm --needed cmake
echo ------------------- APPVEYOR 6 -----------------------------------
if "%GENERATOR%" == "Visual Studio 8 2005" (
  cd buildtools
  call getcmake.bat nopause
  cd ..
)
echo ------------------- APPVEYOR 7 -----------------------------------
if "%COMPILER%" == "mingw"  (
  set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-i686-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
echo ------------------- APPVEYOR 8 -----------------------------------
if "%COMPILER%" == "mingw_x64"  (
  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc make
  pacman -S --noconfirm --needed mingw-w64-i686-cmake mingw-w64-i686-gcc make
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw-w64-x86_64-clang
    pacman -S --noconfirm --needed mingw-w64-i686-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
echo ------------------- APPVEYOR 9 -----------------------------------
cd libs
echo ------------------- APPVEYOR 10 -----------------------------------
if not exist openssl11_%COMPILER% (
  "%CMAKE_COMMAND%" -DCMAKE_GENERATOR="%GENERATOR%" %CMAKE_OPTION_LIBS% -P buildall.cmake
  if exist build rmdir /s /q build
  if exist download rmdir /s /q download
  if exist openssl_%COMPILER%\html rmdir /s /q openssl_%COMPILER%\html
  if exist openssl_%COMPILER%_debug\html rmdir /s /q openssl_%COMPILER%_debug\html
  if exist ..\buildtools\perl\c rmdir /s /q ..\buildtools\perl\c
  if exist ..\buildtools\download rmdir /s /q ..\buildtools\download
)
echo ------------------- APPVEYOR 11 -----------------------------------
cd ..
echo ------------------- APPVEYOR 12 -----------------------------------
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
echo ------------------- APPVEYOR 13 -----------------------------------
cd %BUILD_DIR%
echo ------------------- APPVEYOR 14 -----------------------------------
if exist build_config.cmake del build_config.cmake
echo ------------------- APPVEYOR 15 -----------------------------------
if exist cmakecache.txt del cmakecache.txt
echo ------------------- APPVEYOR 16 -----------------------------------
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
echo ------------------- APPVEYOR 17 -----------------------------------
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
echo ------------------- APPVEYOR 18 -----------------------------------
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
echo ------------------- APPVEYOR 19 -----------------------------------
echo "%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
"%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
echo ------------------- APPVEYOR 20 -----------------------------------
echo "%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
"%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
echo ------------------- APPVEYOR 21 -----------------------------------
echo "%CMAKE_COMMAND%" --build . --target zip
"%CMAKE_COMMAND%" --build . --target zip
echo ------------------- APPVEYOR 22 -----------------------------------
echo "%CMAKE_COMMAND%" --build . --target inno_setup
"%CMAKE_COMMAND%" --build . --target inno_setup
echo ------------------- APPVEYOR 23 -----------------------------------
cd ..
