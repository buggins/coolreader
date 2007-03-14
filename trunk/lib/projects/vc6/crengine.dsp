# Microsoft Developer Studio Project File - Name="crengine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=crengine - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "crengine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "crengine.mak" CFG="crengine - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "crengine - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "crengine - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "crengine - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "crengine - Win32 Unicode Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "crengine - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\crengine.lib"

!ELSEIF  "$(CFG)" == "crengine - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\crengined.lib"

!ELSEIF  "$(CFG)" == "crengine - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "crengine___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "crengine___Win32_Unicode_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "crengine___Win32_Unicode_Debug"
# PROP Intermediate_Dir "crengine___Win32_Unicode_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\crengined.lib"
# ADD LIB32 /nologo /out:"..\..\crengined.lib"

!ELSEIF  "$(CFG)" == "crengine - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "crengine___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "crengine___Win32_Unicode_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "crengine___Win32_Unicode_Release"
# PROP Intermediate_Dir "crengine___Win32_Unicode_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\crengine.lib"
# ADD LIB32 /nologo /out:"..\..\crengine.lib"

!ENDIF 

# Begin Target

# Name "crengine - Win32 Release"
# Name "crengine - Win32 Debug"
# Name "crengine - Win32 Unicode Debug"
# Name "crengine - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\crtxtenc.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=..\..\..\src\hyphman.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lstridmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvbmpbuf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvdocview.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvdrawbuf.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvfnt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvfntman.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvimg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvmemman.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvpagesplitter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvrend.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvstream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvstring.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvstsheet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvstyles.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvtextfm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvtinydom.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\lvxml.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\w32utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\wolutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\xutils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\include\crengine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\crsetup.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\crtxtenc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cssdef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\dtddef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fb2def.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\hyphman.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lstridmap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvarray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvbmpbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvdocview.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvdrawbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvfnt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvfntman.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvhashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvimg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvmemman.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvpagesplitter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvptrvec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvref.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvrefcache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvrend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvstream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvstring.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvstsheet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvstyles.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvtextfm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvtinydom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvtypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lvxml.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\w32utils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\wolutil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\xutils.h
# End Source File
# End Group
# End Target
# End Project
