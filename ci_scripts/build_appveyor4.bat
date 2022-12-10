echo on
echo ------------------- APPVEYOR 4 -----------------------------------
if exist c:\cygwin64\setup-x86_64.exe (
  c:\cygwin64\setup-x86_64.exe --quiet-mode --packages cmake --packages cygwin32-gcc-g++ --packages cygwin32-gcc-core || exit /b !ERRORLEVEL!
)
