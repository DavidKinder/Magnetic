Magnetic Scrolls Interpreter, Glk Notes
---------------------------------------


Introduction
------------

This is a "port" of the Magnetic Scrolls 2.1 interpreter to Glk.  The
complete interface lives in the single file

	glk.c

The main test and development system for the port is Linux, with Xglk as
the Glk library.


Acknowledgements
----------------

Thanks to David Kinder for hints and tips on how to go about the port, and
of course to Niclas Karlsson and his collaborators for the underlying
Magnetic Scrolls interpreter, which looks like it probably wasn't a lot of
fun to work on at times.

Thanks also to Ben Hines <bhines@alumni.ucsd.edu> for the Mac code.  I've
copied in from AGiliTy with, I think, the appropriate modifications, but
it's untested at present.


Running Games
-------------

The interpreter needs to be handed a game in "Magnetic Scrolls" data format.
Typically, this will be a game with the extension ".MAG" or ".mag" (and
"MaSc" in the first four data bytes of the file).

Give the name of the game to run at the system prompt.  For example

	glkmagnetic GUILD.MAG

If there is an associated graphics file, usually a file that has the
extension ".GFX" or ".gfx", make sure that it is in the same directory as
the main data file, and has the same base name.  For example, the matching
graphics file for "GUILD.MAG" is "GUILD.GFX".  The Glk interpreter will
find this file, if it exists, and use it to create game pictures
automatically.

There may also be an associated hints file, with the extension ".HNT" or
".hnt".  Like the graphics file, make sure it is in the same directory as
the other game files, and with the same base name, and the Glk interpreter
will find the file automatically.  Hints files apply only to the later
Magnetic Windows games, so not all the games you find will have them.

If there is no graphics file, the interpreter will run in text only mode.
It will also do this if it cannot open the graphics file, or if the file
does not seem to contain game graphics.

If there is no hints file, the interpreter will not be able to display
game hints, but will otherwise run normally.

As a short cut, you can omit the game extension, so for example

	glkmagnetic GUILD

will run GUILD.MAG, and also use pictures from GUILD.GFX, and hints from
GUILD.HNT, if either or both are available.

Because of the way the Glk interpreter looks for files, you should make
sure that the games are held in files with ".MAG" or ".mag" extensions, and
any associated graphics and hints are held in files with ".GFX" or ".gfx"
and ".HNT" or ".hnt" extensions respectively.  Games held in files with
other extensions may not work.

There are a few command line options that you can add to vary the way that
the game looks to a small extent:

	-np	Don't display game pictures initially
	-ng	Don't automatically gamma correct game picture colours
	-na	Don't expand single-letter abbreviations
	-nc	Don't attempt to interpret selected commands locally

See below for further information about what these options mean.


Compiling
---------

To compile Glk Magnetic Scrolls for Linux, first unpack the source files.
You might need to use the -a argument to unzip in order to convert text
files for your system.

Edit Makefile.glk so that it has the right path to the Glk library you wish
to build with.  If you want to build the IFP plugin, also edit the parts of
Makefile.glk that have paths to IFP components.

To build standalone binary version of Glk Magnetic Scrolls, change directory
to the Glk subdirectory, and use

	make -f Makefile.glk glkmagnetic

To build the IFP plugin, use

	make -f Makefile.glk magnetic-2.1.so

To clean up and delete everything in the case of a build error, use

	make -f Makefile.glk clean


Displaying Pictures
-------------------

Magnetic Scrolls games generally come with attractive graphics, and while
not essential to the game, they do enhance the experience of playing.

Unfortunately, Glk does not provide a method to directly display an image
bitmap, so the Glk port needs to adopt some tricks to get a picture to
display.  The end result is that, on Linux Xglk at least, it can take
several seconds to render a complete picture.

To reduce the problems caused by a game pausing for several seconds for a
picture, the Glk port of Magnetic Scrolls does its picture rendering using
a background "thread".  This draws in the complete picture over a series of
smaller image updates, while at the same time still allowing game play.
To speed up picture rendering, the Glk port also goes to considerable
effort to try to minimize the amount of Glk graphics operations it uses.

If you move to a location with a new picture before the current one has
been fully rendered, the Glk port will simply start working on the newer
picture.

Most Magnetic Windows and Magnetic Scrolls games accept the "graphics"
command, which toggles pictures on and off.  Magnetic Scrolls games usually
start with graphics enabled, but for some reason Magnetic Windows games
tend to start with graphics disabled.  To get pictures, type "graphics"
as more or less the first game command.

Some of the pictures in later Magnetic Windows games are animated.  The
Glk port does not attempt to do animations; it displays only the static
portions of such pictures.

You can use the "-np" option to turn off pictures.  The Glk port also will
not show pictures if the Glk library does not support both graphics and
timers, if there is no suitable ".GFX" or ".gfx" graphics file, or if the
interpreter cannot open the graphics file successfully.  See below for
further notes about displaying pictures.


Gamma Colour Corrections
------------------------

Some of the Magnetic Scrolls pictures look fine without gamma corrections.
On the other hand, others, in particular those in "Corruption", and several
from "Wonderland", can look very dark if not corrected.

To try to adjust for this, by default the Glk port of Magnetic Scrolls
tries to find and use a gamma correction that will make picture colour
contrasts look relatively constant for a particular colour palette.  As
well a brightening dark pictures, this gamma correction will also try to
richen colours a little for particulary bright pictures.

You can use the "-ng" option to turn off gamma colour corrections.  In
this case, all pictures are displayed using their original colour palette.


Game Hints
----------

Where a game comes with hints (Wonderland, and the "collection" games),
the Glk port of Magnetic Scrolls will try to display them if possible.

Hints are shown in a pair of windows which overlay the main game text
window.  The upper hint window gives a menu of hint topics.  The lower
one shows the game hints one at a time.

The Glk port will not show hints if there is no suitable ".HNT" or ".hnt"
hints file, or if the interpreter cannot open the hints file successfully.


Expanding Abbreviations
-----------------------

Many IF games systems allow a player to use single character abbreviations
for selected common commands, for example, 'x' for 'examine', 'l' for look,
and so on.  Not all Magnetic Scrolls games offer this feature, however.

To try to help the player, the Glk port of Magnetic Scrolls will
automatically expand a selection of single character commands, before
passing the expanded string to the game as input.  It expands a command
only if the first word of the command is a single letter, and one of the
following:

        { 'c',  "close" },      { 'g',  "again" },      { 'i',  "inventory" },
        { 'k',  "attack" },     { 'l',  "look" },       { 'p',  "open" },
        { 'q',  "quit" },       { 'r',  "drop" },       { 't',  "take" },
        { 'x',  "examine" },    { 'y',  "yes" },        { 'z',  "wait" },

If you want to suppress abbreviation expansion, you can prefix your input
with a single quote character (like putting literal strings into a spread-
sheet).  If you do this, the Glk interface will strip the quote, then pass
the rest of the string to the main interpreter without any more changes.
So for example,

	'x something

will pass the string "x something" back to the game, whereas

	x something

will pass "examine something" back to the game.

You can turn off abbreviation expansions with the command line option '-na'.


Interpreting Commands Locally
-----------------------------

The Glk port will handle special commands if they are prefixed with the
string 'glk'.  It understands the following special commands:

	undo		  Undoes the prior game move
	script on	  Starts recording the game text output sent to the
			  main game window
	script off	  Turns off game text recording
	inputlog on	  Starts recording input lines typed by the player
	inputlog off	  Stops recording input lines
	readlog on	  Reads an input log file as if it had been typed by
			  a player; reading stops automatically at the end of
			  the file
	abbreviations on  Turn abbreviation expansion on
	abbreviations off Turn abbreviation expansion off
	graphics on	  Turn on Glk graphics
	graphics off	  Turn off Glk graphics (see below)
	gamma on	  Turn automatic gamma colour correction on
	gamma off	  Turn automatic gamma colour correction off
	version		  Prints the Glk library and Glk port version numbers
	commands off	  Turn of Glk special commands; once off, there is no
			  way to turn them back on

If for some reason you need to pass the string "glk" to the interpreter,
you can, as with abbreviations above, prefix it with a single quote character.

You can turn off local command handling with the command line option '-nc'.

If both abbreviation expansion and local command handling are turned off,
there is no need to use single quotes to suppress special interpreter
features.

When turning graphics on and off with Glk commands, you should pay special
attention to whether graphics are turned on or off in the game.  In order
for graphics to be displayed, they must be turned on both in the game
*and* in Glk.  Turning off graphics in Glk is a little more permanent than
turning them off in a game.  In some games, turning graphics off seems to
mean do not display new pictures, but leave the current picture on the
display; turning off graphics in Glk closes the graphics display window
completely.

What can also be confusing is the tendency of Magnetic Windows games to
display a picture automatically only on your first visit to a game
location; on subsequent visits, a "look" command is needed to redisplay
the picture.  This appears not to happen with Magnetic Scrolls games.

Why an additional Glk way to turn off pictures?  Well, the Magnetic
Windows and Magnetic Scrolls games don't always behave the same way with
the "graphics" command, so a control inside the Glk port itself gives a
more consistent way to turn graphics on and off.  The command

    glk graphics

with no on/off argument, prints out a brief information about the current
state of a game's graphics.  In practice, it's probably best to stick to
just one method of turning graphics on and off -- either the game's own
way, or the 'glk graphics' command.

