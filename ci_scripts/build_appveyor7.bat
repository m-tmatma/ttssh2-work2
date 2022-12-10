echo on
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
