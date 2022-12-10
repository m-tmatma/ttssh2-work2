echo on
echo ------------------- APPVEYOR 6 -----------------------------------
if "%GENERATOR%" == "Visual Studio 8 2005" (
  cd buildtools
  call getcmake.bat nopause || exit /b !ERRORLEVEL!
  cd ..
)
