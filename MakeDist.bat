@echo off

"%ProgramFiles(x86)%\Zip\zip" -j \Temp\MagneticWin.zip COPYING Win\Release\*.exe Win\Release\*.chm

"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip COPYING MakeDist.bat Generic\* Scripts\* Amiga\*
"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Dos\*.c Dos\makefile Dos\bcc.cfg Dos\build.txt
"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Dos32\allegro.c Dos32\makefile
"%ProgramFiles(x86)%\Zip\zip" -r \Temp\MagneticSrc.zip Win\* -x Win\Release\*
pushd \Programs
"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Libraries\mfc\ColourButton.*
"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Libraries\mfc\DarkMode.*
"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Libraries\mfc\Dialogs.*
"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Libraries\mfc\MenuBar.*
popd

"%ProgramFiles(x86)%\Zip\zip" \Temp\MagneticSrc.zip Glk\* Gtk\*

