/*
 * EPOC interface for Magnetic Scrolls v2.0 interpreter by Simon Quinn 02/03/02
 * Adapted from the DOS interface by Stefan Jokisch
 *
 */
#include <e32base.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "epoc_console.h"

#define FILE_RESTORE 0
#define FILE_SAVE 1
#define FILE_SCRIPT 2
#define FILE_PLAYBACK 3
#define FILE_RECORD 4

#define MAX_FILE_NAME 128
#define INPUT_BUFFER_SIZE 200
#define TEXT_BUFFER_SIZE 100
#define TEXT_BUFFER2_SIZE 200
#define HISTORY_BUFFER_SIZE 500
#define HISTORY_MIN_ENTRY 1
#define MAX_INPUT_LINES 10

#define ZC_HKEY_MIN 0x09
#define ZC_HKEY_RECORD 0x12
#define ZC_HKEY_PLAYBACK 0x10
#define ZC_HKEY_SEED 0x13
#define ZC_HKEY_HELP 0x09
#define ZC_HKEY_QUIT 0x18
#define ZC_HKEY_SCRIPT 0x14
#define ZC_HKEY_UNDO 0x15
#define ZC_HKEY_INFO 0x16
#define ZC_HKEY_MAX 0x18
#define ZC_BAD 0x7f		//Used in the script read/write I think, not inputted

#define DEFAULT_SAVE_NAME "story.sav"
#define DEFAULT_SCRIPT_NAME "story.scr"
#define DEFAULT_COMMAND_NAME "story.rec"
#define DEFAULT_AUXILARY_NAME "story.aux"

CAdvConsoleClient *iConsole;	//EIK Console to interact with

typedef unsigned char byte;
typedef unsigned short word;

extern const char *optarg;
extern int optind;

static char *prog_name = NULL;

static char story_name[MAX_FILE_NAME + 1];
static char gfx_name[MAX_FILE_NAME + 1];
static char title_name[MAX_FILE_NAME + 1];
static char script_name[MAX_FILE_NAME + 1] = DEFAULT_SCRIPT_NAME;
//static char init_script_name[MAX_FILE_NAME + 1] = "";
static char command_name[MAX_FILE_NAME + 1] = DEFAULT_COMMAND_NAME;
static char save_name[MAX_FILE_NAME + 1] = DEFAULT_SAVE_NAME;

static int user_script_cols = 80;
static int user_context_lines = 0;
static int user_left_margin = 0;
static int user_right_margin = 0;
//static int user_random_seed = -1;

static int screen_height = 0;
static int screen_width = 0;

static int cursor_x = 0;
static int cursor_y = 0;

static char buffer[TEXT_BUFFER_SIZE];
static char buffer2[TEXT_BUFFER2_SIZE];
static char script_buffer[TEXT_BUFFER_SIZE];
static int bufpos = 0;
static int bufpos2 = 0;
static int scrbufpos = 0;

static int total_width = 0;

static bool ostream_screen = TRUE;
static bool ostream_script = FALSE;
static bool ostream_record = FALSE;
static bool istream_replay = FALSE;

static int script_width = 0;

static FILE *sfp = NULL;
static FILE *pfp = NULL;
static FILE *rfp = NULL;

static int line_count = 0;

static int pic_lines = 0;

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

type8 log_on=0;
FILE *log=0,*log2=0;

type16 pic_palette[16];


static void os_process_arguments (int argc, char *argv[])
{
	int /*pos1, */pos2;

//	if (argc < 2) {
//		puts (INFORMATION);
//		exit (EXIT_FAILURE);
//	}

//	pos1 = 0;

//	for (pos2 = pos1; argv[1][pos2] != 0; pos2++)
//		if (argv[1][pos2] == '\\' || argv[1][pos2] == ':')
//			pos1 = pos2 + 1;

	for (pos2 = strlen(argv[1]) - 1; pos2 > 0; pos2--)
		if (argv[1][pos2] == '.')
			break;

	strcpy (story_name, argv[1]);
	strcpy (gfx_name, argv[1]);
	strcpy (title_name, argv[1]);

	strcpy (story_name + pos2, ".mag");
	strcpy (gfx_name + pos2, ".gfx");
	strcpy (title_name + pos2, ".mbm");

	strcpy (script_name, argv[1]);
	strcpy (command_name, argv[1]);
	strcpy (save_name, argv[1]);

	strcpy (script_name + pos2, ".scr");
	strcpy (command_name + pos2, ".rec");
	strcpy (save_name + pos2, ".sav");

	prog_name = argv[0];

}/* os_process_arguments */



int os_char_width (char c)
{
	return 1;

}/* os_char_width */

void os_init_screen (void)
{
	screen_width = iConsole->ScreenSize().iWidth;
	screen_height = iConsole->ScreenSize().iHeight;

	cursor_x = 0;
	cursor_y = 1;

}/* os_init_screen */

void clear_line (int y, int left, int right)
{
	iConsole->ClearChars(TRect(left,y,right,y+1),0);
	
}/* clear_line */

void os_erase_area (int top, int left, int bottom, int right)
{
	iConsole->ClearChars(TRect(left,top, right, bottom),0);
	
}/* os_erase_area */

void scroll_text (void)
{
	int top = pic_lines + 1;
	int bottom = screen_height /*- 1*/;
	int left = user_left_margin;
	int right = screen_width - user_right_margin - 1;

	iConsole->ScrollChars(TRect(left, top, right, bottom), TPoint(0,1));

}/* os_scroll_area */

void os_display_char (char c)
{
    TBuf<1> tempchar;
	tempchar.Append(c);

	iConsole->SetPos(cursor_x, cursor_y);

	iConsole->Write(tempchar);

	cursor_x++;
}

void os_display_string (const char *s)
{
	TBuf<256> tempdesc;

	tempdesc.Copy(_L8(s));

	iConsole->SetPos(cursor_x, cursor_y);

	iConsole->Write(tempdesc);

	cursor_x += tempdesc.Length();

}/* os_display_string */


void print_string (const char *s)
{
	while (*s != 0)
		ms_putchar (*s++);

}/* print_string */

void switch_cursor (bool cursor)
{
	int saved_x = cursor_x;

	if (input.pos < input.length)
	{
		TBuf<2> tempchar;
		tempchar.Append(input.buffer[input.pos]);
		iConsole->Write(tempchar);
	}
	else
		iConsole->Write(_L(" "));

	cursor_x = saved_x;

}/* switch_cursor */

void os_more_prompt (void)
{
	if (more_prompts) {

		cursor_y = screen_height - 1;
		cursor_x = user_left_margin;

		os_display_string ("[MORE]");

		iConsole->Getch();

		os_erase_area (cursor_y,
							user_left_margin,
					/*		cursor_y - 1,  */
							cursor_y + 1,  
							cursor_x);

		cursor_x = user_left_margin;

	}

}/* os_more_prompt */

void set_more_prompts (bool flag)
{

	if (flag && !more_prompts)
		line_count = 0;

	more_prompts = flag;

}/* set_more_prompts */

void flush_script_buffer (void)
{
	script_buffer[scrbufpos++] = 0;
	fputs(script_buffer,sfp);
	scrbufpos = 0;

}/* flush_script_buffer */

void script_put(const char* s)
{
	int width = strlen(s);
	int i;

	for (i = 0; i < width; i++)
	{
		if (scrbufpos < TEXT_BUFFER_SIZE-1)
			script_buffer[scrbufpos++] = *s++;
		else
			flush_script_buffer();
	}
}/* script_put */

void script_close (void)
{
	flush_script_buffer();
	fclose (sfp); ostream_script = FALSE;
}/* script_close */

void script_new_line (void)
{

	flush_script_buffer();
	if (fputc ('\n', sfp) == EOF)
		script_close ();

	script_width = 0;

}/* script_new_line */

void script_word (const char *s)
{
	int width = strlen (s);

	if (user_script_cols != 0 && script_width + width > user_script_cols) {

		if (*s == ' ')
			{ width--; s++; }

		script_new_line ();

	}

	script_put (s); script_width += width;

}/* script_word */

void script_write_input (const char *buf)
{
	int width = strlen (buf);

	flush_script_buffer();
	if (user_script_cols != 0)

		while (*buf) {

			if (script_width == user_script_cols)
				script_new_line ();

			fputc (*buf++, sfp); script_width++;

		}

	else { fputs (buf, sfp); script_width += width; }

}/* script_write_input */

void flush_buffer (void)
{
	buffer2[bufpos2] = 0;

	os_display_string (buffer2);

	bufpos2 = 0;
	total_width = 0;

}/* flush_buffer */

void screen_new_line (void)
{

	if (line_count >= screen_height - pic_lines - 2)
		{ os_more_prompt (); line_count = user_context_lines; }

	flush_buffer ();

	cursor_x = user_left_margin;

	if (cursor_y + 2  > screen_height)
		scroll_text ();

	else cursor_y++;

	line_count++;

}/* screen_new_line */

void screen_write_input (const char *buf)
{

	ms_flush ();

	while (*buf) {

		int width = os_char_width (*buf);

		if (total_width + width > screen_width - user_left_margin - 1)
			screen_new_line ();

		buffer2[bufpos2++] = *buf++; total_width += width;

	}

}/* screen_write_input */

void screen_word (const char *s)
{
	int width = strlen(s);
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

void record_close (void)
{

	fclose (rfp); ostream_record = FALSE;

}/* record_close */

void record_write_input (const char *buf, int key)
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
	else if (key == ZC_HKEY_INFO)
		fputs ("#info", rfp);
	else
		fputs (buf, rfp);

	if (fputc ('\n', rfp) == EOF)
		record_close ();

}/* record_write_input */

void replay_close (void)
{

	set_more_prompts (TRUE);

	fclose (pfp); istream_replay = FALSE;

}/* replay_close */

int replay_read_input (int max, char *buf)
{
//	int key;
	int i;

	if (fgets (buf, max + 1, pfp) == 0)
		{ replay_close (); return ZC_BAD; }

	for (i = 0; buf[i] != 0; i++)
		if (buf[i] == 10 || buf[i] == 13)
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
	else if (!strcmp (buf, "#info"))
		return ZC_HKEY_INFO;
	else
		return EKeyEnter;

}/* replay_read_input */

void cursor_left (void)
{

	if (input.pos > 0) {

		int width = os_char_width (input.buffer[--input.pos]);

		if (cursor_x - width < user_left_margin) {

			input.line--;

			cursor_x = input.right_end[input.line] - width;
			cursor_y--;

		} else cursor_x -= width;

	}
	iConsole->SetPos(cursor_x, cursor_y);

}/* cursor_left */

void cursor_right (void)
{

	if (input.pos < input.length) {

		int width = os_char_width (input.buffer[input.pos++]);

		if (cursor_x + width > screen_width - user_right_margin - 1) {

			input.line++;

			cursor_x = user_left_margin;
			cursor_y++;

		} else cursor_x += width;

	}
	iConsole->SetPos(cursor_x, cursor_y);

}/* cursor_right */

void first_char (void)
{

	while (input.pos > 0)
		cursor_left ();

}/* first_char */

void last_char (void)
{

	while (input.pos < input.length)
		cursor_right ();

}/* last_char */

void prev_word (void)
{

	do {

		cursor_left ();

		if (input.pos == 0)
			return;

	} while (input.buffer[input.pos] == ' ' || input.buffer[input.pos - 1] != ' ');

}/* prev_word */

void next_word (void)
{

	do {

		cursor_right ();

		if (input.pos == input.length)
			return;

	} while (input.buffer[input.pos] == ' ' || input.buffer[input.pos - 1] != ' ');

}/* next_word */

void input_move (char newc, char oldc)
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
			cursor_y++;
		}

		while (*p && cursor_x <= screen_width - user_right_margin - 1)
			os_display_char (*p++);

		if (cursor_x < input.right_end[i])

			os_erase_area (cursor_y, cursor_x,/* cursor_y - 1, */ cursor_y + 1,
								input.right_end[i]+1);

		input.right_end[i] = cursor_x;

	}

	if (cursor_x > screen_width - user_right_margin - 1) {

		input.peak++;

		cursor_x = user_left_margin;

		if (cursor_y + 2 > screen_height) {

			scroll_text ();

			saved_y--;

		} else cursor_y++;

		os_display_string (p);

		input.right_end[i] = cursor_x;

	}

	cursor_x = saved_x;
	cursor_y = saved_y;

	if (newc != 0)
		cursor_right ();

}/* input_move */

void delete_char (void)
{

	input_move (0, input.buffer[input.pos]);
	iConsole->SetPos(cursor_x, cursor_y);

}/* delete_char */

void delete_left (void)
{

	if (input.pos > 0) {
		cursor_left ();
		delete_char ();
	}

}/* delete_left */

void truncate_line (int n)
{

	last_char ();

	while (input.length > n)
		delete_left ();

}/* truncate_line */

void insert_char (char newc)
{
	char oldc = 0;

	if (overwrite)
		oldc = input.buffer[input.pos];

	input_move (newc, oldc);

}/* insert_char */

void insert_string (const char *s)
{

	while (*s != 0) {

		if (input.length + 1 > input.max_length)
			break;

		insert_char (*s++);

	}

}/* insert_string */

void store_input (void)
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

void get_prev_entry (void)
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

void get_next_entry (void)
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

int os_read_line (int max, char *buf)
{
	TKeyCode key;
	TInt keymodifier=0;

	line_count = 0;

	input.buffer = buf;
	input.pos = 0;
	input.length = 0;
	input.max_length = max;
	input.line = 0;
	input.peak = 1;
	input.right_end[0] = cursor_x;

	do 
	{

		history.prefix_len = input.pos;
		history.current = history.latest;

		do
		{
			key = iConsole->Getch();

			keymodifier = iConsole->KeyModifiers();

			//If Ctrl was pressed then ignore case and exit loop
			if(keymodifier & EModifierLeftCtrl)
				break;
			else
			{
				switch (key)
				{
					case EKeyUpArrow:
						get_prev_entry ();
						break;
					case EKeyDownArrow:
						get_next_entry ();
						break;
					case EKeyLeftArrow:
						cursor_left ();
						break;
					case EKeyRightArrow:
						cursor_right ();
						break;
					default:
						break;
				}
			}
		} while (key >= EKeyLeftArrow && key <= EKeyDownArrow);

		if(key == EKeyLeftArrow && (keymodifier & EModifierLeftCtrl))
			prev_word ();
		else if(key == EKeyRightArrow && (keymodifier & EModifierLeftCtrl))
			next_word();
		else
		{
			switch (key)
			{
				case EKeyBackspace:
					delete_left ();
					break;
				case EKeyEnter:
					store_input ();
					break;
				case EKeyEscape:
					truncate_line (0);
					break;
				case EKeyHome:
					first_char ();
					break;
				case EKeyEnd:
					last_char ();
					break;
//				case 10004:		//Zoom Out
//				case 10003:		//Zoom In
//				{
//					truncate_line (0);
//					iConsole->ScreenZoom();
//					os_init_screen();
//					os_erase_area (0, 0, screen_height, screen_width);
//					iConsole->SetPos(cursor_x, cursor_y);
//					break;
//				}

				default:
				{
					if(key >=32 && key <= 255)
						insert_char ((char)key);
					break;
				}
			}
		}

	} while (key != EKeyEnter && !(key >= ZC_HKEY_MIN && key <= ZC_HKEY_MAX));

	last_char ();

	overwrite = FALSE;

	line_count = input.peak - 1;

	return (int) key;

}/* os_read_line */

int console_read_input (int max, char *buf)
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

	if (hot_keys && key >= ZC_HKEY_MIN && key <= ZC_HKEY_MAX && key != EKeyEnter) {

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

	} while (key != EKeyEnter);

	print_string ("\n");

	return buf[0] == 'y' || buf[0] == 'Y';

}/* read_yes_or_no */

void read_string (int max, char *buffer)
{
	int key;

	buffer[0] = 0;

	do {

		key = stream_read_string (max, buffer, TRUE);

	} while (key != EKeyEnter);

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
	bool result;

	bool saved_replay = istream_replay;
	bool saved_record = ostream_record;

	istream_replay = FALSE;
	ostream_record = FALSE;


	TBuf<256> tempfilename;
	TBuf8<256> tempfilename8;
	tempfilename.Copy(_L8(default_name));

	if(flag == FILE_PLAYBACK || flag == FILE_RESTORE )
		iConsole->OpenDialog(tempfilename);
	if(flag == FILE_SAVE || flag == FILE_RECORD || flag == FILE_SCRIPT)
		iConsole->SaveDialog(tempfilename);

	tempfilename8.Copy(tempfilename);
	tempfilename8.ZeroTerminate();
	Mem::Copy(file_name, (unsigned char *) tempfilename8.Ptr(), tempfilename8.Length()+1);

	if (file_name[0] != 0)
		result = TRUE;
	else
		result = FALSE;

	istream_replay = saved_replay;
	ostream_record = saved_record;

	return result;

}/* os_read_file_name */


int record_open (void)
{
	char new_name[MAX_FILE_NAME + 1];

	if (os_read_file_name (new_name, command_name, FILE_RECORD)) {

		strcpy (command_name, new_name);

		if ((rfp = fopen (new_name, "wt")) != NULL)
			ostream_record = TRUE;
		else
			print_string ("Cannot open file\n");

		return true;
	}

	return false;

}/* record_open */

int replay_open (void)
{
	char new_name[MAX_FILE_NAME + 1];

	if (os_read_file_name (new_name, command_name, FILE_PLAYBACK)) {

		strcpy (command_name, new_name);

		if ((pfp = fopen (new_name, "rt")) != NULL) {

			set_more_prompts (read_yes_or_no ("Do you want MORE prompts"));

			istream_replay = TRUE;

		} else print_string ("Cannot open file\n");

		return true;
	}

	return false;

}/* replay_open */

int script_open (void)
{
	char new_name[MAX_FILE_NAME + 1];

	if (os_read_file_name (new_name, script_name, FILE_SCRIPT)) {

		strcpy (script_name, new_name);

		if ((sfp = fopen (script_name, "at")) != NULL) {

			ostream_script = TRUE;
			script_width = 0;

		} else print_string ("Cannot open file\n");

		return true;
	}
	else
		return false;

}/* script_open */

void hot_key_help (void) {

	print_string ("\nHot key -- Help\n");

	//Had to change to Ctrl-V and Ctrl-I as they conflict with other EPOC keys
	print_string (
		"Ctrl-V  show copyright and license info\n"
		"Ctrl-I  help\n"
		"Ctrl-P  playback on\n"
		"Ctrl-R  recording on/off\n"
		"Ctrl-S  seed random numbers\n"
		"Ctrl-T  transcription on/off\n"
		"Ctrl-U  undo last turn\n"
		"Ctrl-X  exit game\n");

}/* hot_key_help */

void hot_key_info (void) {

	print_string ("\nHot key -- Copyright and License Information\n");

	print_string ("\nMagnetic v2.0, an interpreter for Magnetic Scrolls games.\n");
	print_string ("Copyright (C) 1997-2000 Niclas Karlsson.\n\n");

	print_string ("Magnetic is released under the terms of the GNU General Public License.\n");
	print_string ("See the file COPYING that is included with this program for details.\n");
	print_string ("Magnetic was written by Niclas Karlsson, David Kinder, Stefan Meier and ");
	print_string ("Paul David Doherty. This EPOC version is by Simon Quinn.\n");
	print_string ("\n");
}/* hot_key_info */

void hot_key_playback (void)
{
	if (!istream_replay)
		if(replay_open ())
			print_string ("\nHot key -- Playback on\n");

}/* hot_key_playback */

void hot_key_recording (void)
{

	if (istream_replay) {
		print_string ("\nHot key -- Playback off\n");
		replay_close ();
	} else if (ostream_record) {
		print_string ("\nHot key -- Recording off\n");
		record_close ();
	} else {
		if(record_open ())
			print_string ("\nHot key -- Recording on\n");
	}

}/* hot_key_recording */

void hot_key_seed (void)
{

	 print_string ("\nHot key -- Seed random numbers\n");

	 print_string ("Enter seed value (or return to randomize): ");
	 ms_seed (read_number ());

}/* hot_key_seed */

void hot_key_quit (void)
{

	print_string ("\nHot key -- Exit game\n");

	if (read_yes_or_no ("Do you wish to quit"))
	{	
		delete iConsole;
		CloseSTDLIB();
		exit(0);
	}

}/* hot_key_quit */

void hot_key_script (void)
{

	if (ostream_script) {

		script_new_line ();
		script_word ("*** end of transcription ***");
		script_new_line ();

		script_close ();

		print_string ("\nHot key -- Transcription off\n");

	} else {

		if(script_open ())
		{
			print_string ("\nHot key -- Transcription on\n");
			script_new_line ();
			script_word ("*** start of transcription ***");
			script_new_line ();
		}

	}

}/* hot_key_script */

void hot_key_undo (void)
{

	print_string ("\nHot key -- Undo turn");

}/* hot_key_undo */

void handle_hot_key (char key)
{
	if(key == ZC_HKEY_RECORD) 
		hot_key_recording();
	if(key == ZC_HKEY_PLAYBACK) 
		hot_key_playback();
	if(key == ZC_HKEY_SEED) 
		hot_key_seed();
	if(key == ZC_HKEY_HELP) 
		hot_key_help();
	if(key == ZC_HKEY_QUIT) 
		hot_key_quit();
	if(key == ZC_HKEY_SCRIPT) 
		hot_key_script();
	if(key == ZC_HKEY_UNDO) 
		hot_key_undo();
	if(key == ZC_HKEY_INFO) 
		hot_key_info();

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
		else
		{
			if (ostream_screen)
			{
				if (bufpos2 != 0)
					bufpos2--;
			}
			if (ostream_script)
			{
				if (scrbufpos != 0)
            	scrbufpos--;
			}
		}

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
	static int bufpos = 0;

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


type8 ms_load_file (type8s *name, type8 *ptr, type16 size)
{
	char file_name[MAX_FILE_NAME + 1];
	FILE *fp;
	int result;

//	if (name == NULL) {

		if (!os_read_file_name (file_name, save_name, FILE_RESTORE))
			return 1;

		strcpy (save_name, file_name);

//	} else strcpy (save_name, (const char *) name);

	fp = fopen (save_name, "rb");

	if (fp == NULL)
		return 1;

	result = fread (ptr, size, 1, fp);

	fclose (fp);

	return (result != 0) ? 0 : 1;

}/* ms_load_file */

type8 ms_save_file (type8s *name, type8 *ptr, type16 size)
{
	char file_name[MAX_FILE_NAME + 1];
	FILE *fp;
	int result;

//	if (name == NULL) {

		if (!os_read_file_name (file_name, save_name, FILE_SAVE))
			return 1;

		strcpy (save_name, file_name);

//	} else strcpy (save_name, (const char *) name);

	fp = fopen (save_name, "wb");

	if (fp == NULL)
		return 1;

	result = fwrite (ptr, size, 1, fp);

	fclose (fp);

	return (result != 0) ? 0 : 1;

}/* ms_save_file */


void ms_statuschar (type8 c)
{
	static int saved_x = 0;
	static int saved_y = 0;

	static int count = 0;

	char tempstr[128];

	if (count == 0) {

		//Print out reverse characters for the status line

		iConsole->SetAttr(ATT_INVERSE);

		saved_x = cursor_x;
		saved_y = cursor_y;

		cursor_x = 0;
		cursor_y = 0;

		os_display_char(' ');

		count++; 

	}

	if (c == '\n') {

		//Create a string of spaces and display it
		int templen = (screen_width - (count + 11));
		if(templen > 0)
		{
	    	memset(tempstr, ' ', templen);
			tempstr[templen] = 0;
			os_display_string(tempstr);
			count += templen;
		}

		cursor_x = saved_x;
		cursor_y = saved_y;

		count = 0;

		iConsole->SetAttr(ATT_NORMAL);

	} 
	else if (c == '\t') 
	{
		char tempstr[128];

		int templen = (screen_width - (count + 11));
		if(templen > 0)
		{
	    	memset(tempstr, ' ', templen);
			tempstr[templen] = 0;
			os_display_string(tempstr);
			count += templen;
		}

	} 
	else
	{ 
		os_display_char(c);
		count++; 
	}

}/* ms_statuschar */


void ms_showpic (type32 c, type8 mode)
{
	type8 *picdata;
	type16 Width, Height;
	type8 IsAnim = 0;

	switch (mode)
	{
		case 0:	/* Graphics off */
			break;
		case 1:     /* Graphics on (thumbnails) */
		case 2:	/* Graphics on (normal) */
		{
			picdata = ms_extract(c,&Width,&Height,pic_palette, &IsAnim);
#ifndef __ER6__
			if(iConsole->ScreenSizePixels().iHeight > 400)		//Only draw pics on S7,Netbook,Nokia
#endif	
			{
				if (picdata)
				{
					iConsole->PictureDialog(picdata, TSize(Width, Height), pic_palette);
					iConsole->SetPos(0,0);
					iConsole->Getch();
					iConsole->RestoreScreen();
				}
			}
			break;
		}
	}


}/* ms_showpic */


void ms_fatal(type8s *txt)
{
	TBuf<80> tempdesc;

	tempdesc.Copy(_L8(txt));

	iConsole->Write(_L("\nFatal error: "));
	iConsole->Write(tempdesc);

	ms_status();
	exit(1);
}



int mag_main (int argc, char **argv)
{
	FILE* fp;

	os_process_arguments (argc, argv);

	//Display Title if it exists
	//First check to see if the file is there and readable, display it here
	//so user can look at picture whilst system initialises.
#ifndef __ER6__
	if(iConsole->ScreenSizePixels().iHeight > 400)		//Only draw pics on S7,Netbook,Nokia
#endif	
	{	
		fp = fopen (title_name, "rb");
		if (fp != NULL)
		{
			fclose(fp);
			TBuf<256> sqstr;
			sqstr.Copy(_L8(title_name));
			iConsole->PictureTitle(sqstr);
			iConsole->SetPos(0,0);
		}
	}

	os_init_screen ();

	if (ms_init ((signed char *)story_name, (signed char *)gfx_name) == 0)
	{
		iConsole->Write(_L("Couldn't start up game"));
		exit(1);
	}

	//Will wait for key press if title was available above, this allows the system to initialise
	//whilst the picture is displayed
#ifndef __ER6__
	if(iConsole->ScreenSizePixels().iHeight > 400)		//Only draw pics on S7,Netbook,Nokia
#endif	
	{
		if (fp != NULL)		
			iConsole->Getch();
		iConsole->ClearGraphicScreen();			//Clear graphics screen as os_erase_area doesn't clear it all
	}

	os_erase_area (0, 0, screen_height - 1, screen_width - 1);

	while (ms_rungame () != 0);

	ms_freemem();

	return 0;

}/* main */


void ConsoleMainL()
{
	char *argv[2] = {"","                                                                                 "};
	TInt numargs=2;
//	RProcess me;
	TBuf8<128> args8;

	iConsole = new CAdvConsoleClient;

	//Create the title with the opened file
	TBuf<128> tempname=_L("Magnetic ");

	((CConsoleBase *)iConsole)->Create(tempname, TSize(80, 25));

	TBuf<256> filename;

	filename.Copy(_L("D:\\Magnetic\\"));

	//Check to see if the D: drive exists
	RFs aFs;
	aFs.Connect();
	TDriveInfo driveInfo;
	aFs.Drive(driveInfo, EDriveD);
	aFs.Close();
	if(driveInfo.iType == EMediaNotPresent)
		filename.Copy(_L("C:\\Magnetic\\"));
	

	iConsole->OpenDialog(filename);

	if(filename.Length() > 0)
	{
		args8.Copy(filename);
		args8.ZeroTerminate();
		argv[1] = (char *) args8.Ptr();

		mag_main(numargs, argv);
	}

	delete iConsole;
	CloseSTDLIB();
}

GLDEF_C TInt E32Main()
{
	__UHEAP_MARK;
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	TRAPD(error, ConsoleMainL());
	__ASSERT_ALWAYS(!error, User::Panic(_L("PEP"),error));
	delete cleanupStack;
	__UHEAP_MARKEND;
	return 0;
}


/* These functions are needed only when program is a DLL, which it is
 * under WINS.
 */
#if defined(__WINS__)
EXPORT_C TInt WinsMain(TAny *p)
{
//    RealCommandLine.Set(*(TDesC *) p);
    //process.CommandLine() = _L("-U Reebo");     // This doesn't work because CommandLine returns a copy.
    return E32Main();
}

GLDEF_C TInt E32Dll(TDllReason)
{
    return(KErrNone);
}
#endif
