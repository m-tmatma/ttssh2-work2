@echo off

SET plugins=no
SET rebuild=
SET release=

if "%1"=="/?" goto help
@echo on
if "%1"=="plugins" SET plugins=yes
if "%1"=="rebuild" SET rebuild=rebuild
if "%1"=="release" SET release=yes

CALL makechm.bat
CALL build.bat %rebuild%
if ERRORLEVEL 1 goto fail
set release_bak=%release%
CALL ..\buildtools\svnrev\sourcetree_info.bat
set release=%release_bak%

rem  change folder name
if not "%release%"=="yes" goto snapshot
set ver=
for /f "delims=" %%i in ('perl issversion.pl') do @set ver=%%i
set dst=Output\teraterm-%ver%
goto create

:snapshot
set dst=snapshot

:create
del /s /q %dst%\*.*
mkdir %dst%

copy /y ..\teraterm\release\*.exe %dst%
copy /y ..\teraterm\release\*.dll %dst%
copy /y ..\ttssh2\ttxssh\Release\ttxssh.dll %dst%
copy /y ..\cygwin\cygterm\cygterm.cfg %dst%
copy /y "..\cygwin\cygterm\cygterm+.tar.gz" %dst%
if not exist ..\cygwin\cygterm\cygterm+-i686\cygterm.exe goto cygwin32_pass
mkdir "%dst%\cygterm+-i686"
copy /y "..\cygwin\cygterm\cygterm+-i686\cygterm.exe" "%dst%\cygterm+-i686"
:cygwin32_pass
mkdir "%dst%\cygterm+-x86_64"
copy /y "..\cygwin\cygterm\cygterm+-x86_64\cygterm.exe" "%dst%\cygterm+-x86_64"
copy /y "..\cygwin\cygterm\cygterm+-x86_64\cygterm.exe" %dst%
if not exist ..\cygwin\cygterm\msys2term\msys2term.exe goto msys2term_pass
copy /y ..\cygwin\cygterm\msys2term\msys2term.exe %dst%
copy /y ..\cygwin\cygterm\msys2term.cfg %dst%
:msys2term_pass
copy /y ..\cygwin\Release\cyglaunch.exe %dst%
copy /y ..\ttpmenu\Release\ttpmenu.exe %dst%
copy /y ..\TTProxy\Release\TTXProxy.dll %dst%
copy /y ..\TTXKanjiMenu\Release\ttxkanjimenu.dll %dst%
if "%plugins%"=="yes" copy /y ..\TTXSamples\Release\*.dll %dst%
if "%release%"=="yes" copy /y ..\TTXSamples\Release\*.dll %dst%

rem pdb files
del /s /q %dst%_pdb\*.*
mkdir %dst%_pdb

copy /y ..\teraterm\release\*.pdb %dst%_pdb
copy /y ..\ttssh2\ttxssh\Release\ttxssh.pdb %dst%_pdb
copy /y ..\ttpmenu\Release\ttxssh.pdb %dst%_pdb
copy /y ..\TTProxy\Release\TTXProxy.pdb %dst%_pdb
copy /y ..\TTXKanjiMenu\Release\ttxkanjimenu.pdb %dst%_pdb
if "%plugins%"=="yes" copy /y ..\TTXSamples\Release\*.pdb %dst%_pdb

if "%plugins%"=="yes" (
pushd %dst%
if exist TTXFixedWinSize.dll ren TTXFixedWinSize.dll _TTXFixedWinSize.dll
if exist TTXResizeWin.dll ren TTXResizeWin.dll _TTXResizeWin.dll
popd
)
if "%release%"=="yes" (
pushd %dst%
if exist TTXOutputBuffering.dll ren TTXOutputBuffering.dll _TTXOutputBuffering.dll
if exist TTXFixedWinSize.dll ren TTXFixedWinSize.dll _TTXFixedWinSize.dll
if exist TTXResizeWin.dll ren TTXResizeWin.dll _TTXResizeWin.dll
if exist TTXShowCommandLine.dll ren TTXShowCommandLine.dll _TTXShowCommandLine.dll
if exist TTXtest.dll ren TTXtest.dll _TTXtest.dll
popd
)

copy /y ..\doc\ja\teratermj.chm %dst%
copy /y ..\doc\en\teraterm.chm %dst%

copy /y release\*.* %dst%
copy /y release\EDITOR.CNF %dst%\KEYBOARD.CNF
xcopy /s /e /y /i /exclude:archive-exclude.txt release\theme %dst%\theme
xcopy /s /e /y /i /exclude:archive-exclude.txt release\plugin %dst%\plugin
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang %dst%\lang
xcopy /s /e /y /i /exclude:archive-exclude.txt release\lang_utf16le %dst%\lang_utf16le
del /f %dst%\lang\English.lng

perl setini.pl release\TERATERM.INI > %dst%\TERATERM.INI

if "%release%"=="yes" (
copy nul %dst%\ttpmenu.ini
copy nul %dst%\portable.ini
)

exit /b

:help
echo Tera Term���r���h���邽�߂ɕK�v�ȃ\�[�X�R�[�h�����ׂăR���p�C�����܂��B(Compiling ALL source codes)
echo.
echo   %0          �ʏ�̃r���h(Normal building)
echo   %0 rebuild  ���r���h(Re-building)
echo   %0 plugins  �v���O�C�����܂ރr���h(Building with all plugins)
echo   %0 release  �ʏ�̃r���h + �v���O�C�����܂� + �t�H���_��������(Normal + Plugins building + unique folder naming)
echo      �A�[�J�C�u�Ń����[�X�쐬�p(for archive version released)
echo   %0 rebuild ^>build.log 2^>^&1  �r���h���O���̎悷��(Retrieve building log)
echo.
exit /b

:fail
@echo off
echo ===================================================
echo ================= E R R O R =======================
echo ===================================================
echo.
echo �r���h�Ɏ��s���܂��� (Failed to build source code)
exit /b

