rem LibreSSL�̃r���h

cd libressl


cmake -G "Visual Studio 16 2019" -A Win32
perl -e "open(IN,'CMakeCache.txt');while(<IN>){s|=/MD|=/MT|;print $_;}close(IN);" > CMakeCache.txt.tmp
move /y CMakeCache.txt.tmp CMakeCache.txt
rem �ύX���� CMakeCache.txt �Ɋ�Â��Đ��������悤�ɍēx���s
cmake -G "Visual Studio 16 2019" -A Win32

devenv /build Debug LibreSSL.sln /project crypto /projectconfig Debug

devenv /build Release LibreSSL.sln /project crypto /projectconfig Release


cd ..
exit /b 0
