echo on
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
