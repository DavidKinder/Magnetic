@echo off
setlocal
if "%1"=="wins" goto :bldwins
if "%1"=="armi" goto :bldarmi
echo Usage: BLD target
echo where target is wins or armi
goto :eof

:bldwins
call abld build wins udeb
copy /y C:\Symbian\6.0\NokiaCPP\Epoc32\Release\wins\udeb\HUGO.dll C:\Symbian\6.0\NokiaCPP\Epoc32\Wins\c
goto :eof

:bldarmi
call abld build armi urel
rem copy C:\Symbian\6.0\NokiaCPP\Epoc32\Release\armi\urel\magnetic.exe
zip -j magnetic_nokia.zip C:\Symbian\6.0\NokiaCPP\Epoc32\Release\armi\urel\magnetic.exe magnetic_epoc.txt magnetic.txt
goto :eof

