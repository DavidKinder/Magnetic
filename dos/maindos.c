/*
 * DOS interface for Magnetic Scrolls interpreter by Stefan Jokisch
 *
 *      written with Borland C++ 3.1
 *
 * A horrible mess, but it seems to work.
 *
 */

#include <alloc.h>
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"

#define byte0(v)        ((byte *)&v)[0]
#define byte1(v)        ((byte *)&v)[1]
#define byte2(v)        ((byte *)&v)[2]
#define byte3(v)        ((byte *)&v)[3]
#define word0(v)        ((word *)&v)[0]
#define word1(v)        ((word *)&v)[1]

#define FILE_RESTORE 0
#define FILE_SAVE 1
#define FILE_SCRIPT 2
#define FILE_PLAYBACK 3
#define FILE_RECORD 4

#ifndef MAX_FILE_NAME
#define MAX_FILE_NAME 80
#endif
#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE 200
#endif
#ifndef TEXT_BUFFER_SIZE
#define TEXT_BUFFER_SIZE 100
#endif
#ifndef TEXT_BUFFER2_SIZE
#define TEXT_BUFFER2_SIZE 200
#endif
#ifndef HISTORY_BUFFER_SIZE
#define HISTORY_BUFFER_SIZE 500
#endif
#ifndef HISTORY_MIN_ENTRY
#define HISTORY_MIN_ENTRY 1
#endif
#ifndef MAX_INPUT_LINES
#define MAX_INPUT_LINES 10
#endif

#define ZC_BACKSPACE 0x08
#define ZC_RETURN 0x0d
#define ZC_HKEY_MIN 0x0e
#define ZC_HKEY_RECORD 0x0e
#define ZC_HKEY_PLAYBACK 0x0f
#define ZC_HKEY_SEED 0x10
#define ZC_HKEY_HELP 0x11
#define ZC_HKEY_QUIT 0x12
#define ZC_HKEY_SCRIPT 0x13
#define ZC_HKEY_UNDO 0x14
#define ZC_HKEY_MAX 0x14
#define ZC_ESCAPE 0x1b
#define ZC_ASCII_MIN 0x20
#define ZC_ASCII_MAX 0x7e
#define ZC_BAD 0x7f
#define ZC_ARROW_MIN 0x81
#define ZC_ARROW_UP 0x81
#define ZC_ARROW_DOWN 0x82
#define ZC_ARROW_LEFT 0x83
#define ZC_ARROW_RIGHT 0x84
#define ZC_ARROW_MAX 0x84

#define SPECIAL_KEY_MIN 256
#define SPECIAL_KEY_HOME 256
#define SPECIAL_KEY_END 257
#define SPECIAL_KEY_WORD_LEFT 258
#define SPECIAL_KEY_WORD_RIGHT 259
#define SPECIAL_KEY_DELETE 260
#define SPECIAL_KEY_INSERT 261
#define SPECIAL_KEY_F1 262
#define SPECIAL_KEY_MAX 262

#define _MONO_ 0
#define _TEXT_ 1
#define _AMIGA_ 5
#define _VGA_ 6

typedef int bool;

#define TRUE 1
#define FALSE 0

#ifndef DEFAULT_SAVE_NAME
#define DEFAULT_SAVE_NAME "story.sav"
#endif
#ifndef DEFAULT_SCRIPT_NAME
#define DEFAULT_SCRIPT_NAME "story.scr"
#endif
#ifndef DEFAULT_COMMAND_NAME
#define DEFAULT_COMMAND_NAME "story.rec"
#endif
#ifndef DEFAULT_AUXILARY_NAME
#define DEFAULT_AUXILARY_NAME "story.aux"
#endif
#ifndef DEFAULT_FONT_NAME
#define DEFAULT_FONT_NAME "default.fnt"
#endif

typedef unsigned char byte;
typedef unsigned short word;

extern const char *optarg;
extern int optind;

int cdecl getopt (int, char *[], const char *);

extern unsigned _stklen = 0x800;

static display = -1;

static byte old_video_mode = 255;
static byte new_video_mode = 255;

static char *prog_name = NULL;

static char story_name[MAX_FILE_NAME + 1];
static char gfx_name[MAX_FILE_NAME + 1];
static char stripped_story_name[MAX_FILE_NAME + 1];

static char script_name[MAX_FILE_NAME + 1] = DEFAULT_SCRIPT_NAME;
static char command_name[MAX_FILE_NAME + 1] = DEFAULT_COMMAND_NAME;
static char save_name[MAX_FILE_NAME + 1] = DEFAULT_SAVE_NAME;
static char font_name[MAX_FILE_NAME + 1] = DEFAULT_FONT_NAME;

static user_screen_width = -1;
static user_screen_height = -1;
static user_script_cols = 80;
static user_context_lines = 0;
static user_left_margin = 0;
static user_right_margin = 0;
static user_background = -1;
static user_foreground = -1;
static user_reverse_bg = -1;
static user_reverse_fg = -1;

static word *serif_font = NULL;
static byte *serif_width = NULL;

static void interrupt far (*oldvect) (void) = NULL;

static screen_height = 0;
static screen_width = 0;
static font_height = 0;
static font_width = 0;

static bool fixed_font = FALSE;

static max_char_width = 0;

static cursor_x = 0;
static cursor_y = 0;

static char buffer[TEXT_BUFFER_SIZE];
static char buffer2[TEXT_BUFFER2_SIZE];
static bufpos = 0;
static bufpos2 = 0;

static total_width = 0;

static scaler = 2;

static byte scrn_attr = 0x10;

static bool ostream_screen = TRUE;
static bool ostream_script = FALSE;
static bool ostream_record = FALSE;
static bool istream_replay = FALSE;

static script_width = 0;

static FILE *sfp = NULL;
static FILE *pfp = NULL;
static FILE *rfp = NULL;

static line_count = 0;

static pic_number = -1;
static pic_lines = 0;

static byte text_bg = 0;
static byte text_fg = 1;

static bool more_prompts = TRUE;

static struct {
	 char buffer[HISTORY_BUFFER_SIZE];
	 int latest;
	 int current;
	 int prefix_len;
} history;

static struct {
	 char *buffer;
	 int pos;
	 int length;
	 int max_length;
	 int width;
	 int line;
	 int peak;
	 int right_end[MAX_INPUT_LINES];
} input;

static bool overwrite = FALSE;

#define INFORMATION "\
MAGNETIC v1.0 - an interpreter for Magnetic Scrolls games. Written by Niclas\n\
Karlsson in 1997. DOS version by Stefan Jokisch\n\
\n\
Syntax: magnetic [options] story-file\n\
\n\
\t-b #\tbackground colour [text modes only]\n\
\t-B #\tstatus line background [text modes only]\n\
\t-c #\tcontext lines\n\
\t-d #\tdisplay mode (see below, default is VGA 400 lines)\n\
\t-f #\tforeground colour [text modes only]\n\
\t-F #\tstatus line foreground [text modes only]\n\
\t-g font\ttext font\n\
\t-h #\tscreen height\n\
\t-l #\tleft margin\n\
\t-r #\tright margin\n\
\t-S #\ttransscript width\n\
\t-w #\tscreen width\n\
\n\
Display modes are 0 (mono), 1 (text), 5 (VGA 400 lines), 6 (VGA 480 lines)."

static void cleanup (void)
{

	if (old_video_mode != 255) {
		asm mov ah,0
		asm mov al,old_video_mode
		asm int 0x10
	}

	if (oldvect != NULL)
		setvect (0x1b, oldvect);

}/* cleanup */

static void interrupt far fast_exit ()
{

	cleanup (); exit (0);

}/* fast_exit */

extern void status (void);

void internal (const char *s)
{

	cleanup ();

	fputs ("\nFatal error: ", stderr);
	fputs (s, stderr);
	fputs ("\n", stderr);

	status ();

	exit (EXIT_FAILURE);

}/* internal */

static void parse_options (int argc, char **argv)
{
	int c;

	do {

		int num = 0;

		c = getopt (argc, argv, "b:B:c:d:f:F:g:h:l:r:S:w:");

		if (optarg != NULL)
			num = atoi (optarg);

		if (c == 'b')
			user_background = num;
		if (c == 'B')
			user_reverse_bg = num;
		if (c == 'c')
			user_context_lines = num;
		if (c == 'd')
			display = optarg[0] | 32;
		if (c == 'f')
			user_foreground = num;
		if (c == 'F')
			user_reverse_fg = num;
		if (c == 'l')
			user_left_margin = num;
		if (c == 'g')
			strcpy (font_name, optarg);
		if (c == 'h')
			user_screen_height = num;
		if (c == 'r')
			user_right_margin = num;
		if (c == 'S')
			user_script_cols = num;
		if (c == 'w')
			user_screen_width = num;

	} while (c != EOF);

}/* parse_options */

static void os_process_arguments (int argc, char *argv[])
{
	const char *p;
	int i;

	parse_options (argc, argv);

	if (optind != argc - 1) {
		puts (INFORMATION);
		exit (EXIT_FAILURE);
	}

	strcpy (story_name, argv[optind]);

	if (!strchr (font_name, '.'))
		strcat (font_name, ".fnt");
	if (!strchr (story_name, '.'))
		strcat (story_name, ".mag");

	p = story_name;

	for (i = 0; story_name[i] != 0; i++)
		if (story_name[i] == '\\' || story_name[i] == ':')
			p = story_name + i + 1;

	for (i = 0; p[i] != 0 && p[i] != '.' && i < 8; i++)
		stripped_story_name[i] = p[i];

	stripped_story_name[i] = 0;

	strcpy (script_name, stripped_story_name);
	strcpy (command_name, stripped_story_name);
	strcpy (save_name, stripped_story_name);
	strcpy (gfx_name, stripped_story_name);

	strcat (script_name, ".scr");
	strcat (command_name, ".rec");
	strcat (save_name, ".sav");
	strcat (gfx_name, ".gfx");

	prog_name = argv[0];

}/* os_process_arguments */

static void load_font (void)
{
	char fname[81];
	FILE *fp;
	int i, j;

	for (i = 0, j = 0; prog_name[i] != 0; i++)
		if (prog_name[i] == '\\' || prog_name[i] == ':')
			j = i + 1;

	strcpy (fname, prog_name);
	strcpy (fname + j, font_name);

	if ((fp = fopen (font_name, "rb")) == NULL && (fp = fopen (fname, "rb")) == NULL)
		internal ("Cannot load font data");

	if (!(serif_font = farmalloc (3168)) || !fread (serif_font, 3168, 1, fp))
		if (serif_font != NULL)
			farfree (serif_font);

	serif_width = (byte far *) (serif_font + 1536);

	fclose (fp);

	return;

}/* load_font */

static void special_palette (void)
{

	static byte palette[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x00 /* last one is the overscan register */
	};

	asm mov ax,0x1002
	asm mov dx,offset palette
	asm push ds
	asm pop es
	asm int 0x10

}/* special_palette */

static int os_char_width (char c)
{

	if (display <= _TEXT_)
		return 1;

	return (fixed_font) ? 8 : serif_width[c - 32];

}/* os_char_width */

static int os_string_width (const char *s)
{
	int width = 0;

	while (*s != 0)
		width += os_char_width (*s++);

	return width;

}/* os_string_width */

static void os_init_screen (void)
{
	static struct { /* information on modes 0 to 6 */
		byte vmode;
		word width;
		word height;
		byte font_width;
		byte font_height;
	} info[] = {
		{ 0x07,  80,  25,  1,  1 }, /* MONO  */
		{ 0x03,  80,  25,  1,  1 }, /* TEXT  */
		{ 0xff,   0,   0,  0,  0 },
		{ 0xff,   0,   0,  0,  0 },
		{ 0xff,   0,   0,  0,  0 },
		{ 0x12, 640, 400,  8, 16 }, /* AMIGA */
		{ 0x12, 640, 480,  8, 16 }
	};

	static struct { /* information on modes A to E */
		word vesamode;
		word width;
		word height;
	} subinfo[] = {
		{ 0x001,  40, 25 },
		{ 0x109, 132, 25 },
		{ 0x10b, 132, 50 },
		{ 0x108,  80, 60 },
		{ 0x10c, 132, 60 }
	};

	int subdisplay;

	asm mov ah,15
	asm int 0x10
	asm mov old_video_mode,al

	if (display == -1)

		if (old_video_mode == 7)
			display = '0';
		else
			display = '5';

	if (display >= '0' && display <= '6') {
		subdisplay = -1;
		display -= '0';
		_AL = info[display].vmode;
		_AH = 0;
	} else if (display == 'a') {
		subdisplay = 0;
		display = 1;
		_AL = 0x01;
		_AH = 0;
	} else if (display >= 'b' && display <= 'e') {
		subdisplay = display - 'a';
		display = 1;
		_BX = subinfo[subdisplay].vesamode;
		_AX = 0x4f02;
	}

	geninterrupt (0x10);

	if (display <= _TEXT_) {

		asm mov ax,0x1003
		asm mov bl,0
		asm int 0x10
		asm mov ah,1
		asm mov cx,0xffff
		asm int 0x10

		if (user_foreground == -1)
			user_foreground = LIGHTGRAY;
		if (user_background == -1)
			user_background = (display == _MONO_) ? BLACK : BLUE;
		if (user_reverse_fg == -1)
			user_reverse_fg = user_background;
		if (user_reverse_bg == -1)
			user_reverse_bg = user_foreground;

		scrn_attr = (user_background << 4) | user_foreground;

		text_bg = user_background;
		text_fg = user_foreground;

	} else {

		load_font ();

		if (display == _AMIGA_) {

			 outportb (0x03c2, 0x63);

			 outport (0x03d4, 0x0e11);
			 outport (0x03d4, 0xbf06);
			 outport (0x03d4, 0x1f07);
			 outport (0x03d4, 0x9c10);
			 outport (0x03d4, 0x8f12);
			 outport (0x03d4, 0x9615);
			 outport (0x03d4, 0xb916);

		}

		special_palette ();

		asm mov ax,0x1010
		asm mov bx,0
		asm mov dh,0
		asm mov ch,0
		asm mov cl,0
		asm int 0x10
		asm mov ax,0x1010
		asm mov bx,1
		asm mov dh,63
		asm mov ch,63
		asm mov cl,63
		asm int 0x10

	}

	screen_width = info[display].width;
	screen_height = info[display].height;
	font_width = info[display].font_width;
	font_height = info[display].font_height;

	if (subdisplay != -1) {
		screen_width = subinfo[subdisplay].width;
		screen_height = subinfo[subdisplay].height;
	}

	if (user_screen_width != -1)
		screen_width = user_screen_width;
	if (user_screen_height != -1)
		screen_height = user_screen_height;

	cursor_x = 0;
	cursor_y = font_height;

	max_char_width = os_char_width ('W');

	oldvect = getvect (0x1b);
	setvect (0x1b, fast_exit);

}/* os_init_screen */

static void swap_colours (void)
{

	_AL = text_fg;
	_AH = text_bg;
	text_fg = _AH;
	text_bg = _AL;

}/* swap_colours */

static byte far *get_scrnptr (int y)
{

	return MK_FP (0xa000, 80 * y);

}/* get_scrnptr */

static void clear_byte (byte far *scrn, word mask)
{

	outport (0x03ce, 0x0205);

	outportb (0x03ce, 0x08);
	outportb (0x03cf, mask);

	asm les bx,scrn
	asm mov al,es:[bx]
	asm mov al,0
	asm mov es:[bx],al

}/* clear_byte */

static void clear_line (int y, int left, int right)
{
	byte far *scrn = get_scrnptr (y);

	word mask1 = 0x00ff >> (left & 7);
	word mask2 = 0xff80 >> (right & 7);

	int x = right / 8 - left / 8;

	scrn += left / 8;

	if (x == 0) {
		mask1 &= mask2;
		mask2 = 0;
	}

	/* Clear first byte */

	clear_byte (scrn++, mask1);

	/* Clear middle bytes */

	outport (0x03ce, 0xff08);

	while (--x > 0)
		*scrn++ = 0;

	/* Clear last byte */

	clear_byte (scrn, mask2);

}/* clear_line */

static void os_erase_area (int top, int left, int bottom, int right)
{
	int y;

	if (display <= _TEXT_) {

		asm mov ax,0x0600
		asm mov ch,byte ptr top
		asm mov cl,byte ptr left
		asm mov dh,byte ptr bottom
		asm mov dl,byte ptr right
		asm mov bh,scrn_attr
		asm int 0x10

	 } else

		for (y = top; y <= bottom; y++)
			clear_line (y, left, right);

}/* os_erase_area */

static void copy_byte (byte far *scrn1, byte far *scrn2, byte mask)
{
	int i;

	outport (0x03ce, 0x0005);

	outportb (0x03ce, 0x08);
	outportb (0x03cf, mask);

	outportb (0x03ce, 0x04);
	outportb (0x03c4, 0x02);

	for (i = 0; i < 4; i++) {

		outportb (0x03cf, i);
		outportb (0x03c5, 1 << i);

		asm les bx,scrn2
		asm mov ah,es:[bx]
		asm les bx,scrn1
		asm mov al,es:[bx]
		asm mov es:[bx],ah

	}

	outportb (0x03c5, 0x0f);

}/* copy_byte */

static void copy_line (int y1, int y2, int left, int right)
{
	byte far *scrn1 = get_scrnptr (y1);
	byte far *scrn2 = get_scrnptr (y2);

	word mask1 = 0x00ff >> (left & 7);
	word mask2 = 0xff80 >> (right & 7);

	int x = right / 8 - left / 8;

	scrn1 += left / 8;
	scrn2 += left / 8;

	if (x == 0) {
		mask1 &= mask2;
		mask2 = 0;
	}

	/* Copy first byte */

	copy_byte (scrn1++, scrn2++, mask1);

	/* Copy middle bytes */

	outport (0x03ce, 0x0105);

	while (--x > 0)
		*scrn1++ = *scrn2++;

	/* Copy last byte */

	copy_byte (scrn1, scrn2, mask2);

}/* copy_line */

static void scroll_text (void)
{
	int y;

	int top = font_height * (pic_lines + 1);
	int bottom = screen_height - 1;
	int left = user_left_margin;
	int right = screen_width - user_right_margin - 1;

	if (display <= _TEXT_) {

		asm mov ah,6
		asm mov al,1
		asm mov ch,byte ptr top
		asm mov cl,byte ptr left
		asm mov dh,byte ptr bottom
		asm mov dl,byte ptr right
		asm mov bh,scrn_attr
		asm int 0x10

	} else

		for (y = top; y <= bottom; y++)

			if (y <= bottom - 16)
				copy_line (y, y + 16, left, right);
			else
				clear_line (y, left, right);

}/* os_scroll_area */

static void write_pattern (byte far *screen, byte val, byte mask)
{

	if (mask != 0) {

		 asm mov dx,0x03cf
		 asm mov al,mask
		 asm out dx,al
		 asm les bx,screen
		 asm mov ch,text_bg
		 asm mov al,es:[bx]
		 asm mov es:[bx],ch
		 asm mov al,val
		 asm out dx,al
		 asm mov ch,text_fg
		 asm mov al,es:[bx]
		 asm mov es:[bx],ch

	}

}/* write_pattern */

static void os_display_char (char c)
{
	int width = os_char_width (c);

	if (display <= _TEXT_) {

		asm mov ah,2
		asm mov bh,0
		asm mov dh,byte ptr cursor_y
		asm mov dl,byte ptr cursor_x
		asm int 0x10
		asm mov ah,9
		asm mov bh,0
		asm mov bl,byte ptr text_bg
		asm mov cl,4
		asm shl bl,cl
		asm or bl,byte ptr text_fg
		asm mov cx,1
		asm mov al,byte ptr c
		asm int 0x10

	} else {

		void far *table;
		word mask;
		word val;
		byte mask0;
		byte mask1;

		int shift = cursor_x % 8;
		int offset = cursor_x / 8;

		int i;

		if (fixed_font) {
			table = (byte far *) getvect (0x43) + 16 * c;
			mask = 0xff;
		} else {
			table = serif_font + 16 * (c - 32);
			mask = 0xffff << (16 - width);
		}

		mask0 = mask >> shift;
		mask1 = mask << (8 - shift);

		outport (0x03ce, 0x0205);
		outport (0x03ce, 0xff08);

		for (i = 0; i < 16; i++) {

			byte far *screen = get_scrnptr (cursor_y + i) + offset;

			val = (fixed_font) ?
				*((byte far *) table + i) : *((word far *) table + i);

			if (!fixed_font)
				write_pattern (screen++, val >> (8 + shift), mask >> (8 + shift));

			write_pattern (screen + 0, val >> shift, mask0);
			write_pattern (screen + 1, val << (8 - shift), mask1);

		}

	}

	cursor_x += width;

}/* os_display_char */

static void os_display_string (const char *s)
{

	while (*s != 0)
		os_display_char (*s++);

}/* os_display_string */

static void print_string (const char *s)
{

	while (*s != 0)
		ms_putchar (*s++);

}/* print_string */

static void switch_cursor (bool cursor)
{

	if (display <= _TEXT_) {

		if (display == _MONO_)
			_CX = overwrite ? 0x080f : 0x0a0b;
		else
			_CX = overwrite ? 0x0408 : 0x0506;

		if (!cursor)
			_CX = 0xffff;

		asm mov ah,2
		asm mov bh,0
		asm mov dh,byte ptr cursor_y
		asm mov dl,byte ptr cursor_x
		asm int 0x10
		asm mov ah,1
		asm int 0x10

	} else {

		int saved_x = cursor_x;

		if (cursor)
			swap_colours ();

		if (input.pos < input.length)
			os_display_char (input.buffer[input.pos]);
		else
			os_display_char (' ');

		if (cursor)
			swap_colours ();

		cursor_x = saved_x;

	}

}/* switch_cursor */

static int get_key (void)
{
	static byte arrow_key_map[] = {
		0x48, 0x50, 0x4b, 0x4d
	};
	static byte special_key_map[] = {
		0x47, 0x4f, 0x73, 0x74, 0x53, 0x52, 0x3b
	};
	static byte hot_key_map[] = {
		0x13, 0x19, 0x1f, 0x23, 0x2d, 0x14, 0x16
	};

	int key;

	switch_cursor (TRUE);

	while (TRUE)

		if (_bios_keybrd (_KEYBRD_READY)) {

			word code = _bios_keybrd (_KEYBRD_READ);

			if (byte0 (code) != 0) {

				key = byte0 (code);

				if (key == ZC_BACKSPACE)
					goto exit_loop;
				if (key == ZC_RETURN)
					goto exit_loop;
				if (key == ZC_ESCAPE)
					goto exit_loop;
				if (key >= ZC_ASCII_MIN && key <= ZC_ASCII_MAX)
					goto exit_loop;

			} else {

				for (key = ZC_ARROW_MIN; key <= ZC_ARROW_MAX; key++)
					if (byte1 (code) == arrow_key_map[key - ZC_ARROW_MIN])
						goto exit_loop;

				for (key = ZC_HKEY_MIN; key <= ZC_HKEY_MAX; key++)
					if (byte1 (code) == hot_key_map[key - ZC_HKEY_MIN])
						goto exit_loop;

				for (key = SPECIAL_KEY_MIN; key <= SPECIAL_KEY_MAX; key++)
					if (byte1 (code) == special_key_map[key - SPECIAL_KEY_MIN])
						goto exit_loop;

			}

		}

exit_loop:

	switch_cursor (FALSE);

	return key;

}/* get_key */

static void os_more_prompt (void)
{

	if (more_prompts) {

		cursor_y = (screen_height / font_height - 1) * font_height;
		cursor_x = user_left_margin;

		os_display_string ("[MORE]");

		get_key ();

		os_erase_area (cursor_y,
							user_left_margin,
							cursor_y + font_height - 1,
							cursor_x);

		cursor_x = user_left_margin;

	}

}/* os_more_prompt */

static void set_more_prompts (bool flag)
{

	if (flag && !more_prompts)
		line_count = 0;

	more_prompts = flag;

}/* set_more_prompts */

static void script_close (void)
{

	fclose (sfp); ostream_script = FALSE;

}/* script_close */

static void script_new_line (void)
{

	if (fputc ('\n', sfp) == EOF)
		script_close ();

	script_width = 0;

}/* script_new_line */

static void script_word (const char *s)
{
	int width = strlen (s);

	if (user_script_cols != 0 && script_width + width > user_script_cols) {

		if (*s == ' ')
			{ width--; s++; }

		script_new_line ();

	}

	fputs (s, sfp); script_width += width;

}/* script_word */

static void script_write_input (const char *buf)
{
	int width = strlen (buf);

	if (user_script_cols != 0)

		while (*buf) {

			if (script_width == user_script_cols)
				script_new_line ();

			fputc (*buf++, sfp); script_width++;

		}

	else { fputs (buf, sfp); script_width += width; }

}/* script_write_input */

static void flush_buffer (void)
{
	int i;

	buffer2[bufpos2] = 0;

	os_display_string (buffer2);

	bufpos2 = 0;
	total_width = 0;

}/* flush_buffer */

static void screen_new_line (void)
{

	if (line_count >= screen_height / font_height - pic_lines - 2)
		{ os_more_prompt (); line_count = user_context_lines; }

	flush_buffer ();

	cursor_x = user_left_margin;

	if (cursor_y + 2 * font_height > screen_height)
		scroll_text ();

	else cursor_y += font_height;

	line_count++;

}/* screen_new_line */

static void screen_write_input (const char *buf)
{

	ms_flush ();

	while (*buf) {

		int width = os_char_width (*buf);

		if (total_width + width > screen_width - user_left_margin - max_char_width)
			screen_new_line ();

		buffer2[bufpos2++] = *buf++; total_width += width;

	}

}/* screen_write_input */

static void screen_word (const char *s)
{
	int width = os_string_width (s);
	int i;

	if (cursor_x + total_width + width >= screen_width - user_right_margin) {

		screen_new_line ();

		if (*s == ' ')
			{ s++; width -= os_char_width (' '); }

	}

	for (i = 0; s[i] != 0; i++)
		buffer2[bufpos2++] = s[i];

	total_width += width;

}/* screen_word */

static void record_close (void)
{

	fclose (rfp); ostream_record = FALSE;

}/* record_close */

static void record_write_input (const char *buf, int key)
{

	if (key == ZC_HKEY_UNDO)
		fputs ("#undo", rfp);
	else if (key == ZC_HKEY_SEED)
		fputs ("#seed", rfp);
	else if (key == ZC_HKEY_QUIT)
		fputs ("#exit", rfp);
	else if (key == ZC_HKEY_HELP)
		fputs ("#help", rfp);
	else if (key == ZC_HKEY_RECORD)
		fputs ("#end", rfp);
	else
		fputs (buf, rfp);

	if (fputc ('\n', rfp) == EOF)
		record_close ();

}/* record_write_input */

static void replay_close (void)
{

	set_more_prompts (TRUE);

	fclose (pfp); istream_replay = FALSE;

}/* replay_close */

static int replay_read_input (int max, char *buf)
{
	int key;
	int i;

	if (fgets (buf, max + 1, pfp) == 0)
		{ replay_close (); return ZC_BAD; }

	for (i = 0; buf[i] != 0; i++)
		if (buf[i] == '\n')
			buf[i] = 0;

	if (!strcmp (buf, "#undo"))
		return ZC_HKEY_UNDO;
	else if (!strcmp (buf, "#seed"))
		return ZC_HKEY_SEED;
	else if (!strcmp (buf, "#exit"))
		return ZC_HKEY_QUIT;
	else if (!strcmp (buf, "#help"))
		return ZC_HKEY_HELP;
	else if (!strcmp (buf, "#end"))
		return ZC_HKEY_RECORD;
	else
		return ZC_RETURN;

}/* replay_read_input */

static void cursor_left (void)
{

	if (input.pos > 0) {

		int width = os_char_width (input.buffer[--input.pos]);

		if (cursor_x - width < user_left_margin) {

			input.line--;

			cursor_x = input.right_end[input.line] - width;
			cursor_y -= font_height;

		} else cursor_x -= width;

	}

}/* cursor_left */

static void cursor_right (void)
{

	if (input.pos < input.length) {

		int width = os_char_width (input.buffer[input.pos++]);

		if (cursor_x + width > screen_width - user_right_margin - max_char_width) {

			input.line++;

			cursor_x = user_left_margin;
			cursor_y += font_height;

		} else cursor_x += width;

	}

}/* cursor_right */

static void first_char (void)
{

	while (input.pos > 0)
		cursor_left ();

}/* first_char */

static void last_char (void)
{

	while (input.pos < input.length)
		cursor_right ();

}/* last_char */

static void prev_word (void)
{

	do {

		cursor_left ();

		if (input.pos == 0)
			return;

	} while (input.buffer[input.pos] == ' ' || input.buffer[input.pos - 1] != ' ');

}/* prev_word */

static void next_word (void)
{

	do {

		cursor_right ();

		if (input.pos == input.length)
			return;

	} while (input.buffer[input.pos] == ' ' || input.buffer[input.pos - 1] != ' ');

}/* next_word */

static void input_move (char newc, char oldc)
{
	char *p = input.buffer + input.pos;

	int saved_x;
	int saved_y;

	int i;

	if (oldc == 0 && newc != 0 && input.length == input.max_length)
		return;

	if (oldc != 0)
		input.length--;
	if (newc != 0)
		input.length++;

	if (oldc != 0 && newc == 0)
		memmove (p, p + 1, input.length - input.pos + 1);
	if (newc != 0 && oldc == 0)
		memmove (p + 1, p, input.length - input.pos);

	if (newc != 0)
		*p = newc;

	saved_x = cursor_x;
	saved_y = cursor_y;

	for (i = input.line; i < input.peak; i++) {

		if (i != input.line) {
			cursor_x = user_left_margin;
			cursor_y += font_height;
		}

		while (*p && cursor_x <= screen_width - user_right_margin - max_char_width)
			os_display_char (*p++);

		if (cursor_x < input.right_end[i])

			os_erase_area (cursor_y,
								cursor_x,
								cursor_y + font_height - 1,
								input.right_end[i]);

		input.right_end[i] = cursor_x;

	}

	if (cursor_x > screen_width - user_right_margin - max_char_width) {

		input.peak++;

		cursor_x = user_left_margin;

		if (cursor_y + 2 * font_height > screen_height) {

			scroll_text ();

			saved_y -= font_height;

		} else cursor_y += font_height;

		os_display_string (p);

		input.right_end[i] = cursor_x;

	}

	cursor_x = saved_x;
	cursor_y = saved_y;

	if (newc != 0)
		cursor_right ();

}/* input_move */

#undef H(x)

static void delete_char (void)
{

	input_move (0, input.buffer[input.pos]);

}/* delete_char */

static void delete_left (void)
{

	if (input.pos > 0) {
		cursor_left ();
		delete_char ();
	}

}/* delete_left */

static void truncate_line (int n)
{

	last_char ();

	while (input.length > n)
		delete_left ();

}/* truncate_line */

static void insert_char (char newc)
{
	char oldc = 0;

	if (overwrite)
		oldc = input.buffer[input.pos];

	input_move (newc, oldc);

}/* insert_char */

static void insert_string (const char *s)
{

	while (*s != 0) {

		if (input.length + 1 > input.max_length)
			break;

		insert_char (*s++);

	}

}/* insert_string */

static void store_input (void)
{

	if (input.length >= HISTORY_MIN_ENTRY) {

		const char *ptr = input.buffer;

		do {

			if (history.latest++ == HISTORY_BUFFER_SIZE - 1)
				history.latest = 0;

			history.buffer[history.latest] = *ptr;

		} while (*ptr++ != 0);

	}

}/* store_input */

static bool fetch_entry (char *buf, int entry)
{
	int i = 0;

	char c;

	do {

		if (entry++ == HISTORY_BUFFER_SIZE - 1)
			entry = 0;

		c = history.buffer[entry];

		if (i < history.prefix_len && input.buffer[i] != c)
			return FALSE;

		buf[i++] = c;

	} while (c != 0);

	return i > history.prefix_len && i > 1;

}/* fetch_entry */

static void get_prev_entry (void)
{
	char buf[INPUT_BUFFER_SIZE];

	int i = history.current;

	do {

		do {

			if (i-- == 0)
				i = HISTORY_BUFFER_SIZE - 1;

			if (i == history.latest)
				return;

		} while (history.buffer[i] != 0);

	} while (!fetch_entry (buf, i));

	truncate_line (history.prefix_len);

	insert_string (buf + history.prefix_len);

	history.current = i;

}/* get_prev_entry */

static void get_next_entry (void)
{
	char buf[INPUT_BUFFER_SIZE];

	int i = history.current;

	truncate_line (history.prefix_len);

	do {

		do {

			if (i == history.latest)
				return;

			if (i++ == HISTORY_BUFFER_SIZE - 1)
				i = 0;

		} while (history.buffer[i] != 0);

		if (i == history.latest)
			goto no_further;

	} while (!fetch_entry (buf, i));

	insert_string (buf + history.prefix_len);

no_further:

	 history.current = i;

}/* get_next_entry */

static int os_read_line (int max, char *buf)
{
	int key;

	line_count = 0;

	input.buffer = buf;
	input.pos = 0;
	input.length = 0;
	input.max_length = max;
	input.line = 0;
	input.peak = 1;
	input.right_end[0] = cursor_x;

	do {

		history.prefix_len = input.pos;
		history.current = history.latest;

		do {

			key = get_key ();

			if (key == ZC_ARROW_UP)
				get_prev_entry ();
			if (key == ZC_ARROW_DOWN)
				get_next_entry ();
			if (key == ZC_ARROW_LEFT)
				cursor_left ();
			if (key == ZC_ARROW_RIGHT)
				cursor_right ();

		} while (key >= ZC_ARROW_MIN && key <= ZC_ARROW_MAX);

		if (key < ZC_ASCII_MIN || key > ZC_ASCII_MAX) {

			if (key == ZC_BACKSPACE)
				delete_left ();
			if (key == ZC_RETURN)
				store_input ();
			if (key == ZC_ESCAPE)
				truncate_line (0);

			if (key == SPECIAL_KEY_HOME)
				first_char ();
			if (key == SPECIAL_KEY_END)
				last_char ();
			if (key == SPECIAL_KEY_WORD_LEFT)
				prev_word ();
			if (key == SPECIAL_KEY_WORD_RIGHT)
				next_word ();
			if (key == SPECIAL_KEY_DELETE)
				delete_char ();
			if (key == SPECIAL_KEY_INSERT)
				overwrite = !overwrite;
			if (key == SPECIAL_KEY_F1 && pic_number != -1)
				{ scaler = 3 - scaler; ms_showpic (pic_number, 2); }

		} else insert_char (key);

	} while (key != ZC_RETURN && !(key >= ZC_HKEY_MIN && key <= ZC_HKEY_MAX));

	last_char ();

	overwrite = FALSE;

	line_count = input.peak - 1;

	return key;

}/* os_read_line */

static int console_read_input (int max, char *buf)
{

	flush_buffer ();

	return os_read_line (max, buf);

}/* console_read_input */

void handle_hot_key (char);

static int stream_read_string (int max, char *buf, bool hot_keys)
{
	int key;

continue_input:

	do {

		ms_flush ();

		buf[0] = 0;

		if (istream_replay)
			key = replay_read_input (max, buf);
		else
			key = console_read_input (max, buf);

	} while (key == ZC_BAD);

	if (ostream_record && !istream_replay)
		record_write_input (buf, key);

	if (hot_keys && key >= ZC_HKEY_MIN && key <= ZC_HKEY_MAX) {

		handle_hot_key (key);

		if (key == ZC_HKEY_UNDO)
			return 0;

		goto continue_input;

	}

	if (ostream_script)
		script_write_input (buf);
	if (istream_replay)
		screen_write_input (buf);

	return key;

}/* stream_read_string */

static bool read_yes_or_no (const char *s)
{
	char buf[2];
	int key;

	print_string (s);
	print_string ("? (y/n) >");

	do {

		key = stream_read_string (1, buf, FALSE);

	} while (key != ZC_RETURN);

	print_string ("\n");

	return buf[0] == 'y' || buf[0] == 'Y';

}/* read_yes_or_no */

static void read_string (int max, char *buffer)
{
	int key;

	buffer[0] = 0;

	do {

		key = stream_read_string (max, buffer, TRUE);

	} while (key != ZC_RETURN);

	print_string ("\n");

}/* read_string */

int read_number (void)
{
	char buf[6];
	int value = 0;
	int i;

	read_string (5, buf);

	for (i = 0; buf[i] != 0; i++)
		if (buf[i] >= '0' && buf[i] <= '9')
			value = 10 * value + buf[i] - '0';

	 return value;

}/* read_number */

int os_read_file_name (char *file_name, const char *default_name, int flag)
{
	char *extension;
	FILE *fp;
	bool terminal;
	bool result;

	bool saved_replay = istream_replay;
	bool saved_record = ostream_record;

	istream_replay = FALSE;
	ostream_record = FALSE;

	if (flag == FILE_SAVE || flag == FILE_RESTORE)
		extension = ".sav";
	if (flag == FILE_SCRIPT)
		extension = ".scr";
	if (flag == FILE_RECORD || flag == FILE_PLAYBACK)
		extension = ".rec";

	print_string ("Enter file name (\"");
	print_string (extension);
	print_string ("\" will be added).\nDefault is \"");
	print_string (default_name);
	print_string ("\": ");

	read_string (MAX_FILE_NAME - 4, file_name);

	if (file_name[0] == 0)
		strcpy (file_name, default_name);
	if (strchr (file_name, '.') == NULL)
		strcat (file_name, extension);

	result = TRUE;

	if (flag == FILE_SAVE || flag == FILE_RECORD) {

		if ((fp = fopen (file_name, "rb")) == NULL)
			goto finished;

		terminal = fp->flags & _F_TERM;

		fclose (fp);

		if (terminal)
			goto finished;

		result = read_yes_or_no ("Overwrite existing file");

	}

finished:

	istream_replay = saved_replay;
	ostream_record = saved_record;

	return result;

}/* os_read_file_name */

static void record_open (void)
{
	char new_name[MAX_FILE_NAME + 1];

	if (os_read_file_name (new_name, command_name, FILE_RECORD)) {

		strcpy (command_name, new_name);

		if ((rfp = fopen (new_name, "wt")) != NULL)
			ostream_record = TRUE;
		else
			print_string ("Cannot open file\n");

	}

}/* record_open */

static void replay_open (void)
{
	char new_name[MAX_FILE_NAME + 1];

	if (os_read_file_name (new_name, command_name, FILE_PLAYBACK)) {

		strcpy (command_name, new_name);

		if ((pfp = fopen (new_name, "rt")) != NULL) {

			set_more_prompts (read_yes_or_no ("Do you want MORE prompts"));

			istream_replay = TRUE;

		} else print_string ("Cannot open file\n");

	}

}/* replay_open */

void script_open (void)
{
	char new_name[MAX_FILE_NAME + 1];

	if (os_read_file_name (new_name, script_name, FILE_SCRIPT)) {

		strcpy (script_name, new_name);

		if ((sfp = fopen (script_name, "at")) != NULL) {

			ostream_script = TRUE;
			script_width = 0;

		} else print_string ("Cannot open file\n");

	}

}/* script_open */

static void hot_key_help (void) {

	print_string ("\nHot key -- Help\n");

	print_string ("\n"
		"Alt-H  help\n"
		"Alt-P  playback on\n"
		"Alt-R  recording on/off\n"
		"Alt-S  seed random numbers\n"
		"Alt-T  transscription on/off\n"
		"Alt-U  undo last turn\n"
		"Alt-X  exit game\n"
		"\n"
		"F1 toggles between small and large pictures.\n");

}/* hot_key_help */

static void hot_key_playback (void)
{

	print_string ("\nHot key -- Playback on\n");

	if (!istream_replay)
		replay_open ();

}/* hot_key_playback */

static void hot_key_recording (void)
{

	if (istream_replay) {
		print_string ("\nHot key -- Playback off\n");
		replay_close ();
	} else if (ostream_record) {
		print_string ("\nHot key -- Recording off\n");
		record_close ();
	} else {
		print_string ("\nHot key -- Recording on\n");
		record_open ();
	}

}/* hot_key_recording */

extern void srand_emu (type32);

static void hot_key_seed (void)
{

	 print_string ("\nHot key -- Seed random numbers\n");

	 print_string ("Enter seed value (or return to randomize): ");
	 srand_emu (read_number ());

}/* hot_key_seed */

static void hot_key_quit (void)
{

	print_string ("\nHot key -- Exit game\n");

	if (read_yes_or_no ("Do you wish to quit"))
		geninterrupt (0x1b);

}/* hot_key_quit */

static void hot_key_script (void)
{

	if (ostream_script) {

		script_new_line ();
		script_word ("*** end of transscription ***");
		script_new_line ();

		script_close ();

		print_string ("\nHot key -- Transscription off\n");

	} else {

		print_string ("\nHot key -- Transscription on\n");

		script_open ();

		script_new_line ();
		script_word ("*** start of transscription ***");
		script_new_line ();

	}

}/* hot_key_script */

static void hot_key_undo (void)
{

	print_string ("\nHot key -- Undo turn");

}/* hot_key_undo */

static void handle_hot_key (char key)
{
	static void (*hot_keys[]) (void) = {
		hot_key_recording,
		hot_key_playback,
		hot_key_seed,
		hot_key_help,
		hot_key_quit,
		hot_key_script,
		hot_key_undo
	};

	hot_keys[key - ZC_HKEY_MIN] ();

	if (key != ZC_HKEY_UNDO)
		print_string ("\n>");

}/* handle_hot_key */

void ms_putchar (type8 c)
{

	if (c == '\n') {

		ms_flush ();

		if (ostream_screen)
			screen_new_line ();
		if (ostream_script)
			script_new_line ();

	} else if (c == '\b') {

		if (bufpos != 0)
			bufpos--;

	} else {

		if (c == ' ' || bufpos == TEXT_BUFFER_SIZE)
			ms_flush ();

		buffer[bufpos++] = c;

	}

}/* ms_putchar */

void ms_flush (void)
{

	if (bufpos == 0)
		return;

	buffer[bufpos] = 0;

	if (ostream_screen)
		screen_word (buffer);
	if (ostream_script)
		script_word (buffer);

	bufpos=0;

}/* ms_flush */

type8 ms_getchar (void)
{
	static char buffer[INPUT_BUFFER_SIZE];
	static bufpos = 0;

	char c;

	if (bufpos == 0)
		if (stream_read_string (INPUT_BUFFER_SIZE - 1, buffer, TRUE) == 0)
			return 0;

	c = buffer[bufpos++];

	if (c == 0) {

		bufpos = 0;
		return '\n';

	} else return c;

}/* ms_getchar */

type8 ms_load_file (type8 *name, type8 *ptr, type16 size)
{
	char file_name[MAX_FILE_NAME + 1];
	FILE *fp;
	int result;

	if (name == NULL) {

		if (!os_read_file_name (file_name, save_name, FILE_RESTORE))
			return 1;

		strcpy (save_name, file_name);

	} else strcpy (save_name, name);

	fp = fopen (save_name, "rb");

	if (fp == NULL)
		return 1;

	result = fread (ptr, size, 1, fp);

	fclose (fp);

	return (result != 0) ? 0 : 1;

}/* ms_load_file */

type8 ms_save_file (type8 *name, type8 *ptr, type16 size)
{
	char file_name[MAX_FILE_NAME + 1];
	FILE *fp;
	int result;

	if (name == NULL) {

		if (!os_read_file_name (file_name, save_name, FILE_SAVE))
			return 1;

		strcpy (save_name, file_name);

	} else strcpy (save_name, name);

	fp = fopen (save_name, "wb");

	if (fp == NULL)
		return 1;

	result = fwrite (ptr, size, 1, fp);

	fclose (fp);

	return (result != 0) ? 0 : 1;

}/* ms_save_file */

void ms_statuschar (type8 c)
{
	static saved_x = 0;
	static saved_y = 0;

	static count = 0;

	if (count == 0) {

		if (display <= _TEXT_) {
			text_bg = user_reverse_bg;
			text_fg = user_reverse_fg;
		}

		saved_x = cursor_x;
		saved_y = cursor_y;

		cursor_x = 0;
		cursor_y = 0;

		fixed_font = TRUE;

		{ os_display_char (' '); count++; }

	}

	if (c == '\n') {

		while (count * font_width < screen_width)
			{ os_display_char (' '); count++; }

		cursor_x = saved_x;
		cursor_y = saved_y;

		fixed_font = FALSE;

		count = 0;

		if (display <= _TEXT_) {
			text_bg = user_background;
			text_fg = user_foreground;
		}

	} else if (c == '\t') {

		while ((count + 11) * font_width < screen_width)
			{ os_display_char (' '); count++; }

	} else

		{ os_display_char (c); count++; }

}/* ms_statuschar */

void ms_showpic (type8 number, type8 mode)
{

	byte far *image;

	word palette[16];
	word width;
	word height;

	if (display <= _TEXT_)
		return;

	if (mode != 0 && (image = ms_extract (number, &width, &height, palette)) != NULL) {

		byte rgb[16][3];
		byte mapper[16];
		byte h;
		int old_lines;
		int dx, dy;
		int x, y;
		int i;
		int min, max;
		int min_prod, max_prod;
		int bg, fg;
		int count;

		old_lines = pic_lines;

		pic_lines = (scaler * height + 15) / 16;
		pic_number = number;

		if (old_lines == 0) {

			os_more_prompt (); line_count = 0;

			os_erase_area (16,
								0,
								screen_height - 1,
								screen_width - 1);

			cursor_y = 16 * (pic_lines + 1);

		} else {

			if (line_count >= screen_height / 16 - pic_lines - 2)
				{ os_more_prompt (); line_count = user_context_lines; }

			os_erase_area (16,
								0,
								16 * (max (pic_lines, old_lines) + 1) - 1,
								screen_width - 1);

		}

		for (i = 0; i < 16; i++) {
			rgb[i][0] = (palette[i] >> 8) & 7;
			rgb[i][1] = (palette[i] >> 4) & 7;
			rgb[i][2] = (palette[i] >> 0) & 7;
		}

		min = 7 + 7 + 7;
		max = 0;

		min_prod = 0;
		max_prod = 0;

		for (i = 0; i < 16; i++) {

			int sum = rgb[i][0] + rgb[i][1] + rgb[i][2];
			int prod = rgb[i][0] * rgb[i][1] * rgb[i][2];

			if (sum < min || sum == min && prod >= min_prod)
				{ min = sum; min_prod = prod; bg = i; }
			if (sum > max || sum == max && prod >= max_prod)
				{ max = sum; max_prod = prod; fg = i; }

			mapper[i] = i;

		}

		mapper[0] = bg;
		mapper[1] = fg;
		mapper[bg] = 0;
		mapper[fg] = 1;

		h = rgb[bg][0]; rgb[bg][0] = rgb[0][0]; rgb[0][0] = h;
		h = rgb[bg][1]; rgb[bg][1] = rgb[0][1]; rgb[0][1] = h;
		h = rgb[bg][2]; rgb[bg][2] = rgb[0][2]; rgb[0][2] = h;
		h = rgb[fg][0]; rgb[fg][0] = rgb[1][0]; rgb[1][0] = h;
		h = rgb[fg][1]; rgb[fg][1] = rgb[1][1]; rgb[1][1] = h;
		h = rgb[fg][2]; rgb[fg][2] = rgb[1][2]; rgb[1][2] = h;

		for (i = 0; i < 16; i++) {
			rgb[i][0] = (63 * rgb[i][0] + 4) / 7;
			rgb[i][1] = (63 * rgb[i][1] + 4) / 7;
			rgb[i][2] = (63 * rgb[i][2] + 4) / 7;
		}

		asm mov ax,0x1012
		asm mov bx,0
		asm mov cx,16
		asm lea dx,rgb
		asm push ss
		asm pop es
		asm int 0x10

		outport (0x03ce, 0x0205);
		outport (0x03ce, 0xff08);

		dx = (screen_width - scaler * width) / 2;
		dy = (16 * pic_lines - scaler * height) / 2;

		for (y = 0; y < height; y++) {

			byte far *screen = get_scrnptr (16 + dy + scaler * y) + dx / 8;

			int bit = 0x80 >> (dx % 8);

			for (x = 0; x < width; x++) {

				for (count = 0; count < scaler; count++) {

					_AL = *screen;

					outportb (0x03cf, bit);

					screen[0] = mapper[*image];

					if (scaler >= 2)
						screen[80] = mapper[*image];

					bit >>= 1;

					if (bit == 0)
						{ bit = 0x80; screen++; }

				}

				image++;

			}

		}

	} else if (pic_lines != 0) {

      os_more_prompt ();

		os_erase_area (16, 0, screen_height - 1, screen_width - 1);

		pic_lines = 0;
		pic_number = -1;

		asm mov ax,0x1010
		asm mov bx,0
		asm mov dh,0
		asm mov ch,0
		asm mov cl,0
		asm int 0x10
		asm mov ax,0x1010
		asm mov bx,1
		asm mov dh,63
		asm mov ch,63
		asm mov cl,63
		asm int 0x10

		cursor_y = font_height;
		cursor_x = 0;

		screen_new_line ();

	}

}/* ms_showpic */

int cdecl main (int argc, char **argv)
{
	int i;

	os_process_arguments (argc, argv);

	os_init_screen ();

	if (ms_init (story_name, (display >= _AMIGA_) ? gfx_name : NULL) == 0)
		internal ("Couldn't start up game");

	os_erase_area (0, 0, screen_height - 1, screen_width - 1);

	while (ms_rungame () != 0);

	cleanup ();

	ms_freemem();

	return 0;

}/* main */

