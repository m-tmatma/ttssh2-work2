echo on
echo ------------------- APPVEYOR 11 -----------------------------------
cd %BUILD_DIR%
if exist build_config.cmake del build_config.cmake
if exist cmakecache.txt del cmakecache.txt
