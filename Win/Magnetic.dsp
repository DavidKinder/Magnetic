# Microsoft Developer Studio Project File - Name="Magnetic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Magnetic - Win32 DebugLog
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Magnetic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Magnetic.mak" CFG="Magnetic - Win32 DebugLog"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Magnetic - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Magnetic - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Magnetic - Win32 DebugLog" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W4 /Gm /GR /GX /Zi /Od /I "..\generic" /I "\Programs\Libraries\libpng" /I "\Programs\Libraries\zlib" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Magnetic"
# PROP BASE Intermediate_Dir "Magnetic"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W4 /GX /O2 /I "..\generic" /I "\Programs\Libraries\libpng" /I "\Programs\Libraries\zlib" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Magnetic"
# PROP BASE Intermediate_Dir "Magnetic"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugLog"
# PROP Intermediate_Dir "DebugLog"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "..\generic" /I "..\LibPNG" /I "..\ZLib" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "CUSTOM_INTERNAL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W4 /Gm /GR /GX /Zi /Od /I "..\generic" /I "\Programs\Libraries\libpng" /I "\Programs\Libraries\zlib" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "LOGGFX" /D "LOGGFX_EXT" /D LOG_FILE=\"c:\\temp\\mag.log\" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Magnetic - Win32 Debug"
# Name "Magnetic - Win32 Release"
# Name "Magnetic - Win32 DebugLog"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Amp11"

# PROP Default_Filter ""
# Begin Group "binfile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Amp11\binfile\binfarc.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfile.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfplnt.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfstd.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\Amp11\mp3dec.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Amp11\mpdecode.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Amp11\mpxdec.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile" /D "FASTBITS" /D "NOUNISTD" /D "__NT__"
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "LibPNG"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\png.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngerror.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngget.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngmem.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngpread.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngread.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngrio.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngrtran.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngrutil.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngset.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngtrans.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "ZLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\adler32.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\crc32.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\deflate.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\infblock.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\infcodes.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\inffast.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\inflate.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\inftrees.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\infutil.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\trees.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\uncompr.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\zutil.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BCMenu.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ColorBtn.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Magnetic.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MagneticDoc.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MagneticTitle.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile" /D "__NT__"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile" /D "__NT__"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile" /D "__NT__"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MagneticView.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PictureWnd.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ScrollBackDlg.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile" /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W4 /I ".\Amp11" /I ".\Amp11\binfile" /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W4 /I ".\Amp11" /I ".\Amp11\binfile" /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.cpp

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W1 /I ".\Amp11" /I ".\Amp11\binfile"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\Generic\emu.c

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

# ADD CPP /MD /W3
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

# ADD CPP /W3
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BCMenu.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfarc.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfile.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfplnt.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\binfstd.h
# End Source File
# Begin Source File

SOURCE=.\ColorBtn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\Generic\defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=.\Magnetic.h
# End Source File
# Begin Source File

SOURCE=.\MagneticDoc.h
# End Source File
# Begin Source File

SOURCE=.\MagneticTitle.h
# End Source File
# Begin Source File

SOURCE=.\MagneticView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\mp3dec.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\mpdecode.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\mpxdec.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\PictureWnd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\png.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\libpng\pngconf.h
# End Source File
# Begin Source File

SOURCE=.\Amp11\binfile\ptypes.h
# End Source File
# Begin Source File

SOURCE=.\ScrollBackDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\zlib\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Logo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Magnetic.ico
# End Source File
# Begin Source File

SOURCE=.\Magnetic.rc

!IF  "$(CFG)" == "Magnetic - Win32 Debug"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 Release"

!ELSEIF  "$(CFG)" == "Magnetic - Win32 DebugLog"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\Magnetic.rc2
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
# Section Magnetic : {359703C9-0000-0000-3339-2D313144302D}
# 	1:17:CG_IDS_DISK_SPACE:103
# 	1:19:CG_IDS_PHYSICAL_MEM:102
# 	1:25:CG_IDS_DISK_SPACE_UNAVAIL:104
# 	2:14:PhysicalMemory:CG_IDS_PHYSICAL_MEM
# 	2:9:DiskSpace:CG_IDS_DISK_SPACE
# 	2:16:SpaceUnavailable:CG_IDS_DISK_SPACE_UNAVAIL
# 	2:7:NewFunc:1
# 	2:10:SysInfoKey:1234
# End Section
# Section Magnetic : {A82AB342-BB35-11CF-8771-00A0C9039735}
# 	2:2:BH://
# End Section
# Section Magnetic : {00006700-0000-0000-0000-000000000000}
# 	1:17:ID_INDICATOR_DATE:108
# 	2:2:BH:
# 	2:17:ID_INDICATOR_DATE:ID_INDICATOR_DATE
# End Section
# Section Magnetic : {43495445-2D20-5720-494E-333220444542}
# 	1:26:CG_IDS_DISK_SPACE_UNAVAIL1:107
# 	1:18:CG_IDS_DISK_SPACE1:106
# 	1:20:CG_IDS_PHYSICAL_MEM1:105
# 	2:14:PhysicalMemory:CG_IDS_PHYSICAL_MEM1
# 	2:9:DiskSpace:CG_IDS_DISK_SPACE1
# 	2:16:SpaceUnavailable:CG_IDS_DISK_SPACE_UNAVAIL1
# 	2:10:SysInfoKey:1234
# End Section
