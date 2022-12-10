echo on
echo ------------------- APPVEYOR 5 -----------------------------------
if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed make                     || exit /b !ERRORLEVEL!
)
