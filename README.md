# Magnetic

Magnetic is an interpreter for the games written between 1985 and 1991 by Magnetic Scrolls, a text adventure producer based in London, England. Although they only produced seven games they have acquired legendary status for text adventures of as good quality as Infocom accompanied by exceptional graphics.

The Magnetic interpreter was written by Niclas Karlsson, with additional code by myself, Stefan Meier and Paul David Doherty.

Extracting the game data from the original releases can be a complex process; for more information see the Magnetic page at Stefan's [Magnetic Scrolls Memorial](https://msmemorial.if-legends.org/memorial.php).

![Magnetic playing The Pawn](The%20Pawn.png)

## Building

Download and install Visual Studio 2019 Community edition from https://visualstudio.microsoft.com/. In the installer, under "Workloads", make sure that "Desktop development with C++" is selected, and under "Individual components" that "C++ MFC for latest build tools (x86 & x64)" is selected.

To be able to build the CHM help file, download and install Microsoft's [HTML Help Workshop](https://web.archive.org/web/20200810052030/https://www.microsoft.com/en-us/download/confirmation.aspx?id=21138). Note that this is a link to the version on the Internet Archive as the link to this on Microsoft's site does not work at the moment.

Install git. I use the version of git that is part of MSYS2, a Linux-like environment for Windows, but Git for Windows can be used from a Windows command prompt.

Open the environment that you are using git from, and switch to the root directory that the build environment will be created under (from here referred to as "\<root>"). Clone this and the other required repositories of mine with git:
```
git clone https://github.com/DavidKinder/Magnetic.git Adv/Magnetic
git clone https://github.com/DavidKinder/Libraries.git Libraries
```

### Third-party libraries

#### libpng

Download the latest version of zlib from https://zlib.net/. Unpack the archive and copy the contents of the top-level directory to "\<root>/Libraries/zlib".

Download the latest version of libpng from http://www.libpng.org/pub/png/libpng.html. Unpack the archive and copy the contents of the top-level directory to "\<root>/Libraries/libpng". Copy the file "\<root>/Libraries/libpng/scripts/pnglibconf.h.prebuilt" to "\<root>/Libraries/libpng/pnglibconf.h".

Open "\<root>/Libraries/libpng/pnglibconf.h" in a text editor, and find and delete all lines that define symbols starting with "PNG_SAVE_", "PNG_SIMPLIFIED_WRITE_" and "PNG_WRITE_".

### Compiling the project

Start Visual Studio, open the solution "\<root>/Adv/Magnetic/Win/Magnetic.sln", then build and run the "Magnetic" project.
