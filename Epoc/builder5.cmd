@echo off
setlocal
if "%1"=="wins" goto :bldwins
if "%1"=="WINS" goto :bldwins
if "%1"=="marm" goto :bldmarm
if "%1"=="MARM" goto :bldmarm
echo Usage: BLD target
echo where target is wins or marm
goto :eof

:bldwins
set USERDEFS=__MAGNETIC__
nmake -f magnetic.wins
copy /y \EPOC32\RELEASE\WINS\DEB\MAGNETIC.dll \epoc32\wins\c\
goto :eof

:bldmarm
set USERDEFS=__MAGNETIC__
nmake -f magneticer5.marm clean
nmake -f magneticer5.marm
@echo on
zip -j magnetic_epoc.zip \epoc32\release\marm\rel\magnetic.exe magnetic_epoc.txt
rem copy \epoc32\release\marm\rel\magnetic.exe f:\
@echo off
goto :eof
