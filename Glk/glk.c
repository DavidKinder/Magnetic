/*
 * Copyright (C) 2002  Simon Baldwin
 * Mac portions Copyright (C) 2002  Ben Hines
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

/*
 * Glk interface for Magnetic Scrolls 2.1
 * --------------------------------------
 *
 * This module contains the the Glk porting layer for the Magnetic
 * Scrolls interpreter.  It defines the Glk arguments list structure,
 * the entry points for the Glk library framework to use, and all
 * platform-abstracted I/O to link to Glk's I/O.
 *
 * The following items are omitted from this Glk port:
 *
 *  o Glk tries to assert control over _all_ file I/O.  It's just too
 *    disruptive to add it to existing code, so for now, the Magnetic
 *    Scrolls interpreter is still dependent on stdio and the like.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "defs.h"

#include "glk.h"

#if TARGET_OS_MAC
  #include "macglk_startup.h"
#else
  #include "glkstart.h"
#endif


/*---------------------------------------------------------------------*/
/*  Module variables, miscellaneous other stuff                        */
/*---------------------------------------------------------------------*/

/* Glk Magnetic Scrolls port version number. */
static const glui32	MS_GLK_PORT_VERSION		= 0x00000405;

/*
 * We use a maximum of five Glk windows, one for status, one for pictures,
 * two for hints, and one for everything else.  The status and pictures
 * windows may be NULL, depending on user selections and the capabilities
 * of the Glk library.  The hints windows will normally be NULL, except
 * when in the hints subsystem.
 */
static winid_t		ms_glk_main_window		= NULL;
static winid_t		ms_glk_status_window		= NULL;
static winid_t		ms_glk_graphics_window		= NULL;
static winid_t		ms_glk_hint_menu_window		= NULL;
static winid_t		ms_glk_hint_text_window		= NULL;

/*
 * Transcript stream and input log.  These are NULL if there is no
 * current collection of these strings.
 */
static strid_t		ms_glk_transcript_stream	= NULL;
static strid_t		ms_glk_inputlog_stream		= NULL;

/* Input read log stream, for reading back an input log. */
static strid_t		ms_glk_readlog_stream		= NULL;

/* Note about whether graphics is possible, or not. */
static int		ms_glk_graphics_possible	= TRUE;

/* Options that may be turned off by command line flags. */
static int		ms_glk_graphics_enabled		= TRUE;
static int		ms_glk_gamma_enabled		= TRUE;
static int		ms_glk_abbreviations_enabled	= TRUE;
static int		ms_glk_commands_enabled		= TRUE;

/* Forward declaration of event wait function. */
static void		ms_glk_event_wait (glui32 wait_type, event_t *event);


/*---------------------------------------------------------------------*/
/*  Glk arguments list                                                 */
/*---------------------------------------------------------------------*/

#if !TARGET_OS_MAC
glkunix_argumentlist_t glkunix_arguments[] = {
    { (char *) "-nc", glkunix_arg_NoValue,
	(char *) "-nc        No local handling for Glk special commands" },
    { (char *) "-na", glkunix_arg_NoValue,
	(char *) "-na        Turn off abbreviation expansions" },
    { (char *) "-np", glkunix_arg_NoValue,
	(char *) "-np        Turn off pictures" },
    { (char *) "-ng", glkunix_arg_NoValue,
	(char *) "-ng        Turn off automatic gamma correction on pictures" },
    { (char *) "", glkunix_arg_ValueCanFollow,
	(char *) "filename   game to run" },
{ NULL, glkunix_arg_End, NULL }
};
#endif


/*---------------------------------------------------------------------*/
/*  Glk port utility functions                                         */
/*---------------------------------------------------------------------*/


/*
 * ms_glk_malloc()
 *
 * Non-failing malloc; calls ms_fatal if memory allocation fails.
 */
static void *
ms_glk_malloc (int size)
{
	void		*pointer;		/* Return value pointer. */

	/* Malloc, and call ms_fatal if the malloc fails. */
	pointer = malloc (size);
	if (pointer == NULL)
		ms_fatal ("Out of system memory");

	/* Return the allocated pointer. */
	return pointer;
}


/*---------------------------------------------------------------------*/
/*  Glk port CRC functions                                             */
/*---------------------------------------------------------------------*/

/* CRC table initialization magic number, and all-ones value. */
static const glui32	MS_GLK_CRC_MAGIC		= 0xEDB88320;
static const glui32	MS_GLK_CRC_ALLONES		= 0xFFFFFFFF;


/*
 * ms_glk_update_crc()
 *
 * Update a running CRC with the bytes buffer[0..length-1] -- the CRC should
 * be initialized to all 1's, and the transmitted value is the 1's complement
 * of the final running CRC.
 *
 * This algorithm is taken from the PNG specification, version 1.0.
 */
static glui32
ms_glk_update_crc (glui32 crc, const unsigned char *buffer, int length)
{
	static int	initialized	= FALSE;/* First call flag. */
	static glui32	crc_table[UCHAR_MAX+1];	/* CRC table for all bytes. */

	glui32		c;			/* Updated CRC, for return. */
	int		index;			/* Buffer index. */

	/* Build the CRC lookup table if this is the first call. */
	if (!initialized)
	    {
		int	n, k;			/* General loop indexes. */

		/* Create a table entry for each byte value. */
		for (n = 0; n < UCHAR_MAX + 1; n++)
		    {
			c = (glui32) n;
			for (k = 0; k < CHAR_BIT; k++)
			    {
				if (c & 1)
					c = MS_GLK_CRC_MAGIC ^ (c >> 1);
				else
					c = c >> 1;
			    }
			crc_table[ n ] = c;
		    }

		/* Note the table is built. */
		initialized = TRUE;
	    }

	/* Update the CRC with each character in the buffer. */
	c = crc;
	for (index = 0; index < length; index++)
		c = crc_table[ (c ^ buffer[ index ]) & UCHAR_MAX ]
							^ (c >> CHAR_BIT);

	/* Return the updated CRC. */
	return c;
}


/*
 * ms_glk_buffer_crc()
 *
 * Return the CRC of the bytes buffer[0..length-1].
 */
static glui32
ms_glk_buffer_crc (const unsigned char *buffer, int length)
{
	glui32		c;			/* CRC intermediate value. */

	/* Calculate and return the CRC. */
	c = ms_glk_update_crc (MS_GLK_CRC_ALLONES, buffer, length);
	return c ^ MS_GLK_CRC_ALLONES;
}


/*---------------------------------------------------------------------*/
/*  Glk port picture functions                                         */
/*---------------------------------------------------------------------*/

/* R,G,B colour triple definition. */
typedef struct {
	int	red, green, blue;		/* Colour attributes. */
} ms_glk_rgb_t;

/*
 * Colour conversions lookup tables, and a word about gamma corrections.
 *
 * When uncorrected, some game pictures can look dark (Corruption, Won-
 * derland), whereas others look just fine (Guild Of Thieves, Jinxter).
 *
 * The standard general-purpose gamma correction is around 2.1, with
 * specific values, normally, of 2.5-2.7 for IBM PC systems, and 1.8 for
 * Macintosh.  However, applying even the low end of this range can
 * make some pictures look washed out, yet improve others nicely.
 *
 * To try to solve this, here we'll offer a range of precalculated tables
 * with discrete gamma values.  On displaying a picture, we'll try to
 * find a gamma correction that seems to offer a reasonable level of
 * contrast for the picture.  In practice, we can actually allow some
 * gamma values a little less than 1.0, too, to even out a few pictures
 * that might be considered a bit over-bright.
 *
 * Here's an AWK script to create the gamma table:
 *
 * BEGIN { max=255.0; step=max/7.0
 *         for (gamma=0.1; gamma<4.0; gamma+=0.1) {
 *             printf "{\"%2.1f\",{ 0,", gamma
 *             for (i=1; i<8; i++)
 *                 printf "%3.0f,", (((step*i / max) ^ (1.0/gamma)) * max)
 *             printf "255,255,255,255,255,255,255,255 }"
 *             printf ",%s},\n", (gamma>0.95 && gamma<1.05) ? "TRUE" : "FALSE"
 *         } }
 *
 * TODO Why is colour conversion based on 3 bits a good idea?  Base image
 * colours are noted in defs.h as being in the range 0-F, so 4 bits seems
 * more likely.  But other ports, notably Windows and Amiga, scale as if
 * colours were 0-7.  Just in case, here the table allows for 4 bits, but
 * the displayed colours will be wrong if this happens.
 */
typedef const struct {
	const char     		*level;		/* Gamma correction level. */
	const unsigned char	table[16];	/* Colour lookup table. */
	const int      		corrected;	/* Flag if non-linear. */
} ms_glk_gamma_entry_t;
static ms_glk_gamma_entry_t	MS_GLK_GAMMA_TABLE[] = {
#if 0
{"0.1",{ 0,  0,  0,  0,  1,  9, 55,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.2",{ 0,  0,  0,  4, 16, 47,118,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.3",{ 0,  0,  4, 15, 39, 83,153,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.4",{ 0,  2, 11, 31, 63,110,173,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.5",{ 0,  5, 21, 47, 83,130,187,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.6",{ 0, 10, 32, 62,100,146,197,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.7",{ 0, 16, 43, 76,115,158,205,255,255,255,255,255,255,255,255,255 },FALSE},
#endif
{"0.8",{ 0, 22, 53, 88,127,167,210,255,255,255,255,255,255,255,255,255 },FALSE},
{"0.9",{ 0, 29, 63, 99,137,175,215,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.0",{ 0, 36, 73,109,146,182,219,255,255,255,255,255,255,255,255,255 },TRUE},
{"1.1",{ 0, 43, 82,118,153,188,222,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.2",{ 0, 50, 90,126,160,193,224,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.3",{ 0, 57, 97,133,166,197,226,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.4",{ 0, 64,104,139,171,201,228,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.5",{ 0, 70,111,145,176,204,230,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.6",{ 0, 76,117,150,180,207,232,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.7",{ 0, 81,122,155,183,209,233,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.8",{ 0, 87,127,159,187,212,234,255,255,255,255,255,255,255,255,255 },FALSE},
{"1.9",{ 0, 92,132,163,190,214,235,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.0",{ 0, 96,136,167,193,216,236,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.1",{ 0,101,140,170,195,217,237,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.2",{ 0,105,144,173,198,219,238,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.3",{ 0,109,148,176,200,220,238,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.4",{ 0,113,151,179,202,222,239,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.5",{ 0,117,154,182,204,223,240,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.6",{ 0,121,158,184,206,224,240,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.7",{ 0,124,160,186,207,225,241,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.8",{ 0,127,163,188,209,226,241,255,255,255,255,255,255,255,255,255 },FALSE},
{"2.9",{ 0,130,166,190,210,227,242,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.0",{ 0,133,168,192,212,228,242,255,255,255,255,255,255,255,255,255 },FALSE},
#if 0
{"3.1",{ 0,136,170,194,213,229,243,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.2",{ 0,139,172,196,214,230,243,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.3",{ 0,141,174,197,215,230,243,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.4",{ 0,144,176,199,216,231,244,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.5",{ 0,146,178,200,217,232,244,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.6",{ 0,149,180,202,218,232,244,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.7",{ 0,151,182,203,219,233,245,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.8",{ 0,153,183,204,220,233,245,255,255,255,255,255,255,255,255,255 },FALSE},
{"3.9",{ 0,155,185,205,221,234,245,255,255,255,255,255,255,255,255,255 },FALSE},
#endif
{NULL, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },FALSE}
};

/*
 * Weighting values for calculating the luminance of a colour.  There are
 * two commonly used sets of values for these -- 299,587,114, taken from
 * NTSC (Never The Same Color) 1953 standards, and 212,716,72, which is the
 * set that modern CRTs tend to match.  The NTSC ones seem to give the best
 * subjective results.
 */
static ms_glk_rgb_t	MS_GLK_LUMINANCE_WEIGHTS	= { 299, 587, 114 };

/*
 * Maximum number of regions to consider in a single repaint pass.  A
 * couple of hundred seems to strike the right balance between not too
 * sluggardly picture updates, and responsiveness to input during
 * graphics rendering, when combined with short timeouts.
 */
static const int	MS_GLK_REPAINT_LIMIT		= 256;

/*
 * Graphics timeout; we like an update call after this period (ms).  In
 * practice, this timeout may actually be shorter than the time taken
 * to reach the limit on repaint regions, but because Glk guarantees that
 * user interactions (in this case, line events) take precedence over
 * timeouts, this should be okay; we'll still see a game that responds
 * to input each time the background repaint function yields.
 */
static const int	MS_GLK_GRAPHICS_TIMEOUT		= 10;

/* Pixel size multiplier for image size scaling. */
static const int	MS_GLK_GRAPHICS_PIXEL		= 2;

/* Proportion of the display to use for graphics. */
static const int	MS_GLK_GRAPHICS_PROPORTION	= 60;

/*
 * The current picture bitmap being displayed, its width, height, palette,
 * and animation flag.
 */
#define	MS_GLK_PALETTE_SIZE				16
static type8		*ms_glk_graphics_bitmap		= NULL;
static type16		ms_glk_graphics_width		= 0;
static type16		ms_glk_graphics_height		= 0;
static type16		ms_glk_graphics_palette[MS_GLK_PALETTE_SIZE];
						/*	= { 0, ... }; */
static type8		ms_glk_graphics_animated	= FALSE;

/*
 * Flags set on new picture and on resize or arrange events, and a state
 * value to indicate whether background repaint is stopped or active.
 */
static int		ms_glk_graphics_new_picture	= FALSE;
static int		ms_glk_graphics_repaint		= FALSE;
static int		ms_glk_graphics_active		= FALSE;

/* Flag to try to monitor the state of interpreter graphics. */
static int		ms_glk_graphics_interpreter	= FALSE;

/*
 * Pointer to any temporary graphics malloc'ed memory.  This is memory
 * that, ideally, should be free'd on exit.
 */
static type8		*ms_glk_graphics_temporary	= NULL;

/*
 * Pointer to the current active gamma table entry.  Because of the way
 * it's queried, this may not be NULL, otherwise we risk a race with the
 * updater.  So, it's initialized instead to the gamma table.  The real
 * value in use is inserted on the first picture update timeout call for
 * a new picture.
 */
static ms_glk_gamma_entry_t
			*ms_glk_current_gamma		= MS_GLK_GAMMA_TABLE;


/*
 * ms_glk_graphics_open()
 *
 * If it's not open, open the graphics window.  Returns TRUE if graphics
 * was successfully started, or already on.
 */
static int
ms_glk_graphics_open (void)
{
	/* If graphics are off, turn them back on now. */
	if (ms_glk_graphics_window == NULL)
	    {
		/* (Re-)open a graphics window. */
		ms_glk_graphics_window = glk_window_open
				(ms_glk_main_window,
				winmethod_Above | winmethod_Proportional,
				MS_GLK_GRAPHICS_PROPORTION,
				wintype_Graphics, 0);

		/* If the graphics fails, return FALSE. */
		if (ms_glk_graphics_window == NULL)
			return FALSE;
	    }

	/* Graphics opened successfully, or already open. */
	return TRUE;
}


/*
 * ms_glk_graphics_close()
 *
 * If it is open, close the graphics window.
 */
static void
ms_glk_graphics_close (void)
{
	/* If the graphics window is open, close it and set NULL. */
	if (ms_glk_graphics_window != NULL)
	    {
		glk_window_close (ms_glk_graphics_window, NULL);
		ms_glk_graphics_window = NULL;
	    }
}


/*
 * ms_glk_graphics_displayed()
 *
 * Return TRUE if graphics are currently being displayed, FALSE otherwise.
 */
static int
ms_glk_graphics_displayed (void)
{
	return (ms_glk_graphics_window != NULL);
}


/*
 * ms_glk_graphics_start()
 *
 * Start any background picture update processing.
 */
static void
ms_glk_graphics_start (void)
{
	/* Ignore the call if pictures are disabled. */
	if (!ms_glk_graphics_enabled)
		return;

	/* If not running, start the updating "thread". */
	if (!ms_glk_graphics_active)
	    {
		glk_request_timer_events (MS_GLK_GRAPHICS_TIMEOUT);
		ms_glk_graphics_active = TRUE;
	    }
}


/*
 * ms_glk_graphics_stop()
 *
 * Stop any background picture update processing.
 */
static void
ms_glk_graphics_stop (void)
{
	/* If running, stop the updating "thread". */
	if (ms_glk_graphics_active)
	    {
		glk_request_timer_events (0);
		ms_glk_graphics_active = FALSE;
	    }
}


/*
 * ms_glk_graphics_picture_available()
 *
 * Return TRUE if the graphics module data is loaded with a usable picture,
 * FALSE if there is no picture available to display.
 */
static int
ms_glk_graphics_picture_available (void)
{
	return (ms_glk_graphics_bitmap != NULL);
}


/*
 * ms_glk_graphics_picture_details()
 *
 * Return the width, height, and number of colours of the current loaded
 * picture.  The function returns zeroes if no picture is loaded.
 */
static void
ms_glk_graphics_picture_details (int *width, int *height, int *colours)
{
	if (width != NULL)
		*width	= ms_glk_graphics_width;
	if (height != NULL)
		*height	= ms_glk_graphics_height;
	if (colours != NULL)
		*colours= MS_GLK_PALETTE_SIZE;
}


/*
 * ms_glk_graphics_interpreter_enabled()
 *
 * Return TRUE if it looks like interpreter graphics are turned on, FALSE
 * otherwise.
 */
static int
ms_glk_graphics_interpreter_enabled (void)
{
	return ms_glk_graphics_interpreter;
}


/*
 * ms_glk_graphics_gamma_correction()
 *
 * Returns the current level of applied gamma correction, as a string.
 */
static const char *
ms_glk_graphics_gamma_correction (void)
{
	assert (ms_glk_current_gamma != NULL);

	/* Return the string representing the gamma correction. */
	return ms_glk_current_gamma->level;
}


/*
 * ms_glk_graphics_paint()
 *
 * Set up a complete repaint of the current picture in the graphics window.
 * This function should be called on the appropriate Glk window resize and
 * arrange events.
 */
static void
ms_glk_graphics_paint (void)
{
	/*
	 * Ignore the call if pictures are disabled, or if there is no
	 * graphics window currently displayed.
	 */
	if (!ms_glk_graphics_enabled
			|| !ms_glk_graphics_displayed ())
		return;

	/* Set the repaint flag, and start graphics. */
	ms_glk_graphics_repaint = TRUE;
	ms_glk_graphics_start ();
}


/*
 * ms_glk_graphics_restart()
 *
 * Restart graphics as if the current picture is a new picture.  This
 * This function should be called whenever graphics is re-enabled after
 * being disabled, or on change of gamma colour correction policy.
 */
static void
ms_glk_graphics_restart (void)
{
	/*
	 * Ignore the call if pictures are disabled, or if there is no
	 * graphics window currently displayed.
	 */
	if (!ms_glk_graphics_enabled
			|| !ms_glk_graphics_displayed ())
		return;

	/* Set the new picture flag, and start graphics. */
	ms_glk_graphics_new_picture = TRUE;
	ms_glk_graphics_start ();
}


/*
 * ms_glk_graphics_swap_array()
 *
 * Swap two elements of an integer array.
 */
static void
ms_glk_graphics_swap_array (int int_array[], int index_a, int index_b)
{
	int	temporary;			/* Temporary swap value */

	/* Swap values. */
	temporary		= int_array[ index_a ];
	int_array[ index_a ]	= int_array[ index_b ];
	int_array[ index_b ]	= temporary;
}


/*
 * ms_glk_graphics_linear_gamma()
 *
 * Return the gamma table entry for the linear (no correction) mapping.
 */
static ms_glk_gamma_entry_t *
ms_glk_graphics_linear_gamma (void)
{
	static int	initialized	= FALSE;/* First call flag. */
	static ms_glk_gamma_entry_t
			*linear_gamma;		/* Linear table entry */

	ms_glk_gamma_entry_t
			*entry;			/* Table iterator */

	/* On first call, find and cache the uncorrected table entry. */
	if (!initialized)
	    {
		/* Scan the table for an uncorrected entry. */
		linear_gamma = NULL;
		for (entry = MS_GLK_GAMMA_TABLE; entry->level != NULL; entry++)
		    {
			if (!entry->corrected)
			    {
				linear_gamma = entry;
				break;
			    }
		    }

		/* There must always be a linear table entry. */
		assert (linear_gamma != NULL);

		/* Set initialized flag. */
		initialized = TRUE;
	    }

	/* Return the cached linear gamma table entry. */
	return linear_gamma;
}


/*
 * ms_glk_graphics_auto_gamma()
 *
 * Given a palette, try to select a suitable gamma correction for it that
 * gives relatively good contrast.
 *
 * This function searches the gamma tables, computing colour luminance for
 * each colour in the palette given this gamma.  From luminances, it then
 * computes the contrasts between the colours, and settles on the gamma
 * correction that gives the most even and well-distributed picture
 * contrast.
 *
 * Note that the function doesn't consider how often (if at all) a palette
 * colour is used.  Some weighting might improve things.  However, the
 * simple method seems to work adequately.  In practice, since there are
 * only 16 colours in a palette, most pictures use most colours in a
 * relatively well distributed manner.  This function probably wouldn't
 * work well on real photographs, though.
 */
static ms_glk_gamma_entry_t *
ms_glk_graphics_auto_gamma (type16 palette[])
{
	int			index;		/* Array iterator */
	ms_glk_gamma_entry_t	*entry, *result;/* Table iterator, result */
	int			lowest_variance;/* Lowest contrast variance */
	assert (palette != NULL);

	/* Initialize the search result, and lowest contrast variance found. */
	result		= NULL;
	lowest_variance	= INT_MAX;

	/* Iterate over the gamma corrections table. */
	for (entry = MS_GLK_GAMMA_TABLE; entry->level != NULL; entry++)
	    {
		const unsigned char
				*color_table;	/* Gamma colour table */
		int		luminance[MS_GLK_PALETTE_SIZE];
						/* Luminance for each colour */
		int		sorted;		/* Array sorted flag */
		int		contrast[MS_GLK_PALETTE_SIZE - 1];
						/* Contrast between colours */
		int		sum, mean;	/* Contrast sum and mean */
		int		variance;	/* Variance in contrast */

		/* Set the colour table for this entry. */
		color_table = entry->table;

		/* Calculate the energy of each colour at this gamma. */
		for (index = 0; index < MS_GLK_PALETTE_SIZE; index++)
		    {
			type16	color;		/* Raw picture colour */
			ms_glk_rgb_t
				gamma_color;	/* Gamma-corrected colour */

			/* Find the 16-bit base picture colour. */
			color = palette[ index ];

			/*
			 * Split this colour into components, and gamma-
			 * correct and rescale to 0-255 for each, according
			 * to the gamma table entry being considered.
			 */
			gamma_color.red   = color_table[ (color & 0xF00) >> 8 ];
			gamma_color.green = color_table[ (color & 0x0F0) >> 4 ];
			gamma_color.blue  = color_table[ (color & 0x00F)      ];

			/*
			 * Calculate luminance for this colour, by weighting
			 * the three components.  The weightings apply a
			 * scaling of 1000 to the colour brightnesses, so
			 * divide by the weighting sum to get a luminance in
			 * the range 0-255.
			 */
			luminance[ index ] =
			  ( gamma_color.red   * MS_GLK_LUMINANCE_WEIGHTS.red
			  + gamma_color.green * MS_GLK_LUMINANCE_WEIGHTS.green
			  + gamma_color.blue  * MS_GLK_LUMINANCE_WEIGHTS.blue  )
			 / ( MS_GLK_LUMINANCE_WEIGHTS.red
			   + MS_GLK_LUMINANCE_WEIGHTS.green
			   + MS_GLK_LUMINANCE_WEIGHTS.blue  );
		    }

		/*
		 * Bubble-sort luminance values into inverse order, so that
		 * the darkest colour is at index 0.
		 */
		do
		    {
			sorted = TRUE;
			for (index = 0;
				index < MS_GLK_PALETTE_SIZE - 1; index++)
			    {
				if (luminance[ index + 1 ] < luminance[ index ])
				    {
					ms_glk_graphics_swap_array
						(luminance, index, index + 1);

					/* Flag as not yet sorted. */
					sorted = FALSE;
				    }
			    }
		    }
		while (!sorted);

		/*
		 * Calculate the difference in luminance between adjacent
		 * luminances in the sorted array.
		 */
		for (index = 0; index < MS_GLK_PALETTE_SIZE - 1; index++)
			contrast[ index ] = luminance[ index + 1 ]
						- luminance[ index ];

		/* Now find the mean contrast. */
		sum = 0;
		for (index = 0; index < MS_GLK_PALETTE_SIZE - 1; index++)
			sum += contrast[ index ];
		mean = sum / (MS_GLK_PALETTE_SIZE - 1);

		/* From the mean, find the variance in contrasts. */
		sum = 0;
		for (index = 0; index < MS_GLK_PALETTE_SIZE - 1; index++)
		    {
			int	delta;		/* Contrast difference */

			/* Sum the square of contrast differences from mean */
			delta	= contrast[ index ] - mean;
			sum	+= delta * delta;
		    }
		variance = sum / (MS_GLK_PALETTE_SIZE - 1);

		/*
		 * Compare the variance to the lowest so far, and if it is
		 * lower, note the gamma entry that produced it as being
		 * the current best found.
		 */
		if (variance < lowest_variance)
		    {
			result		= entry;
			lowest_variance	= variance;
		    }
	    }

	/*
	 * Return the gamma table entry that produced the most even
	 * picture contrast.
	 */
	assert (result != NULL);
	return result;
}


/*
 * ms_showpic()
 *
 * Called by the main interpreter when it wants us to display a picture.
 * The function gets the picture bitmap, palette, and dimensions, and
 * saves them in module variables for the background rendering function.
 *
 * The graphics window is opened if required, or closed if mode is zero.
 *
 * The function checks for changes of actual picture by calculating the
 * CRC for picture data; this helps to prevent unnecessary repaints in
 * cases where the interpreter passes us the same picture as we're already
 * displaying.  There is a less than 1 in 4,294,967,296 chance that a new
 * picture will be missed.  We'll live with that.
 *
 * Why use CRCs, rather than simply storing the values of picture passed in
 * a static variable?  Because some games, typically Magnetic Scrolls, use
 * the picture argument as a form of string pointer, and can pass in the
 * same value for several, perhaps all, game pictures.  If we just checked
 * for a change in the picture argument, we'd never see one.  So we must
 * instead look for changes in the real picture data.
 */
void
ms_showpic (type32 picture, type8 mode)
{
	static glui32	current_crc	= 0;	/* CRC of the current picture */

	type8		*bitmap;		/* New picture bitmap */
	type16		width, height;		/* New picture dimensions */
	type16		palette[MS_GLK_PALETTE_SIZE];
						/* New picture palette */
	type8		animated;		/* New picture animation */
	glui32		crc;			/* New picture's CRC */
	type16		old_width, old_height;	/* Old picture dimensions */

	/* See if the mode indicates no graphics. */
	if (mode == 0)
	    {
		/*
		 * If we are currently displaying the graphics window, stop
		 * any update "thread" and turn off graphics.
		 */
		if (ms_glk_graphics_enabled
				&& ms_glk_graphics_displayed ())
		    {
			ms_glk_graphics_stop ();
			ms_glk_graphics_close ();
		    }

		/* Note that the interpreter turned graphics off. */
		ms_glk_graphics_interpreter = FALSE;

		/* Nothing more to do now graphics are off. */
		return;
	    }

	/* Note that the interpreter turned graphics on. */
	ms_glk_graphics_interpreter = TRUE;

	/*
	 * Obtain the image details for the requested picture.  The call
	 * returns NULL if there's a problem with the picture.
	 */
	bitmap = ms_extract (picture, &width, &height, palette, &animated);
	if (bitmap == NULL)
		return;

	/* Calculate the CRC for the bitmap data. */
	crc = ms_glk_buffer_crc (bitmap, width * height);

	/*
	 * If there is no change of picture, we might be able to largely
	 * ignore the call.  Check for a change.
	 */
	if (width == ms_glk_graphics_width
				&& height == ms_glk_graphics_height
				&& crc == current_crc)
	    {
		/*
		 * Even when no picture change, force repaint when
		 * graphics are turned back on.
		 */
		if (ms_glk_graphics_enabled
				&& !ms_glk_graphics_displayed ())
		    {
			/* Ensure graphics are on. */
			if (!ms_glk_graphics_open ())
				return;

			/* Set repaint flag, start the updating "thread". */
			ms_glk_graphics_repaint = TRUE;
			ms_glk_graphics_start ();
		    }

		/* Nothing more if the picture remains the same. */
		return;
	    }

	/*
	 * We know now that this is a genuine change of picture.  In this
	 * case, record picture details, ensure graphics is on, set the
	 * appropriate repaint and new picture flags, and start the
	 * background graphics update.
	 */

	/* Retain the old picture dimensions, for a check later. */
	old_width	= ms_glk_graphics_width;
	old_height	= ms_glk_graphics_height;

	/* Save the picture details for the update code. */
	ms_glk_graphics_bitmap		= bitmap;
	ms_glk_graphics_width		= width;
	ms_glk_graphics_height		= height;
	memcpy (ms_glk_graphics_palette, palette, sizeof (palette));
	ms_glk_graphics_animated	= animated;

	/* Retain the new picture CRC. */
	current_crc = crc;

	/*
	 * If graphics are enabled, ensure the window is displayed,
	 * set the appropriate flags, and start graphics update.  If
	 * they're not enabled, the picture details will simply stick
	 * around in module variables until they are required.
	 */
	if (ms_glk_graphics_enabled)
	    {
		/* Ensure graphics on, and return (fail) if not possible. */
		if (!ms_glk_graphics_open ())
			return;

		/*
		 * Set the new picture flag.  If the picture dimensions have
		 * changed, also set the repaint flag, to trigger a clear
		 * of the graphics window prior to rendering the new picture.
		 */
		ms_glk_graphics_new_picture = TRUE;
		if (width != old_width
				|| height != old_height)
			ms_glk_graphics_repaint = TRUE;

		/* Start the updating "thread". */
		ms_glk_graphics_start ();
	    }
}


/*
 * ms_glk_graphics_paint_region()
 *
 * This is a partially optimized point plot.  Given a point in the
 * graphics bitmap, it tries to extend the point to a colour region, and
 * fill a number of pixels in a single Glk rectangle fill.  The goal
 * here is to reduce the number of Glk rectangle fills, which tend to be
 * extremely inefficient operations for generalized point plotting.
 *
 * The extension works in image layers; each palette colour is assigned
 * a layer, and we paint each layer individually, starting at the lowest.
 * So, the region is free to fill any unpainted pixel in either the
 * same layer as the given point, or any higher layer.  In practice, it
 * is good enough to simply look for unpainted pixels, and construct
 * a region as large as possible from these, then on marking points as
 * painted, mark only those in the same layer as the initial point.
 *
 * The optimization here is not the best possible, but is reasonable.
 * What we do is to try and stretch the region horizontally first, then
 * vertically.  In practice, we might find larger areas by stretching
 * vertically and then horizontally, or by stretching both dimensions at
 * the same time.  In mitigation, the number of colours in a picture is
 * small (16), and the aspect ratio of pictures makes them generally
 * wider than they are tall.
 *
 * Once we've found the region, we render it with a single Glk rectangle
 * fill, and mark all the pixels in this region that match the layer of
 * the initial given point as painted.
 */
static void
ms_glk_graphics_paint_region (winid_t glk_window,
			glui32 glk_palette[], int glk_layers[], type8 *bitmap,
			int x, int y, int x_offset, int y_offset,
			int pixel_size, type16 width, type16 height,
			type8 *glk_painted_xy)
{
	type8		pixel;			/* Pixel reference colour */
	int		x_min, x_max;		/* X region extent */
	int		y_min, y_max;		/* Y region extent */
	int		x_index, y_index;	/* Region iterator indexes */
	int		stop;			/* Y stretch stop flag */
	assert (glk_palette != NULL && bitmap != NULL);
	assert (glk_layers != NULL && glk_painted_xy != NULL);

	/*
	 * Start by finding the extent to which we can pull the x coordinate
	 * and still find unpainted pixels.
	 */
	for (x_min = x; x_min - 1 >= 0
			&& !glk_painted_xy[ y * width + x_min - 1 ]; )
		x_min--;
	for (x_max = x; x_max + 1 < width
			&& !glk_painted_xy[ y * width + x_max + 1 ]; )
		x_max++;

	/*
	 * Now try to stretch the height of the region, by extending the
	 * y coordinate as much as possible too.  Again, we're looking
	 * for pixels that aren't yet painted.  We need to check across the
	 * full width of the current region.
	 */
	stop = FALSE;
	for (y_min = y; y_min - 1 >= 0 && !stop; )
	    {
		for (x_index = x_min; x_index <= x_max && !stop; x_index++)
		    {
			if (glk_painted_xy[ (y_min - 1) * width + x_index ])
				stop = TRUE;
		    }
		if (!stop)
			y_min--;
	    }
	stop = FALSE;
	for (y_max = y; y_max + 1 < height && !stop; )
	    {
		for (x_index = x_min; x_index <= x_max && !stop; x_index++)
		    {
			if (glk_painted_xy[ (y_max + 1) * width + x_index ])
				stop = TRUE;
		    }
		if (!stop)
			y_max++;
	    }

	/* Find the colour for the initial pixel. */
	pixel = bitmap[ y * width + x ];
	assert (pixel < MS_GLK_PALETTE_SIZE);

	/* Fill the region using Glk's rectangle fill. */
	glk_window_fill_rect (glk_window,
			glk_palette[ pixel ],
			x_min * pixel_size + x_offset,
			y_min * pixel_size + y_offset,
			(x_max - x_min + 1) * pixel_size,
			(y_max - y_min + 1) * pixel_size);

	/*
	 * Update the flag for each pixel in the reference layer that was
	 * rendered by the rectangle fill.  We don't flag pixels that are
	 * not in this layer (and are by definition in higher layers, as
	 * we've flagged all lower layers), since although we coloured
	 * them, we did it for optimization reasons, and they're not yet
	 * coloured correctly.
	 */
	for (y_index = y_min; y_index <= y_max; y_index++)
	    {
		for (x_index = x_min; x_index <= x_max; x_index++)
		    {
			int	index;			/* x,y pixel index */

			/* Get the index for x_index,y_index. */
			index = y_index * width + x_index;

			/* If the pixel layers match, set painted flag. */
			if (glk_layers[ bitmap [ index ]]
						== glk_layers [ pixel ])
			    {
				assert (!glk_painted_xy[ index ]);
				glk_painted_xy[ index ] = TRUE;
			    }
		    }
	    }
}


/*
 * ms_glk_graphics_timeout()
 *
 * This is a background function, called on Glk timeouts.  Its job is
 * to repaint some of the current graphics image.  On successive calls,
 * it does a part of the repaint, then yields to other processing.  This
 * is useful since the Glk primitive to plot points in graphical windows
 * is extremely slow; this way, the repaint happens without blocking
 * game play.
 *
 * The function should be called on Glk timeout events.  When the repaint
 * is complete, the function will turn off Glk timers.
 */
static void
ms_glk_graphics_timeout (void)
{
	static type8	*glk_painted_xy	= NULL;	/* Painted flag for x,y coord */
	static int	painted_xy_size	= 0;	/* Painted flags array length */
	static int	glk_layers[MS_GLK_PALETTE_SIZE];
						/* Assigned images layers */
	static glui32	glk_palette[MS_GLK_PALETTE_SIZE];
						/* Precomputed Glk palette */
	static int	x_offset, y_offset;	/* Point plot offsets */
	static int	saved_layer;		/* Saved current layer */
	static int	saved_x, saved_y;	/* Saved x,y coord */

	int		layer;			/* Image layer iterator */
	int		x, y;			/* Image iterators */
	int		regions;		/* Count of regions painted */

	/* Ignore the call if the current graphics state is inactive. */
	if (!ms_glk_graphics_active)
		return;
	assert (ms_glk_graphics_window != NULL);

	/*
	 * If we received a new picture, set up the local static variables
	 * for that picture - recreate the painted flags if appropriate,
	 * assign colours to image layers for repaint optimizations, decide
	 * on any gamma correction for the picture, and convert the colour
	 * palette.
	 */
	if (ms_glk_graphics_new_picture)
	    {
		int		new_size;	/* New picture size */
		int		index;		/* Palette colour index */
		int		counts[MS_GLK_PALETTE_SIZE];
						/* Colour use counts */
		int		colors[MS_GLK_PALETTE_SIZE];
						/* Temporary colour indexes */
		int		sorted;		/* Layer sort flag */
		ms_glk_gamma_entry_t
				*gamma_entry;	/* Gamma table entry */
		const unsigned char
				*color_table;	/* Gamma colour table */

		/* Does picture size need a new set of paint flags? */
		new_size = ms_glk_graphics_width * ms_glk_graphics_height;
		if (painted_xy_size != new_size)
		    {
			/* Free the current flags set. */
			if (glk_painted_xy != NULL)
				free (glk_painted_xy);

			/* Create a new set of flags. */
			painted_xy_size	= new_size;
			glk_painted_xy	= ms_glk_malloc (painted_xy_size
							* sizeof (type8));

			/*
			 * Note point paint flags as temporary data.  We can
			 * then remember to free this data at the end of a
			 * game.
			 */
			ms_glk_graphics_temporary = glk_painted_xy;
		    }

		/*
		 * Assign image layers, with layers sorted by usage so that
		 * the lowest layer is the most used colour.  This will help
		 * to minimize the amount of Glk area fills that need to
		 * happen for this picture - if we paint lower levels first,
		 * then the paint can take in larger areas if it's permitted
		 * to include as-yet-unpainted higher levels.
		 *
		 * Start by counting colour usage...
		 */
		for (index = 0; index < MS_GLK_PALETTE_SIZE; index++)
			counts[ index ] = 0;
		for (y = 0; y < ms_glk_graphics_height; y++)
		    {
			for (x = 0; x < ms_glk_graphics_width; x++)
			    {
				counts[ ms_glk_graphics_bitmap[
					y * ms_glk_graphics_width + x ]]++;
			    }
		    }

		/* ... now bubble-sort counts to form colour indexes... */
		for (index = 0; index < MS_GLK_PALETTE_SIZE; index++)
			colors[ index ] = index;
		do
		    {
			sorted = TRUE;
			for (index = 0;
				index < MS_GLK_PALETTE_SIZE - 1; index++)
			    {
				if (counts[ index + 1 ] > counts[ index ])
				    {
					/* Swap colours and matching counts. */
					ms_glk_graphics_swap_array
						(colors, index, index + 1);
					ms_glk_graphics_swap_array
						(counts, index, index + 1);

					/* Flag as not yet sorted. */
					sorted = FALSE;
				    }
			    }
		    }
		while (!sorted);

		/* ... and finally assign layers. */
		for (index = 0; index < MS_GLK_PALETTE_SIZE; index++)
			glk_layers[ colors[ index ]] = index;

		/* Select a suitable gamma for the picture. */
		gamma_entry = ms_glk_gamma_enabled ?
			  ms_glk_graphics_auto_gamma (ms_glk_graphics_palette)
			: ms_glk_graphics_linear_gamma ();

		/* Extract the colour table from the current gamma. */
		color_table = gamma_entry->table;

		/*
		 * Now pre-convert all the picture palette colours into their
		 * corresponding Glk colours.  Since there are only a few
		 * real colours, this saves a lot of duplication of effort
		 * later on in the rendering loop.
		 */
		for (index = 0; index < MS_GLK_PALETTE_SIZE; index++)
		    {
			type16	color;		/* Raw picture colour. */

			/* Find the 16-bit base picture colour. */
			color = ms_glk_graphics_palette[ index ];

			/* Convert to a 32-bit Glk colour. */
			glk_palette[ index ] =
			    color_table[ (color & 0xF00) >> 8 ] << 16
			  | color_table[ (color & 0x0F0) >> 4 ] << 8
			  | color_table[ (color & 0x00F)      ];
		    }

		/*
		 * Finally, save the selected gamma entry so that it can
		 * be queried by others.
		 */
		ms_glk_current_gamma = gamma_entry;
	    }

	/*
	 * If the repaint flag is set, then we need to clear the window,
	 * to avoid leaving pieces of the last picture lying about.
	 */
	if (ms_glk_graphics_repaint)
	    {
		/* Clear the graphics window. */
		glk_window_clear (ms_glk_graphics_window);
	    }

	/*
	 * For either a new picture or a repaint of a prior one, calculate
	 * new values for the x and y offsets used to draw image points,
	 * and clear the pixel paint flags.  Also, reset the saved image
	 * scan coordinates so that we scan for unpainted pixels from top
	 * left starting at layer zero.
	 */
	if (ms_glk_graphics_new_picture
			|| ms_glk_graphics_repaint)
	    {
		glui32		width, height;	/* Graphics window dimensions */
		int		index;		/* Painted flags index */

		/*
		 * Measure the current graphics window dimensions, and calcu-
		 * late an x and y offset to use on point plotting, so that
		 * the image centers inside the graphical window.
		 */
		glk_window_get_size (ms_glk_graphics_window, &width, &height);
		x_offset = ((int) width
			- ms_glk_graphics_width  * MS_GLK_GRAPHICS_PIXEL) / 2;
		y_offset = ((int) height
			- ms_glk_graphics_height * MS_GLK_GRAPHICS_PIXEL) / 2;

		/* Clear all paint flags. */
		for (index = 0; index < painted_xy_size; index++)
			glk_painted_xy[ index ] = FALSE;

		/* Start a fresh picture rendering pass. */
		saved_layer	= 0;
		saved_x		= 0;
		saved_y		= 0;

		/* Clear the new picture and repaint flags. */
		ms_glk_graphics_new_picture	= FALSE;
		ms_glk_graphics_repaint		= FALSE;
	    }

	/*
	 * Make a portion of an image pass, from lower to higher image layers,
	 * scanning for unpainted pixels that are in the current image layer
	 * we are painting.  Each unpainted pixel gives rise to a region
	 * paint, which equates to one Glk rectangle fill.
	 *
	 * When the limit on regions is reached, save the current image pass
	 * layer and coordinates, and yield control to the main game playing
	 * code by returning.  On the next call, pick up where we left off.
	 */
	regions = 0;
	for (layer = saved_layer; layer < MS_GLK_PALETTE_SIZE; layer++)
	    {
		for (y = saved_y; y < ms_glk_graphics_height; y++)
		    {
			for (x = saved_x; x < ms_glk_graphics_width; x++)
			    {
				int	index;		/* x,y pixel index */

				/* Get the index for this pixel. */
				index = y * ms_glk_graphics_width + x;
				assert (index < painted_xy_size);

				/*
				 * Ignore pixels already painted, and pixels
				 * not in the current layer.
				 */
				if (glk_layers[ ms_glk_graphics_bitmap [index]]
									== layer
					&& !glk_painted_xy[ index ])
				    {
					/*
					 * Rather than painting just one pixel,
					 * here we try to paint the maximal
					 * region we can for the layer of the
					 * given pixel.
					 */
					ms_glk_graphics_paint_region
						(ms_glk_graphics_window,
						glk_palette, glk_layers,
						ms_glk_graphics_bitmap,
						x, y, x_offset, y_offset,
						MS_GLK_GRAPHICS_PIXEL,
						ms_glk_graphics_width,
						ms_glk_graphics_height,
						glk_painted_xy);

					/*
					 * Increment count of regions handled,
					 * and yield, by returning, if the
					 * limit on paint regions is reached.
					 * Before returning, save the current
					 * layer and scan coordinates, so we
					 * can pick up here on the next call.
					 */
					regions++;
					if (regions >= MS_GLK_REPAINT_LIMIT)
					    {
						saved_layer	= layer;
						saved_x		= x;
						saved_y		= y;
						return;
					    }
				    }
			    }

			/* Reset the saved x coordinate on y increment. */
			saved_x = 0;
		    }

		/* Reset the saved y coordinate on layer change. */
		saved_y = 0;
	    }

	/*
	 * If we reach this point, then we didn't get to the limit on
	 * regions painted on this pass.  In that case, we've finished
	 * rendering the image.  Stop Glk timeouts, and reset the state
	 * to stopped.
	 */
	assert (regions < MS_GLK_REPAINT_LIMIT);
	ms_glk_graphics_stop ();
}


/*---------------------------------------------------------------------*/
/*  Glk port output functions                                          */
/*---------------------------------------------------------------------*/

/*
 * Output buffer.  We receive characters one at a time, and it's a bit
 * more efficient for everyone if we buffer them, and output a complete
 * string on a flush call.
 */
#define	GLK_BUFFER_LENGTH				10240
static char		ms_glk_output_buffer[GLK_BUFFER_LENGTH];
static int		ms_glk_output_length		= 0;

/* Flag to indicate if the last thing flushed was a ">" prompt. */
static int		ms_glk_output_prompt		= FALSE;


/*
 * ms_glk_game_prompted()
 *
 * Return TRUE if the last game output appears to have been a ">" prompt.
 * Once called, the flag is reset to FALSE, and requires more game output
 * to set it again.
 */
static int
ms_glk_game_prompted (void)
{
	int		result;				/* Return value. */

	/* Save the current flag value, and reset the main flag. */
	result = ms_glk_output_prompt;
	ms_glk_output_prompt = FALSE;

	/* Return the old value. */
	return result;
}


/*
 * ms_flush()
 *
 * Flush any buffered output text to the Glk main window, and clear the
 * buffer.
 */
void
ms_flush (void)
{
	assert (glk_stream_get_current () != NULL);

	/* Do nothing if the buffer is currently empty. */
	if (ms_glk_output_length > 0)
	    {
		/*
		 * Print the buffer to the stream for the main window,
		 * in game output style.
		 */
		glk_set_style (style_Normal);
		glk_put_buffer (ms_glk_output_buffer, ms_glk_output_length);

		/*
		 * If we just flushed a non-empty line, set the prompt flag.
		 * Some games, typically Magnetic Windows, don't re-prompt
		 * on empty input lines, so to correct this we can do our
		 * own.
		 */
		if (!(ms_glk_output_length == 1
				&& ms_glk_output_buffer[0] == '\n'))
			ms_glk_output_prompt = TRUE;
		else
			ms_glk_output_prompt = FALSE;

		/* Clear the buffer of current contents. */
		ms_glk_output_length = 0;
	    }
}


/*
 * ms_putchar()
 *
 * Buffer a character for eventual printing to the main window.
 */
void
ms_putchar (type8 c)
{
	assert (ms_glk_output_length <= sizeof (ms_glk_output_buffer) - 1);

	/*
	 * See if the character is a backspace.  Magnetic Scrolls games
	 * can send backspace characters to the display.  We'll need to
	 * handle such characters specially.
	 */
	if (c == '\b')
	    {
		/* For a backspace, take a character out of the buffer. */
		if (ms_glk_output_length > 0)
			ms_glk_output_length--;

		/* Nothing more to do with this character. */
		return;
	    }

	/* Add the new character to the buffer. */
	ms_glk_output_buffer[ ms_glk_output_length++ ] = c;

	/*
	 * If the buffer is full, or if the character was a newline, flush
	 * now.  Other implementations also flush on space, but we won't.
	 * The Glk display windows aren't updated until glk_select(), so
	 * in practice, we just need to remember to call ms_flush() when
	 * we're ready to read in another line of text input from Glk.
	 *
	 * In this way, we avoid problems where the interpreter outputs a
	 * backspace just after a space; if we had flushed on space we'd
	 * have an empty buffer, and so be unable to handle it properly.
	 */
	if (ms_glk_output_length == sizeof (ms_glk_output_buffer)
			|| c == '\n')
		ms_flush ();
}


/*
 * ms_glk_message_string()
 * ms_glk_message_char()
 *
 * Print a simple text message in a style that stands out from game text.
 * This makes interpreter messages a bit more evident.
 */
static void
ms_glk_message_string (const char *message)
{
	assert (message != NULL);

	/*
	 * Print the message, in a style that hints that it's from the
	 * interpreter, not the game.
	 */
	glk_set_style (style_Emphasized);
	glk_put_string ((char *) message);
	glk_set_style (style_Normal);
}

static void
ms_glk_message_char (char c)
{
	/* Print the character, in a message style. */
	glk_set_style (style_Emphasized);
	glk_put_char (c);
	glk_set_style (style_Normal);
}


/*
 * ms_glk_normal_string()
 * ms_glk_normal_char()
 *
 * Print a text message in normal text style.  Convenience functions to
 * avoid a lot of casting to (char*) with glk base functions.
 */
static void
ms_glk_normal_string (const char *message)
{
	assert (message != NULL);

	/* Print the message, in normal text style. */
	glk_set_style (style_Normal);
	glk_put_string ((char *) message);
}

static void
ms_glk_normal_char (char c)
{
	/* Print the character, in normal text style. */
	glk_set_style (style_Normal);
	glk_put_char (c);
}


/*
 * ms_glk_banner_string()
 *
 * Print a text message for the banner at the start of a game run.
 */
static void
ms_glk_banner_string (const char *banner)
{
	assert (banner != NULL);

	/* Print the banner in a subheadings style. */
	glk_set_style (style_Subheader);
	glk_put_string ((char *) banner);
	glk_set_style (style_Normal);
}


/*
 * ms_fatal()
 *
 * Handle fatal interpreter error message.
 */
void
ms_fatal (type8s *txt)
{
	/* Print an interpreter message indicating the error. */
	ms_glk_message_string
			("\n\nThe Magnetic Scrolls interpreter encountered"
			" a fatal error:\n\n");
	ms_glk_message_string (txt);
	ms_glk_message_string
			("\n\nThe current game state is not recoverable."
			" Sorry.\n");
	glk_exit ();
}


/*---------------------------------------------------------------------*/
/*  Glk port status line functions                                     */
/*---------------------------------------------------------------------*/

/*
 * The interpreter feeds us status line characters one at a time, with
 * Tab indicating right justify, and CR indicating the line is complete.
 * To get this to fit with the Glk event and redraw model, here we'll
 * buffer each completed status line, so we have a stable string to
 * output when needed.
 */
#define	GLK_STATBUFFER_LENGTH				256
static char		ms_glk_status_buffer[GLK_STATBUFFER_LENGTH];
static int		ms_glk_status_length		= 0;


/*
 * ms_statuschar()
 *
 * Receive one status character from the interpreter.  Characters are
 * buffered internally, and on CR, the buffer is copied to the main
 * static status buffer for use by the status line printing function.
 */
void
ms_statuschar (type8 c)
{
	static char	buffer[GLK_STATBUFFER_LENGTH];
						/* Local line buffer. */
	static int	length		= 0;	/* Local buffered data len. */

	/* If the status character is newline, transfer buffered data. */
	if (c == '\n')
	    {
		/* Transfer locally buffered string. */
		memcpy (ms_glk_status_buffer, buffer, length);
		ms_glk_status_length = length;

		/* Empty the local buffer. */
		length = 0;
		return;
	    }

	/* If there is space, add the character to the local buffer. */
	if (length < sizeof (buffer))
		buffer[ length++ ] = c;
}


/*
 * ms_glk_status_redraw()
 *
 * If there is a status window, update the information in it with the
 * current contents of the completed status line buffer.
 */
static void
ms_glk_status_redraw (void)
{
	glui32		width, height;		/* Status window dimensions. */
	strid_t		status_stream;		/* Status window stream. */
	int		index;			/* Buffer character index. */

	/* Do nothing if there is no status window. */
	if (ms_glk_status_window == NULL)
		return;

	/* Measure the status window, and do nothing if height is zero. */
	glk_window_get_size (ms_glk_status_window, &width, &height);
	if (height <= 0)
		return;

	/* Clear the current status window contents, and position cursor. */
	glk_window_clear (ms_glk_status_window);
	glk_window_move_cursor (ms_glk_status_window, 0, 0);
	status_stream = glk_window_get_stream (ms_glk_status_window);

	/* See if we have a completed status line to display. */
	if (ms_glk_status_length > 0)
	    {
		/* Output each character from the status line buffer. */
		for (index = 0; index < ms_glk_status_length; index++)
		    {
			/*
			 * If the character is Tab, position the cursor to
			 * eleven characters shy of the status window right.
			 */
			if (ms_glk_status_buffer[ index ] == '\t')
				glk_window_move_cursor
					(ms_glk_status_window, width - 11, 0);
			else
				/* Just add the character at this position. */
				glk_put_char_stream (status_stream,
						ms_glk_status_buffer[ index ]);
		    }
	    }
	else
	    {
		/*
		 * We have no status line information to display, so just
		 * print a standard message.  This happens with Magnetic
		 * Windows games, which don't, in general, use a status line.
		 */
		glk_put_string_stream (status_stream,
					"Glk Magnetic version 2.1");
	    }
}


/*---------------------------------------------------------------------*/
/*  Glk port hint functions                                            */
/*---------------------------------------------------------------------*/

/* Hint type definitions. */
static const int	MS_GLK_HINT_TYPE_FOLDER		= 1;
static const int	MS_GLK_HINT_TYPE_TEXT		= 2;

/* Success and fail return codes from hint functions. */
static const int	MS_GLK_HINT_SUCCESS		= 1;
static const int	MS_GLK_HINT_ERROR		= 0;

/* Default window sizes for non-windowing Glk libraries. */
static const int	MS_GLK_HINT_DEFAULT_WIDTH	= 72;
static const int	MS_GLK_HINT_DEFAULT_HEIGHT	= 25;

/*
 * Special hint nodes indicating the root hint node, and a value to signal
 * quit from hints subsystem.
 */
static const int	MS_GLK_HINT_ROOT_NODE		= 0;
static const int	MS_GLK_HINTS_DONE		= -1;

/*
 * Note of the interpreter's hints array.  Note that keeping its address
 * like this assumes that it's either static or heap in the interpreter.
 */
static struct ms_hint	*ms_glk_hints			= NULL;

/* Details of the current hint node on display from the hints array. */
static int		ms_glk_current_hint_node	= 0;

/*
 * Array of cursors for each hint.  The cursor indicates the current
 * position in a folder, and the last hint shown in text hints.  Space
 * is allocated as needed for a given set of hints.
 */
static int		*ms_glk_hint_cursor		= NULL;

/*
 * Pointer to any temporary hints malloc'ed memory.  As with its graphics
 * counterpart, this is memory that should be free'd on exit.
 */
static int		*ms_glk_hint_temporary		= NULL;


/*
 * ms_glk_hint_max_node()
 *
 * Return the maximum hint node referred to by the tree under the given
 * node.  The result is the largest index found, or node, if greater.
 * Because the interpreter doesn't supply it, we need to uncover it the
 * hard way.  The function is recursive, and since it is a tree search,
 * assumes that hints is a tree, not a graph.
 */
int
ms_glk_hint_max_node (struct ms_hint hints[], int node)
{
	type16	nodetype;			/* Node type. */
	int	result = node;			/* Result node. */

	/* First find the type of the node passed in. */
	nodetype = hints[ node ].nodetype;

	/*
	 * While we're looking, make sure the hints array doesn't contain
	 * any undefined node types.  Since this function should visit
	 * all nodes, putting the test here verifies the whole hints array.
	 */
	assert (nodetype == MS_GLK_HINT_TYPE_FOLDER
				|| nodetype == MS_GLK_HINT_TYPE_TEXT);

	/* Check the node type -- folder or text. */
	if (nodetype == MS_GLK_HINT_TYPE_FOLDER)
	    {
		int	index;			/* Folder element iterator */

		/* Initialize the result to be the node handed in. */
		result = node;

		/* Now recurse for each link node referenced in this node. */
		for (index = 0; index < hints[ node ].elcount; index++)
		    {
			int	link_result;	/* Node result for link */

			/*
			 * Find the maximum node reference for each link, and
			 * update the result if greater.
			 */
			link_result = ms_glk_hint_max_node
					(hints, hints[ node ].links[ index ]);
			if (link_result > result)
				result = link_result;
		    }
	    }
	else
		if (nodetype == MS_GLK_HINT_TYPE_TEXT)
		    {
			/* The largest node here is simply this node. */
			result = node;
		    }

	/* Return the largest node index found. */
	return result;
}


/*
 * ms_glk_hint_topic()
 *
 * Return the topic string for a given hint node.  This is found by searching
 * the parent node for a link to the node handed in.  For the root node, the
 * string is defaulted, since the root node has no parent.
 */
static char *
ms_glk_hint_topic (struct ms_hint hints[], int node)
{
	int	parent;			/* Node's parent node. */
	int	index;			/* Node link iterator. */
	assert (hints != NULL);

	/* If the node is not the root node, search its parent. */
	if (node != MS_GLK_HINT_ROOT_NODE)
	    {
		/* Locate the node's parent. */
		parent = hints[ node ].parent;

		/* Iterate over the parent's links. */
		for (index = 0; index < hints[ parent ].elcount; index++)
		    {
			/* Check for a link match. */
			if (hints[ parent ].links[ index ] == node)
				return hints[ parent ].content[ index ];
		    }

		/* No match, so return a generic string. */
		return "Hints Menu";
	    }
	else
	    {
		/* Return a default topic string for the root node. */
		return "Hints Menu";
	    }
}


/*
 * ms_glk_hint_open()
 *
 * If not already open, open the hints windows.  Returns TRUE if the windows
 * opened, or were already open.
 *
 * The function creates two hints windows -- a text grid on top, for menus,
 * and a text buffer below for hints.
 */
static int
ms_glk_hint_open (void)
{
	/*
	 * If not present, open the hints menu window.  The initial size
	 * is two lines, but we'll change this later to suit the hint.
	 */
	if (ms_glk_hint_menu_window == NULL)
	    {
		/* Open the hint menu window. */
		ms_glk_hint_menu_window = glk_window_open (ms_glk_main_window,
					winmethod_Above|winmethod_Fixed,
					2, wintype_TextGrid, 0);

		/* If the window open fails, return FALSE. */
		if (ms_glk_hint_menu_window == NULL)
			return FALSE;
	    }

	/*
	 * Now open the hints text window.  This is set to be 100% of the
	 * size of the main window, so should cover what remains of it
	 * completely.
	 */
	if (ms_glk_hint_text_window == NULL)
	    {
		/* Open the hint text window. */
		ms_glk_hint_text_window = glk_window_open (ms_glk_main_window,
					winmethod_Above|winmethod_Proportional,
					100, wintype_TextBuffer, 0);

		/* If this fails, close the menu window and return FALSE. */
		if (ms_glk_hint_text_window == NULL)
		    {
			glk_window_close (ms_glk_hint_menu_window, NULL);
			return FALSE;
		    }
	    }

	/* Hints windows opened successfully. */
	return TRUE;
}


/*
 * ms_glk_hint_close()
 *
 * If open, close the hints windows.
 */
static void
ms_glk_hint_close (void)
{
	/* If the hints text window is open, close it. */
	if (ms_glk_hint_text_window != NULL)
	    {
		glk_window_close (ms_glk_hint_text_window, NULL);
		ms_glk_hint_text_window = NULL;

		/* Same for the hint menu window. */
		if (ms_glk_hint_menu_window != NULL)
		    {
			glk_window_close (ms_glk_hint_menu_window, NULL);
			ms_glk_hint_menu_window = NULL;
		    }
	    }
}


/*
 * ms_glk_hint_windows_available()
 *
 * Return TRUE if hints windows are available.  If they're not, the hints
 * system will need to use alternative output methods.
 */
static int
ms_glk_hint_windows_available (void)
{
	return (ms_glk_hint_menu_window != NULL
			&& ms_glk_hint_text_window != NULL);
}


/*
 * ms_glk_hint_menu_print()
 * ms_glk_hint_menu_header()
 * ms_glk_hint_menu_justify()
 * ms_glk_hint_text_print()
 * ms_glk_hint_menutext_done()
 * ms_glk_hint_menutext_start()
 *
 * Output functions for writing hints.  These functions will write to hints
 * windows where available, and to the main window where not.  When writing
 * to hints windows, they also take care not to line wrap in the menu window.
 * Limited formatting is available.
 */
static void
ms_glk_hint_menu_print (int line, int column, const char *string,
			int width, int height)
{
	int		posn, index; 		/* Line and string positions */
	strid_t		hint_stream;		/* Hint menu window stream */
	assert (string != NULL);

	/* Ignore the call if the line number exceeds available space. */
	if (line >= height)
		return;

	/* See if hints windows are available. */
	if (ms_glk_hint_windows_available ())
	    {
		/* Get the hint menu window stream. */
		hint_stream = glk_window_get_stream (ms_glk_hint_menu_window);

		/*
		 * Write characters until the end of the string, or the end of
		 * the window.
		 */
		for (posn = column, index = 0;
				posn < width && index < strlen (string);
				posn++, index++)
		    {
			glk_window_move_cursor (ms_glk_hint_menu_window,
								posn, line);
			glk_put_char_stream (hint_stream, string[ index ]);
		    }
	    }
	else
	    {
		static int	current_line	= 0;	/* Retained line num */
		static int	current_column	= 0;	/* Retained col num */

		/*
		 * Check the line number against the last one output.  If it
		 * is less, assume the start of a new block.  In this case,
		 * perform a hokey type of screen clear.
		 */
		if (line < current_line)
		    {
			int	index;			/* Line count */

			/* Output a number of newlines. */
			for (index = 0; index < height; index++)
				ms_glk_normal_char ('\n');
			current_line	= 0;
			current_column	= 0;
		    }

		/* Print blank lines until the target line is reached. */
		for (; current_line < line; current_line++)
		    {
			ms_glk_normal_char ('\n');
			current_column	= 0;
		    }

		/* Now print spaces until the target column is reached. */
		for (; current_column < column; current_column++)
			ms_glk_normal_char (' ');

		/* Print the string to the main window, and update column. */
		ms_glk_normal_string (string);
		current_column += strlen (string);
	    }
}

static void
ms_glk_hint_menu_header (int line, const char *string,
			int width, int height)
{
	int		posn;			/* Line positioning */
	assert (string != NULL);

	/* Calculate the positioning for centred text. */
	posn = (strlen (string) < width)
			? (width - strlen (string)) / 2 : 0;

	/* Output the text in the approximate line centre. */
	ms_glk_hint_menu_print (line, posn, string, width, height);
}

static void
ms_glk_hint_menu_justify (int line,
			const char *left_string, const char *right_string,
			int width, int height)
{
	int		posn;			/* Line positioning */
	assert (left_string != NULL && right_string != NULL);

	/* Calculate the positioning for the right justified text. */
	posn = (strlen (right_string) < width)
			? (width - strlen (right_string)) : 0;

	/* Write left text normally to window left. */
	ms_glk_hint_menu_print (line, 0, left_string, width, height);

	/* Output the left text flush with the right of the window. */
	ms_glk_hint_menu_print (line, posn, right_string, width, height);
}

static void
ms_glk_hint_text_print (const char *string)
{
	strid_t		hint_stream;		/* Hint text window stream */
	assert (string != NULL);

	/* See if hints windows are available. */
	if (ms_glk_hint_windows_available ())
	    {
		/* Get the hint text window stream. */
		hint_stream = glk_window_get_stream (ms_glk_hint_text_window);

		/* Output the string to the window stream. */
		glk_put_string_stream (hint_stream, (char *)string);
	    }
	else
	    {
		/* Simply output the text to the main window. */
		ms_glk_normal_string (string);
	    }
}

static void
ms_glk_hint_menutext_start (void)
{
	/*
	 * Twiddle for non-windowing libraries; 'clear' the main window
	 * by writing a null string at line 1, then a null string at line
	 * 0.  This works because we know the current output line in
	 * ms_glk_hint_menu_print() is zero, since we set it that way
	 * with ms_glk_hint_menutext_done(), or if this is the first call,
	 * then that's its initial value.
	 */
	if (!ms_glk_hint_windows_available ())
	    {
		ms_glk_hint_menu_print (1, 0, "",
			MS_GLK_HINT_DEFAULT_WIDTH, MS_GLK_HINT_DEFAULT_HEIGHT);
		ms_glk_hint_menu_print (0, 0, "",
			MS_GLK_HINT_DEFAULT_WIDTH, MS_GLK_HINT_DEFAULT_HEIGHT);
	    }
}

static void
ms_glk_hint_menutext_done (void)
{
	/*
	 * Twiddle for non-windowing libraries; 'clear' the main window
	 * by writing an empty string to line zero.  For windowing Glk
	 * libraries, this function does nothing.
	 */
	if (!ms_glk_hint_windows_available ())
	    {
		ms_glk_hint_menu_print (0, 0, "",
			MS_GLK_HINT_DEFAULT_WIDTH, MS_GLK_HINT_DEFAULT_HEIGHT);
	    }
}


/*
 * ms_glk_hint_menutext_char_event()
 *
 * Request and return a character event from the hints windows.  In practice,
 * this means either of the hints windows if available, or the main window
 * if not.
 */
static void
ms_glk_hint_menutext_char_event (event_t *event)
{
	/* See if hints windows are available. */
	if (ms_glk_hint_windows_available ())
	    {
		/* Request a character from either hint window. */
		glk_request_char_event (ms_glk_hint_menu_window);
		glk_request_char_event (ms_glk_hint_text_window);

		/* Wait for either event to arrive. */
		ms_glk_event_wait (evtype_CharInput, event);
		assert (event->win == ms_glk_hint_menu_window
				|| event->win == ms_glk_hint_text_window);

		/* Cancel the unused event request. */
		if (event->win == ms_glk_hint_text_window)
			glk_cancel_char_event (ms_glk_hint_menu_window);
		else
			if (event->win == ms_glk_hint_menu_window)
				glk_cancel_char_event (ms_glk_hint_text_window);
	    }
	else
	    {
		/* Request a character from the main window, and wait. */
		glk_request_char_event (ms_glk_main_window);
		ms_glk_event_wait (evtype_CharInput, event);
	    }
}


/*
 * ms_glk_hint_display_folder()
 *
 * Update the hints windows for the given folder hint node.
 */
static void
ms_glk_hint_display_folder (struct ms_hint hints[], int cursor[], int node)
{
	glui32		width, height;		/* Menu window dimensions */
	int		index;			/* General loop index */
	assert (hints != NULL && cursor != NULL);

	/* Arrange windows to suit the hint folder. */
	if (ms_glk_hint_windows_available ())
	    {
		/* Resize the hint menu window to fit the current hint. */
		glk_window_set_arrangement
			(glk_window_get_parent (ms_glk_hint_menu_window),
			 winmethod_Above|winmethod_Fixed,
			 hints[ node ].elcount + 5, NULL);

		/* Measure the hint menu window, and clear it. */
		glk_window_get_size (ms_glk_hint_menu_window, &width, &height);
		glk_window_clear (ms_glk_hint_menu_window);

		/* Simply clear the hint text window. */
		glk_window_clear (ms_glk_hint_text_window);
	    }
	else
	    {
		/* No hints windows, so default width and height. */
		width	= MS_GLK_HINT_DEFAULT_WIDTH;
		height	= MS_GLK_HINT_DEFAULT_HEIGHT;
	    }

	/* Paint in the menu header. */
	ms_glk_hint_menu_header (0,
				ms_glk_hint_topic (hints, node),
				width, height);
	ms_glk_hint_menu_justify (1,
				" N = next subject  ",
				"  P = previous ",
				width, height);
	ms_glk_hint_menu_justify (2,
				" RETURN = read subject  ",
				(node == MS_GLK_HINT_ROOT_NODE)
					? "  Q = resume game "
					: "  Q = previous menu ",
				width, height);

	/* Output the menu for the node's folder hint. */
	for (index = 0; index < hints[ node ].elcount; index++)
	    {
		/* Set pointer to the selected hint. */
		if (index == cursor[ node ])
			ms_glk_hint_menu_print (index + 4, 3, ">",
					width, height);

		/* Print the folder text. */
		ms_glk_hint_menu_print (index + 4, 5,
				hints[ node ].content[ index ],
				width, height);
	    }

	/*
	 * Terminate with a blank line; using a single space here improves
	 * cursor positioning for optimized output libraries (for example,
	 * without it, curses output will leave the cursor at the end of
	 * the previous line).
	 */
	ms_glk_hint_menu_print
			(hints[ node ].elcount + 4, 0, " ", width, height);
}


/*
 * ms_glk_hint_display_text()
 *
 * Update the hints windows for the given text hint node.
 */
static void
ms_glk_hint_display_text (struct ms_hint hints[], int cursor[], int node)
{
	glui32		width, height;		/* Menu window dimensions */
	int		index;			/* General loop index */

	/* Arrange windows to suit the hint text. */
	if (ms_glk_hint_windows_available ())
	    {
		/* Resize the hint menu window to just two lines. */
		glk_window_set_arrangement
			(glk_window_get_parent (ms_glk_hint_menu_window),
			 winmethod_Above|winmethod_Fixed, 2, NULL);

		/* Measure the hint menu window, and clear it. */
		glk_window_get_size (ms_glk_hint_menu_window, &width, &height);
		glk_window_clear (ms_glk_hint_menu_window);

		/* Clear the hints text window. */
		glk_window_clear (ms_glk_hint_text_window);
	    }
	else
	    {
		/* No hints windows, so default width and height. */
		width	= MS_GLK_HINT_DEFAULT_WIDTH;
		height	= MS_GLK_HINT_DEFAULT_HEIGHT;
	    }

	/* Paint in a short menu header. */
	ms_glk_hint_menu_header (0,
				ms_glk_hint_topic (hints, node),
				width, height);
	ms_glk_hint_menu_justify (1,
				" RETURN = read hint  ",
				"  Q = previous menu ",
				width, height);

	/*
	 * Output hints, up to the number that should be shown.  This number
	 * is the cursor for the hint.
	 */
	ms_glk_hint_text_print ("\n");
	for (index = 0; index < hints[ node ].elcount; index++)
	    {
		char	buffer[16];		/* Conversion buffer */

		/* Output the hint index number. */
		sprintf (buffer, "%3d", index + 1);
		ms_glk_hint_text_print (buffer);
		ms_glk_hint_text_print (".  ");

		/* Output the hint, or a dash if it's not yet exposed. */
		if (index < cursor[ node ])
			ms_glk_hint_text_print
					(hints[ node ].content[ index ]);
		else
			ms_glk_hint_text_print ("-");
		ms_glk_hint_text_print ("\n");
	    }
}


/*
 * ms_glk_hint_display()
 *
 * Display the given hint using the appropriate display function.
 */
static void
ms_glk_hint_display (struct ms_hint hints[], int cursor[], int node)
{
	type16		nodetype;		/* Current node type. */
	assert (hints != NULL && cursor != NULL);

	/* Call the right display function for the current hint. */
	nodetype = hints[ node ].nodetype;
	if (nodetype == MS_GLK_HINT_TYPE_FOLDER)
	    {
		ms_glk_hint_display_folder (hints, cursor, node);
	    }
	else
		if (nodetype == MS_GLK_HINT_TYPE_TEXT)
		    {
			ms_glk_hint_display_text (hints, cursor, node);
		    }
}


/*
 * ms_glk_hint_handle_folder()
 *
 * Handle a character event for the given folder hint.  Return the next
 * node to handle, or -1 on Quit at the root node.
 */
static int
ms_glk_hint_handle_folder (struct ms_hint hints[], int cursor[], int node,
			event_t *event)
{
	unsigned char	response;		/* Response character */
	int		next_node;		/* Return value */
	assert (hints != NULL && cursor != NULL);
	assert (event->type = evtype_CharInput);

	/* Convert key event into a single response character. */
	switch (event->val1)
	    {
		case keycode_Down:	response = 'N';  break;
		case keycode_Up:	response = 'P';  break;
		case keycode_Right:
		case keycode_Return:	response = '\n'; break;
		case keycode_Left:	response = 'Q';  break;
		default:
			response = glk_char_to_upper (event->val1);
			break;
	    }

	/* Now handle the response character. */
	switch (response)
	    {
		case 'N':
			/* Advance the hint cursor. */
			cursor[ node ]++;
			if (cursor[ node ] >= hints[ node ].elcount)
				cursor[ node ] = 0;
			next_node = node;
			break;

		case 'P':
			/* Regress the hint cursor. */
			cursor[ node ]--;
			if (cursor[ node ] < 0)
				cursor[ node ] = hints[ node ].elcount - 1;
			next_node = node;
			break;

		case '\n':
			/* Return the hint node at the cursor position. */
			next_node = hints[ node ].links[ cursor[ node ]];
			break;

		case 'Q':
			/* If at root, return -1, otherwise return parent. */
			if (node == MS_GLK_HINT_ROOT_NODE)
				next_node = MS_GLK_HINTS_DONE;
			else
				next_node = hints[ node ].parent;
			break;

		default:
			next_node = node;
			break;
	    }

	/* Return the next node identified for handling. */
	return next_node;
}


/*
 * ms_glk_hint_handle_text()
 *
 * Handle a character event for the given text hint.  Return the next
 * node to handle.
 */
static int
ms_glk_hint_handle_text (struct ms_hint hints[], int cursor[], int node,
			event_t *event)
{
	unsigned char	response;		/* Response character */
	int		next_node;		/* Return value */
	assert (hints != NULL && cursor != NULL);
	assert (event->type = evtype_CharInput);

	/* Convert key event into a single response character. */
	switch (event->val1)
	    {
		case keycode_Right:
		case keycode_Return:	response = '\n'; break;
		case keycode_Left:	response = 'Q';  break;
		default:
			response = glk_char_to_upper (event->val1);
			break;
	    }

	/* Now handle the response character. */
	switch (response)
	    {
		case '\n':
			/* If not at end, advance the hint cursor. */
			if (cursor[ node ] < hints[ node ].elcount)
				cursor[ node ]++;
			next_node = node;
			break;

		case 'Q':
			/* Done with this hint node, so return parent. */
			next_node = hints[ node ].parent;
			break;

		default:
			next_node = node;
			break;
	    }

	/* Return the next node identified for handling. */
	return next_node;
}


/*
 * ms_glk_hint_handle()
 *
 * Handle the given hint using the appropriate handler function.  Return
 * the next node to handle.
 */
static int
ms_glk_hint_handle (struct ms_hint hints[], int cursor[], int node,
			event_t *event)
{
	type16		nodetype;		/* Current node type. */
	int		next_node;		/* Return value */
	assert (hints != NULL && cursor != NULL);
	assert (event->type = evtype_CharInput);

	/* Call the right handler function for the current hint. */
	nodetype = hints[ node ].nodetype;
	if (nodetype == MS_GLK_HINT_TYPE_FOLDER)
	    {
		next_node = ms_glk_hint_handle_folder
						(hints, cursor, node, event);
	    }
	else
		if (nodetype == MS_GLK_HINT_TYPE_TEXT)
		    {
			next_node = ms_glk_hint_handle_text
						(hints, cursor, node, event);
		    }
		else
		    {
			/* Unknown node type?  This should never happen. */
			next_node = MS_GLK_HINT_ROOT_NODE;
		    }

	/* Return the next node. */
	return next_node;
}


/*
 * ms_showhints()
 *
 * Start game hints.  These are modal, though there's no overriding Glk
 * reason why.  It's just that this matches the way they're implemented by
 * most Inform games.  This may not be the best way of doing help, but at
 * least it's likely to be familiar, and anything more ambitious may be
 * beyond the current Glk capabilities.
 *
 * This function uses CRCs to detect any change of hints data.  Normally,
 * we'd expect none, at least within a given game run, but we can probably
 * handle it okay if it happens.
 */
type8
ms_showhints (struct ms_hint *hints)
{
	static int	initialized	= FALSE;/* First call flag. */
	static glui32	current_crc	= 0;	/* CRC of the current hints */

	int		hint_count;		/* Count of hints in array */
	glui32		crc;			/* Hints array CRC */
	int		node;			/* Hint traversal node */
	assert (hints != NULL);

	/*
	 * Find the number of hints in the array.  To do this, we'll visit
	 * every node in a tree search, starting at the root, to locate the
	 * maximum node number found, then add one to that.  It's a pity
	 * that the interpreter doesn't hand us this information directly.
	 */
	hint_count = ms_glk_hint_max_node (hints, MS_GLK_HINT_ROOT_NODE) + 1;

	/* Calculate a CRC for the hints array data. */
	crc = ms_glk_buffer_crc ((unsigned char *)hints,
					hint_count * sizeof (struct ms_hint));

	/*
	 * If the CRC has changed, or this is the first call, assign a new
	 * cursor array.
	 */
	if (crc != current_crc
			|| !initialized)
	    {
		int	index;			/* Array iterator */

		/* Free any current cursor array. */
		if (ms_glk_hint_cursor != NULL)
			free (ms_glk_hint_cursor);

		/* Now create a new cursor array. */
		ms_glk_hint_cursor = ms_glk_malloc (hint_count * sizeof (int));

		/*
		 * Note hints cursor as temporary data, so we can free it
		 * on interpreter exit.
		 */
		ms_glk_hint_temporary = ms_glk_hint_cursor;

		/* Retain the hints CRC, for later comparisons. */
		current_crc = crc;

		/* Reset all hint cursors to an initial state. */
		for (index = 0; index < hint_count; index++)
			ms_glk_hint_cursor[ index ] = 0;

		/* Set the initialized flag. */
		initialized = TRUE;
	    }

	/*
	 * Save the hints array passed in.  This is done here since even
	 * if the data remains the same (found by the CRC check above),
	 * the pointer to it might have changed.
	 */
	ms_glk_hints = hints;

	/*
	 * Try to create the hints windows.  If they can't be created,
	 * perhaps because the Glk library doesn't support it, the output
	 * functions will work around this.
	 */
	ms_glk_hint_open ();
	ms_glk_hint_menutext_start ();

	/*
	 * Begin hints display at the root node, and navigate until the
	 * user exit hints.
	 */
	node = MS_GLK_HINT_ROOT_NODE;
	while (node != MS_GLK_HINTS_DONE)
	    {
		int		*cursor;	/* Hints cursor array */
		event_t		event;		/* Glk event buffer */

		/*
		 * Update the current hint module variable so that the
		 * redraw function knows what hint to display if called.
		 */
		ms_glk_current_hint_node = node;

		/* Display the node being visited. */
		cursor = ms_glk_hint_cursor;
		ms_glk_hint_display (hints, cursor, node);

		/* Get and handle an event for the node. */
		ms_glk_hint_menutext_char_event (&event);
		node = ms_glk_hint_handle (hints, cursor, node, &event);
	    }

	/* Done with hint windows. */
	ms_glk_hint_menutext_done ();
	ms_glk_hint_close ();

	/* Return successfully. */
	return MS_GLK_HINT_SUCCESS;
}


/*
 * ms_glk_hint_redraw()
 *
 * Update the hints windows for the current hint.  This function should be
 * called from the event handler on resize events, to repaint the hints
 * display.  It does nothing if no hints windows have been opened, since
 * in this case, there's no resize action required -- either we're not in
 * the hints subsystem, or hints are being displayed in the main game
 * window, for whatever reason.
 */
static void
ms_glk_hint_redraw (void)
{
	/* If no hint windows are open, ignore the call. */
	if (ms_glk_hint_windows_available ())
	    {
		/* Draw in the current hint. */
		assert (ms_glk_hints != NULL && ms_glk_hint_cursor != NULL);
		ms_glk_hint_display (ms_glk_hints, ms_glk_hint_cursor,
						ms_glk_current_hint_node);
	    }
}


/*---------------------------------------------------------------------*/
/*  Glk command escape functions                                       */
/*---------------------------------------------------------------------*/

/* Valid command control values. */
static const char	*MS_GLK_COMMAND_ON		= "on";
static const char	*MS_GLK_COMMAND_OFF		= "off";


/*
 * ms_glk_command_undo()
 *
 * Stub function for the undo command.  The real work is to return the
 * undo code to the input functions.
 */
static void
ms_glk_command_undo (const char *argument)
{
}


/*
 * ms_glk_command_script()
 *
 * Turn game output scripting (logging) on and off.
 */
static void
ms_glk_command_script (const char *argument)
{
	assert (argument != NULL);

	/* Set up a transcript according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		frefid_t	fileref;	/* Glk file reference. */

		/* See if a transcript is already active. */
		if (ms_glk_transcript_stream != NULL)
		    {
			ms_glk_normal_string ("Glk transcript is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* Get a Glk file reference for a transcript. */
		fileref = glk_fileref_create_by_prompt
				(fileusage_Transcript | fileusage_TextMode,
					filemode_WriteAppend, 0);
		if (fileref == NULL)
		    {
			ms_glk_message_string ("Glk transcript failed.\n");
			return;
		    }

		/* Open a Glk stream for the fileref. */
		ms_glk_transcript_stream = glk_stream_open_file
					(fileref, filemode_WriteAppend, 0);
		if (ms_glk_transcript_stream == NULL)
		    {
			glk_fileref_destroy (fileref);
			ms_glk_message_string ("Glk transcript failed.\n");
			return;
		    }
		glk_fileref_destroy (fileref);

		/* Set the new transcript stream as the main echo stream. */
		glk_window_set_echo_stream (ms_glk_main_window,
						ms_glk_transcript_stream);

		/* Confirm action. */
		ms_glk_normal_string ("Glk transcript is now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* If a transcript is active, close it. */
		if (ms_glk_transcript_stream != NULL)
		    {
			glk_stream_close (ms_glk_transcript_stream, NULL);
			ms_glk_transcript_stream = NULL;

			/* Clear the main echo stream. */
			glk_window_set_echo_stream (ms_glk_main_window, NULL);

			/* Confirm action. */
			ms_glk_normal_string ("Glk transcript is now ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
		    }
		else
		    {
			/* Note that scripts are already disabled. */
			ms_glk_normal_string ("Glk transcript is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
		    }
	    }
	else if (strlen (argument) == 0)
	    {
		/*
		 * There was no argument on the line, so print out the current
		 * transcript mode.
		 */
		ms_glk_normal_string ("Glk transcript is ");
		if (ms_glk_transcript_stream != NULL)
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
		else
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string ("Glk transcript can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}



/*
 * ms_glk_command_inputlog()
 *
 * Turn game input logging on and off.
 */
static void
ms_glk_command_inputlog (const char *argument)
{
	assert (argument != NULL);

	/* Set up an input log according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		frefid_t	fileref;	/* Glk file reference. */

		/* See if an input log is already active. */
		if (ms_glk_inputlog_stream != NULL)
		    {
			ms_glk_normal_string ("Glk input logging is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* Get a Glk file reference for an input log. */
		fileref = glk_fileref_create_by_prompt
				(fileusage_InputRecord | fileusage_BinaryMode,
					filemode_WriteAppend, 0);
		if (fileref == NULL)
		    {
			ms_glk_message_string ("Glk input logging failed.\n");
			return;
		    }

		/* Open a Glk stream for the fileref. */
		ms_glk_inputlog_stream = glk_stream_open_file
					(fileref, filemode_WriteAppend, 0);
		if (ms_glk_inputlog_stream == NULL)
		    {
			glk_fileref_destroy (fileref);
			ms_glk_message_string ("Glk input logging failed.\n");
			return;
		    }
		glk_fileref_destroy (fileref);

		/* Confirm action. */
		ms_glk_normal_string ("Glk input logging is now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* If an input log is active, close it. */
		if (ms_glk_inputlog_stream != NULL)
		    {
			glk_stream_close (ms_glk_inputlog_stream, NULL);
			ms_glk_inputlog_stream = NULL;

			/* Confirm action. */
			ms_glk_normal_string ("Glk input log is now ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
		    }
		else
		    {
			/* Note that there is no current input log. */
			ms_glk_normal_string ("Glk input logging is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
		    }
	    }
	else if (strlen (argument) == 0)
	    {
		/*
		 * There was no argument on the line, so print out the current
		 * input logging mode.
		 */
		ms_glk_normal_string ("Glk input logging is ");
		if (ms_glk_inputlog_stream != NULL)
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
		else
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string ("Glk input logging can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}


/*
 * ms_glk_command_readlog()
 *
 * Set the game input log, to read input from a file.
 */
static void
ms_glk_command_readlog (const char *argument)
{
	assert (argument != NULL);

	/* Set up a read log according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		frefid_t	fileref;	/* Glk file reference. */

		/* See if a read log is already active. */
		if (ms_glk_readlog_stream != NULL)
		    {
			ms_glk_normal_string ("Glk read log is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* Get a Glk file reference for an input log. */
		fileref = glk_fileref_create_by_prompt
				(fileusage_InputRecord | fileusage_BinaryMode,
					filemode_Read, 0);
		if (fileref == NULL)
		    {
			ms_glk_message_string ("Glk read log failed.\n");
			return;
		    }

		/* Open a Glk stream for the fileref. */
		ms_glk_readlog_stream = glk_stream_open_file
					(fileref, filemode_Read, 0);
		if (ms_glk_readlog_stream == NULL)
		    {
			glk_fileref_destroy (fileref);
			ms_glk_message_string ("Glk read log failed.\n");
			return;
		    }
		glk_fileref_destroy (fileref);

		/* Confirm action. */
		ms_glk_normal_string ("Glk read log is now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* If an input log is active, close it. */
		if (ms_glk_readlog_stream != NULL)
		    {
			glk_stream_close (ms_glk_readlog_stream, NULL);
			ms_glk_readlog_stream = NULL;

			/* Confirm action. */
			ms_glk_normal_string ("Glk read log is now ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
		    }
		else
		    {
			/* Note that there is no current read log. */
			ms_glk_normal_string ("Glk read log is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
		    }
	    }
	else if (strlen (argument) == 0)
	    {
		/*
		 * There was no argument on the line, so print out the current
		 * read logging mode.
		 */
		ms_glk_normal_string ("Glk read log is ");
		if (ms_glk_readlog_stream != NULL)
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
		else
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string ("Glk read log can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}


/*
 * ms_glk_command_abbreviations()
 *
 * Turn abbreviation expansions on and off.
 */
static void
ms_glk_command_abbreviations (const char *argument)
{
	assert (argument != NULL);

	/* Set up abbreviation expansions according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		/* See if expansions are already on. */
		if (ms_glk_abbreviations_enabled)
		    {
			ms_glk_normal_string ("Glk abbreviation expansions"
					" are already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* The user has turned expansions on. */
		ms_glk_abbreviations_enabled = TRUE;
		ms_glk_normal_string ("Glk abbreviation expansions are now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* See if expansions are already off. */
		if (!ms_glk_abbreviations_enabled)
		    {
			ms_glk_normal_string ("Glk abbreviation expansions"
					" are already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* The user has turned expansions off. */
		ms_glk_abbreviations_enabled = FALSE;
		ms_glk_normal_string ("Glk abbreviation expansions are now ");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else if (strlen (argument) == 0)
	    {
		/*
		 * There was no argument on the line, so print out the current
		 * expansion mode.
		 */
		ms_glk_normal_string ("Glk abbreviation expansions are ");
		if (ms_glk_abbreviations_enabled)
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
		else
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string ("Glk abbreviation expansions can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}


/*
 * ms_glk_command_print_version_number()
 * ms_glk_command_version()
 *
 * Print out the Glk library version number.
 */
static void
ms_glk_command_print_version_number (glui32 version)
{
	char		buffer[64];		/* Output buffer string. */

	/* Print out the three version number component parts. */
	sprintf (buffer, "%lu.%lu.%lu",
			(version & 0xFFFF0000) >> 16,
			(version & 0x0000FF00) >>  8,
			(version & 0x000000FF)      );
	ms_glk_normal_string (buffer);
}
static void
ms_glk_command_version (const char *argument)
{
	glui32		version;		/* Glk lib version number. */

	/* Get the Glk library version number. */
	version = glk_gestalt (gestalt_Version, 0);

	/* Print the three components of the version number. */
	ms_glk_normal_string ("The Glk library version is ");
	ms_glk_command_print_version_number (version);
	ms_glk_normal_string (".\n");
	ms_glk_normal_string ("This is version ");
	ms_glk_command_print_version_number (MS_GLK_PORT_VERSION);
	ms_glk_normal_string (" of the Glk Magnetic Scrolls port.\n");
}


/*
 * ms_glk_command_graphics()
 *
 * Enable or disable graphics more permanently than is done by the
 * main interpreter.  Also, print out a few brief details about the
 * graphics state of the program.
 */
static void
ms_glk_command_graphics (const char *argument)
{
	assert (argument != NULL);

	/* If graphics is not possible, simply say so and return. */
	if (!ms_glk_graphics_possible)
	    {
		ms_glk_normal_string ("Glk graphics are not available.\n");
		return;
	    }

	/* Set up command handling according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		/* See if graphics are already on. */
		if (ms_glk_graphics_enabled)
		    {
			ms_glk_normal_string ("Glk graphics are already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* Set the graphics enabled flag to on. */
		ms_glk_graphics_enabled = TRUE;

		/*
		 * If there is a picture loaded and ready, call the
		 * restart function to repaint it.
		 */
		if (ms_glk_graphics_picture_available ())
		    {
			if (!ms_glk_graphics_open ())
			    {
				ms_glk_normal_string ("Glk graphics error.\n");
				return;
			    }
			ms_glk_graphics_restart ();
		    }

		/* Confirm actions. */
		ms_glk_normal_string ("Glk graphics are now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* See if graphics are already off. */
		if (!ms_glk_graphics_enabled)
		    {
			ms_glk_normal_string ("Glk graphics are already ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
			return;
		    }

		/*
		 * Set graphics to disabled, and stop any graphics
		 * processing.  Close the graphics window.
		 */
		ms_glk_graphics_enabled = FALSE;
		ms_glk_graphics_stop ();
		ms_glk_graphics_close ();

		/* Confirm actions. */
		ms_glk_normal_string ("Glk graphics are now ");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else if (strlen (argument) == 0)
	    {
		/* Print details about graphics availability. */
		ms_glk_normal_string ("Glk graphics are available,");
		if (ms_glk_graphics_enabled)
			ms_glk_normal_string (" and enabled.\n");
		else
			ms_glk_normal_string (" but disabled.\n");

		/* See if there is any current loaded picture. */
		if (ms_glk_graphics_picture_available ())
		    {
			int	width, height;	/* Current picture size */
			int	colours;	/* Number of colours */
			char	buffer[16];	/* Integer conversion buffer */

			/* Print the picture's dimensions and colours. */
			ms_glk_graphics_picture_details
						(&width, &height, &colours);
			ms_glk_normal_string ("There is a picture loaded, ");
			sprintf (buffer, "%d", width);
			ms_glk_normal_string (buffer);
			ms_glk_normal_string (" by ");
			sprintf (buffer, "%d", height);
			ms_glk_normal_string (buffer);
			ms_glk_normal_string (" pixels, ");
			sprintf (buffer, "%d", colours);
			ms_glk_normal_string (buffer);
			ms_glk_normal_string (" colours.\n");
		    }

		/* Indicate the state of interpreter graphics. */
		if (!ms_glk_graphics_interpreter_enabled ())
			ms_glk_normal_string ("Interpreter graphics are"
						" disabled.\n");

		/* Show if graphics are displayed, or not. */
		if (ms_glk_graphics_enabled)
		    {
			if (ms_glk_graphics_displayed ())
			    {
				/* Indicate graphics display. */
				ms_glk_normal_string ("Graphics are currently"
						" being displayed,");

				/* Add a note about gamma corrections. */
				if (ms_glk_gamma_enabled)
				    {
					ms_glk_normal_string (" with gamma ");
					ms_glk_normal_string
					  (ms_glk_graphics_gamma_correction ());
					ms_glk_normal_string (" correction.\n");
				    }
				else
				    {
					ms_glk_normal_string (" without gamma"
							" correction.\n");
				    }
			    }
			else
			    {
				/* Indicate no current graphics display. */
				ms_glk_normal_string ("Graphics are currently"
						" not being displayed.\n");
			    }
		    }
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string ("Glk graphics can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}


/*
 * ms_glk_command_gamma()
 *
 * Enable or disable picture gamma corrections.
 */
static void
ms_glk_command_gamma (const char *argument)
{
	assert (argument != NULL);

	/*
	 * If graphics is not possible, there is no point fiddling about
	 * with gamma corrections.
	 */
	if (!ms_glk_graphics_possible)
	    {
		ms_glk_normal_string ("Glk automatic gamma correction is"
							" not available.\n");
		return;
	    }

	/* Set up gamma correction according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		/* See if gamma correction is already on. */
		if (ms_glk_gamma_enabled)
		    {
			ms_glk_normal_string
				("Glk automatic gamma correction is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* Set the gamma to on, and restart graphics. */
		ms_glk_gamma_enabled = TRUE;
		ms_glk_graphics_restart ();

		/* Confirm actions. */
		ms_glk_normal_string ("Glk automatic gamma correction is now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* See if gamma correction is already off. */
		if (!ms_glk_gamma_enabled)
		    {
			ms_glk_normal_string
				("Glk automatic gamma correction is already ");
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
			ms_glk_normal_string (".\n");
			return;
		    }

		/* Set the gamma to off, and restart graphics. */
		ms_glk_gamma_enabled = FALSE;
		ms_glk_graphics_restart ();

		/* Confirm actions. */
		ms_glk_normal_string ("Glk automatic gamma correction is now ");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else if (strlen (argument) == 0)
	    {
		/*
		 * There was no argument on the line, so print out the current
		 * gamma correction mode.
		 */
		ms_glk_normal_string ("Glk automatic gamma correction is ");
		if (ms_glk_gamma_enabled)
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
		else
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string
				("Glk automatic gamma correction can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}


/*
 * ms_glk_command_command()
 *
 * Turn command escapes on and off.  Once off, there's no way to turn
 * them back on.
 */
static void
ms_glk_command_command (const char *argument)
{
	assert (argument != NULL);

	/* Set up command handling according to the argument given. */
	if (!strcasecmp (argument, MS_GLK_COMMAND_ON))
	    {
		/* The user has turned commands on, although already on. */
		ms_glk_commands_enabled = TRUE;
		ms_glk_normal_string ("Glk commands are now ");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string (".\n");
	    }
	else if (!strcasecmp (argument, MS_GLK_COMMAND_OFF))
	    {
		/* The user has turned commands off. */
		ms_glk_commands_enabled = FALSE;
		ms_glk_normal_string ("Glk commands are now ");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else if (strlen (argument) == 0)
	    {
		/*
		 * There was no argument on the line, so print out the current
		 * command mode.
		 */
		ms_glk_normal_string ("Glk commands are ");
		if (ms_glk_commands_enabled)
			ms_glk_normal_string (MS_GLK_COMMAND_ON);
		else
			ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string (".\n");
	    }
	else
	    {
		/*
		 * The command argument isn't a valid one, so print a list of
		 * valid command arguments.
		 */
		ms_glk_normal_string ("Glk commands can be '");
		ms_glk_normal_string (MS_GLK_COMMAND_ON);
		ms_glk_normal_string ("', or '");
		ms_glk_normal_string (MS_GLK_COMMAND_OFF);
		ms_glk_normal_string ("'.\n");
	    }
}


/* Escape introducer string. */
static const char	*MS_GLK_COMMAND_ESCAPE		= "glk";

/* Small table of Glk subcommands and handler functions. */
typedef const struct {
	const char	*command;		/* Glk subcommand. */
	void		(*handler) (const char *argument);
						/* Subcommand handler. */
	const int	takes_argument;		/* Argument flag. */
	const int	undo_return;		/* "Undo" return value. */
} ms_glk_commands_t;
static ms_glk_commands_t ms_glk_commands_table[] = {
	{ "undo",	ms_glk_command_undo,		FALSE,	TRUE  },
	{ "script",	ms_glk_command_script,		TRUE,	FALSE },
	{ "inputlog",	ms_glk_command_inputlog,	TRUE,	FALSE },
	{ "readlog",	ms_glk_command_readlog,		TRUE,	FALSE },
	{ "abbreviations",
			ms_glk_command_abbreviations,	TRUE,	FALSE },
	{ "graphics",	ms_glk_command_graphics,	TRUE,	FALSE },
	{ "gamma",	ms_glk_command_gamma,		TRUE,	FALSE },
	{ "version",	ms_glk_command_version,		FALSE,	FALSE },
	{ "commands",	ms_glk_command_command,		TRUE,	FALSE },
	{ NULL,		NULL,				FALSE,	FALSE }
};

/* List of whitespace command-argument separator characters. */
static const char	*MS_GLK_COMMAND_WHITESPACE	= "\t ";


/*
 * ms_glk_command_dispatch()
 *
 * Given a command string and an argument, this function finds and
 * runs any handler for it.  It returns TRUE if the command is valid,
 * FALSE otherwise.  On TRUE returns, it will also set the value for
 * undo_command to the table undo return value.
 */
static int
ms_glk_command_dispatch (const char *command, const char *argument,
			int *undo_command)
{
	ms_glk_commands_t	*entry;	/* Command table search entry. */

	/* Search the command table for a match of the command string. */
	for (entry = ms_glk_commands_table; entry->handler != NULL; entry++)
	    {
		/* On match, call the handler and exit the loop. */
		if (!strcasecmp (command, entry->command))
		    {
			ms_glk_normal_char ('\n');
			(entry->handler) (argument);

			/* Issue a brief warning if an argument was ignored. */
			if (!entry->takes_argument && strlen (argument) > 0)
			    {
				ms_glk_normal_string ("The '");
				ms_glk_normal_string (entry->command);
				ms_glk_normal_string ("' command ignores"
							" arguments.\n");
			    }

			/* Set the undo command code, and return TRUE. */
			*undo_command = entry->undo_return;
			return TRUE;
		    }
	    }

	/* There was no matching entry in the table. */
	return FALSE;
}


/*
 * ms_glk_command_usage()
 *
 * On an empty, or invalid, command, print out a list of valid commands
 * and perhaps some Glk status information.
 */
static void
ms_glk_command_usage (const char *command)
{
	ms_glk_commands_t	*entry;	/* Command table iteration entry. */
	ms_glk_commands_t	*next;	/* Next entry in command table. */

	/* Print a blank line separator. */
	ms_glk_normal_char ('\n');

	/* If the command is not empty, indicate that it was invalid. */
	if (strlen (command) > 0)
	    {
		ms_glk_normal_string ("The Glk command '");
		ms_glk_normal_string (command);
		ms_glk_normal_string ("' is not valid.\n");
	    }

	/* Print out a list of valid commands. */
	ms_glk_normal_string ("Glk commands are ");
	for (entry = ms_glk_commands_table; entry->handler != NULL; entry++)
	    {
		next = entry + 1;
		if (next->command != NULL)
		    {
			ms_glk_normal_string ("'");
			ms_glk_normal_string (entry->command);
			ms_glk_normal_string ("', ");
		    }
		else
		    {
			ms_glk_normal_string (" and '");
			ms_glk_normal_string (entry->command);
			ms_glk_normal_string ("'.\n");
		    }
	    }

	/*
	 * If no command was given, call each command handler function with
	 * an empty argument to prompt each to report the current setting.
	 */
	if (strlen (command) == 0)
	    {
		ms_glk_normal_char ('\n');
		for (entry = ms_glk_commands_table;
				entry->handler != NULL; entry++)
			(entry->handler) ("");
	    }
}


/*
 * ms_glk_command_escape()
 *
 * This function is handed each input line.  If the line contains a
 * specific Glk port command, handle it and return TRUE, otherwise
 * return FALSE.  Undo_command is set if the command is recognized as
 * a request to undo a game move.
 */
static int
ms_glk_command_escape (char *string, int *undo_command)
{
	unsigned int		index;		/* General character index. */
	assert (string != NULL);

	/*
	 * Skip any leading whitespace in string.  Throughout, this function
	 * avoids strtok(), because it uses a static buffer and is therefore
	 * not all that secure.
	 */
	index = 0;
	while (strchr (MS_GLK_COMMAND_WHITESPACE, string[ index ]) != NULL
					 && string[ index ] != '\0')
		index++;

	/* Check for Glk command escape string (introducer). */
	if (!strncasecmp (&string[ index ],
			MS_GLK_COMMAND_ESCAPE, strlen (MS_GLK_COMMAND_ESCAPE)))
	    {
		char	*string_copy;	/* Destroyable copy of string. */
		char	*command;	/* Glk subcommand. */
		char	*argument;	/* Glk subcommand argument. */

		/* Take a copy of the string. */
		string_copy = ms_glk_malloc (strlen (string) + 1);
		strcpy (string_copy, string);

		/* Move the current index to the end of the introducer. */
		index += strlen (MS_GLK_COMMAND_ESCAPE);

		/* Find the subcommand; the word after the introducer. */
		while (strchr (MS_GLK_COMMAND_WHITESPACE,
					string_copy[ index ]) != NULL
						&& string_copy[ index ] != '\0')
			index++;
		command = &string_copy[ index ];

		/* Skip over command word, be sure it terminates with NUL. */
		while (strchr (MS_GLK_COMMAND_WHITESPACE,
					string_copy[ index ]) == NULL
						&& string_copy[ index ] != '\0')
			index++;
		if (string_copy[ index ] != '\0')
		    {
			string_copy[ index ] = '\0';
			index++;
		    }

		/* Now find any argument data for the command. */
		while (strchr (MS_GLK_COMMAND_WHITESPACE,
					string_copy[ index ]) != NULL
						&& string_copy[ index ] != '\0')
			index++;
		argument = &string_copy[ index ];

		/* Delete any trailing whitespace from the argument data. */
		index = strlen (argument) - 1;
		while (strchr (MS_GLK_COMMAND_WHITESPACE,
					argument[ index ]) != NULL
						&& index > 0)
		    {
			argument[ index ] = '\0';
			index--;
		    }

		/*
		 * Try to handle the command and argument as a Glk subcommand.
		 * If this fails, output a short usage/status message.
		 */
		if (!ms_glk_command_dispatch (command,
						argument, undo_command))
		    {
			ms_glk_command_usage (command);
			*undo_command = FALSE;
		    }

		/* Done with the copy of the string. */
		free (string_copy);

		/* Return TRUE to indicate string contained a Glk command. */
		return TRUE;
	    }

	/* No Glk command escape found in the string. */
	return FALSE;
}


/*---------------------------------------------------------------------*/
/*  Glk port input functions                                           */
/*---------------------------------------------------------------------*/

/* Quote used to suppress abbreviation expansion and local commands. */
static const char	MS_GLK_QUOTED_INPUT		= '\'';

/* Definition of whitespace characters to skip over. */
static const char	*MS_GLK_WHITESPACE		= "\t ";

/* Magnetic Scrolls normal-looking prompt string. */
static const char	*MS_GLK_NORMAL_PROMPT		= ">";

/* Length of the input buffer we'll allocate for reading input lines. */
#define	MS_GLK_INPUT_BUFFER_LENGTH			256


/*
 * ms_glk_char_is_whitespace()
 *
 * Check for ASCII whitespace characters.  Returns TRUE if the character
 * qualifies as whitespace (NUL is not whitespace).
 */
static int
ms_glk_char_is_whitespace (char c)
{
	return (c != '\0' && strchr (MS_GLK_WHITESPACE, c) != NULL);
}


/* Table of single-character command abbreviations. */
typedef const struct {
	const char	abbreviation;		/* Abbreviation character. */
	const char	*expansion;		/* Expansion string. */
} ms_glk_abbreviation_t;
static ms_glk_abbreviation_t ms_glk_abbreviations[] = {
	{ 'c',	"close" },	{ 'g',	"again" },	{ 'i',	"inventory" },
	{ 'k',	"attack" },	{ 'l',	"look" },	{ 'p',	"open" },
	{ 'q',	"quit" },	{ 'r',	"drop" },	{ 't',	"take" },
	{ 'x',	"examine" },	{ 'y',	"yes" },	{ 'z',	"wait" },
	{ '\0',	NULL }
};

/*
 * ms_glk_expand_abbreviations()
 *
 * Expand a few common one-character abbreviations commonly found in other
 * game systems, but not always normal in Magnetic Scrolls games.
 */
static void
ms_glk_expand_abbreviations (char *buffer, int size)
{
	int			index;		/* Buffer character index. */
	char			*command;	/* Single-character command. */
	ms_glk_abbreviation_t	*entry;		/* Table search entry. */
	assert (buffer != NULL);

	/* Skip leading spaces in command.  If command is empty, return. */
	for (index = 0; buffer[ index ] != '\0'
			&& ms_glk_char_is_whitespace (buffer [ index ]); )
		index++;
	if (buffer[ index ] == '\0')
		return;

	/* Note the start of the real command text. */
	command = &buffer[ index ];

	/* If the command is not a single letter one, do nothing. */
	if (strlen (command) > 1
			&& !ms_glk_char_is_whitespace (command[1]))
		return;

	/* Scan the abbreviations table for a match. */
	for (entry = ms_glk_abbreviations; entry->expansion != NULL; entry++)
	    {
		/* Check for a single character match. */
		if (entry->abbreviation == glk_char_to_lower
						((unsigned char) command[0]))
		    {
			/* Note the expansion equivalent. */
			const char	*expansion = entry->expansion;

			/* Match found, check for a fit. */
			if (strlen (buffer) + strlen (expansion) - 1 >= size)
				return;

			/* Replace the character with the full string. */
			memmove (command + strlen (expansion) - 1,
						command, strlen (buffer) + 1);
			memcpy (command, expansion, strlen (expansion));

			/*
			 * Indicate what action we took, using a style to
			 * hint that this is from the interpreter.
			 */
			ms_glk_message_string ("[");
			ms_glk_message_char   (entry->abbreviation);
			ms_glk_message_string (" -> ");
			ms_glk_message_string (expansion);
			ms_glk_message_string ("]\n");

			/* Return the expanded buffer. */
			return;
		    }
	    }
}


#if 0
/*
 * ms_glk_confirm()
 *
 * Print a confirmation prompt, and read a single input character, taking
 * only [YyNn] input.  If the character is 'Y' or 'y', return TRUE.
 */
static int
ms_glk_confirm (char *prompt)
{
	event_t		event;			/* Glk event buffer. */
	unsigned char	response;		/* Response character. */
	assert (prompt != NULL);

	/*
	 * Print the confirmation prompt, in a style that hints that it's
	 * from the interpreter, not the game.
	 */
	ms_glk_message_string (prompt);

	/* Wait for a single 'Y' or 'N' character. */
	do
	    {
		glk_request_char_event (ms_glk_main_window);
		ms_glk_event_wait (evtype_CharInput, &event);
		response = glk_char_to_upper (event.val1);
	    }
	while (response != 'Y' && response != 'N');

	/* Echo the confirmation response, and a blank line. */
	glk_put_string (response == 'Y' ? "Yes" : "No" );
	glk_put_string ("\n\n");

	/* Return TRUE if 'Y' was entered. */
	return (response == 'Y');
}
#endif


/*
 * ms_getchar()
 *
 * Return the single next character to the interpreter.  If we are taking
 * input from a read log, the function returns character from the assoc-
 * iated Glk stream until empty, otherwise it uses a a buffered keyboard
 * input line, getting a new line into the buffer if it is currently empty.
 *
 * This function makes a special case of some command strings, and will
 * also perform abbreviation expansion.
 */
type8
ms_getchar (void)
{
	static char	buffer[MS_GLK_INPUT_BUFFER_LENGTH];
						/* Line input buffer. */
	static int	position	= 0;	/* Current buffer return. */
	assert (ms_glk_main_window != NULL);

	/*
	 * If we have an input log to read from, use that until it is
	 * exhausted.  On end of file, close the stream and resume input
	 * from line requests.
	 */
	if (ms_glk_readlog_stream != NULL)
	    {
		glsi32		character;	/* Next stream character. */

		/*
		 * Get the next character.  We'll work here character by
		 * character, and rely on Glk for any buffering.
		 */
		character = glk_get_char_stream (ms_glk_readlog_stream);

		/* See if we have reached the end of the log stream. */
		if (character == -1)
		    {
			/* Close the log stream and continue. */
			glk_stream_close (ms_glk_readlog_stream, NULL);
			ms_glk_readlog_stream = NULL;
		    }
		else
		    {
			/*
			 * If we read a character, echo it in input style
			 * and return it.
			 */
			glk_set_style (style_Input);
			glk_put_char (character);
			glk_set_style (style_Normal);
			return character;
		    }
	    }

	/* If there is no buffered data, read in a new line. */
	if (position == 0)
	    {
		event_t		event;		/* Glk event buffer. */
		int		index;		/* Buffer character index. */

		/*
		 * Flush buffered output, and refresh the current status
		 * line display.
		 */
		ms_flush ();
		ms_glk_status_redraw ();

		/*
		 * If it looks like we didn't get a prompt from the game,
		 * do our own.  This happens with Magnetic windows on
		 * empty input lines.
		 */
		if (!ms_glk_game_prompted())
		    {
			ms_glk_normal_char ('\n');
			ms_glk_normal_string (MS_GLK_NORMAL_PROMPT);
		    }

		/* Get a new line from Glk. */
		glk_request_line_event (ms_glk_main_window, buffer,
						sizeof (buffer) - 1, 0);
		ms_glk_event_wait (evtype_LineInput, &event);

		/* Terminate the input line with a NUL. */
		assert (event.val1 <= sizeof (buffer) - 1);
		buffer[ event.val1 ] = '\0';

		/*
		 * If neither abbreviations nor local commands are enabled,
		 * use the data read above without further massaging.
		 */
		if (ms_glk_abbreviations_enabled
					|| ms_glk_commands_enabled)
		    {

			/* Find the first non-space character in the buffer. */
			for (index = 0; buffer[ index ] != '\0'
					&& ms_glk_char_is_whitespace
							(buffer [ index ]); )
				index++;

			/*
			 * If the first non-space input character is a quote,
			 * bypass all abbreviation expansion and local command
			 * recognition, and use the unadulterated input, less
			 * introductory quote.
			 */
			if (buffer[ index ] == MS_GLK_QUOTED_INPUT)
			    {
				/* Delete the quote with memmove(). */
				memmove (buffer, buffer + 1,
						strlen (buffer + 1) + 1);
			    }
			else
			    {
				int	undo_command;	/* Undo flag */

				/*
				 * Check for, and expand, any abbreviated
				 * commands.
				 */
				if (ms_glk_abbreviations_enabled)
					ms_glk_expand_abbreviations
						(buffer, sizeof (buffer));

				/*
				 * Check for Glk port special commands, and if
				 * found then suppress the interpreter's use of
				 * this input.
				 */
				if (ms_glk_commands_enabled
					&& ms_glk_command_escape
							(buffer, &undo_command))
				    {
					/*
					 * Make a special case of "undo"
					 * commands, returning 0 (the special
					 * interpreter "undo" value), else
					 * return newline, leaving position
					 * at zero, so we get a new line read
					 * again on next call.  Happily, the
					 * Magnetic Scrolls interpreter doesn't
					 * complain about empty input lines.
					 */
					return (undo_command ? 0 : '\n');
				    }
			    }
		    }

		/*
		 * If there is an input log active, log this input string
		 * to it.  Note that by logging here we get any abbreviation
		 * expansions but we won't log glk special commands.
		 */
		if (ms_glk_inputlog_stream != NULL)
		    {
			glk_put_string_stream (ms_glk_inputlog_stream, buffer);
			glk_put_char_stream   (ms_glk_inputlog_stream, '\n');
		    }
	    }

	/*
	 * If the next character to return is NUL, we've reached the end of
	 * the current input buffer.  Signal this by returning newline, and
	 * set position to 0 to provoke a new input line read from Glk on
	 * the next call of this function.
	 *
	 * Note that if the next character to return is NUL and position is
	 * zero, this means that the Glk line read above returned no chara-
	 * cters (that is, the user pressed return immediately).  This is
	 * fine; position remains at zero, so on next call, we request more
	 * input from Glk.
	 */
	if (buffer[ position ] == '\0')
	    {
		/* The line input buffer is exhausted. */
		position = 0;
		return '\n';
	    }

	/* Return the next buffered character. */
	assert (position < sizeof (buffer));
	return buffer[ position++ ];
}


/*---------------------------------------------------------------------*/
/*  Glk port event functions                                           */
/*---------------------------------------------------------------------*/

/*
 * ms_glk_event_wait()
 *
 * Process Glk events until one of the expected type arrives.  Return
 * the event of that type.  Some event types are handled locally, and
 * so can't be waited for.
 */
static void
ms_glk_event_wait (glui32 wait_type, event_t *event)
{
	assert (event != NULL);

	/* Get events, until one matches the requested type. */
	do
	    {
		/* Get next event. */
		glk_select (event);

		/* Handle events of interest locally. */
		switch (event->type)
		    {
			/* Refresh any sensitive windows on size events. */
			case evtype_Arrange:
			case evtype_Redraw:
				/* Refresh the status window. */
				ms_glk_status_redraw ();

				/* Refresh any hints display. */
				ms_glk_hint_redraw ();

				/* Trigger graphics repaint, with clear. */
				ms_glk_graphics_paint ();
				break;

			/* Do background graphics updates on timeout. */
			case evtype_Timer:
				ms_glk_graphics_timeout ();
				break;
		    }
	    }
	while (event->type != wait_type);
}


/*---------------------------------------------------------------------*/
/*  Glk port file functions                                            */
/*---------------------------------------------------------------------*/

/* Success and fail return codes from file functions. */
static const int	MS_GLK_FILE_SUCCESS		= 0;
static const int	MS_GLK_FILE_ERROR		= 1;


/*
 * ms_save_file ()
 * os_load_file ()
 *
 * Save the current game state to a file, and load a game state.
 */
type8
ms_save_file (type8s *name, type8 *ptr, type16 size)
{
	assert (ptr != NULL);

	/* If there is no name, use Glk to prompt for one, and save. */
	if (name == NULL)
	    {
		frefid_t	fileref;	/* Glk file reference. */
		strid_t		stream;		/* Glk stream reference. */

		/* Get a Glk file reference for a game save file. */
		fileref = glk_fileref_create_by_prompt
				(fileusage_SavedGame, filemode_Write, 0);
		if (fileref == NULL)
			return MS_GLK_FILE_ERROR;

		/* Open a Glk stream for the fileref. */
		stream = glk_stream_open_file (fileref, filemode_Write, 0);
		if (stream == NULL)
		    {
			glk_fileref_destroy (fileref);
			return MS_GLK_FILE_ERROR;
		    }

		/* Write the game state data. */
		glk_put_buffer_stream (stream, ptr, size);

		/* Close and destroy the Glk stream and fileref. */
		glk_stream_close (stream, NULL);
		glk_fileref_destroy (fileref);
	    }
	else
	    {
		FILE		*stream;	/* Stdio file reference. */

		/*
		 * Open the output file directly.  There's no Glk way to
		 * achieve this.
		 */
		stream = fopen (name, "wb");
		if (stream == NULL)
			return MS_GLK_FILE_ERROR;

		/* Write saved game data. */
		if (fwrite (ptr, 1, size, stream) != size)
		    {
			fclose (stream);
			return MS_GLK_FILE_ERROR;
		    }

		/* Close the stdio stream. */
		fclose (stream);
	    }

	/* All done. */
	return MS_GLK_FILE_SUCCESS;
}

type8
ms_load_file (type8s *name, type8 *ptr, type16 size)
{
	assert (ptr != NULL);

	/* If there is no name, use Glk to prompt for one, and load. */
	if (name == NULL)
	    {
		frefid_t	fileref;	/* Glk file reference. */
		strid_t		stream;		/* Glk stream reference. */

		/* Get a Glk file reference for a game save file. */
		fileref = glk_fileref_create_by_prompt
				(fileusage_SavedGame, filemode_Read, 0);
		if (fileref == NULL)
			return MS_GLK_FILE_ERROR;

		/* Open a Glk stream for the fileref. */
		stream = glk_stream_open_file (fileref, filemode_Read, 0);
		if (stream == NULL)
		    {
			glk_fileref_destroy (fileref);
			return MS_GLK_FILE_ERROR;
		    }

		/* Read back the game state data. */
		glk_get_buffer_stream (stream, ptr, size);

		/* Close and destroy the Glk stream and fileref. */
		glk_stream_close (stream, NULL);
		glk_fileref_destroy (fileref);
	    }
	else
	    {
		FILE		*stream;	/* Stdio file reference. */

		/*
		 * Open the input file directly.  As above, there's no
		 * Glk way to achieve this.
		 */
		stream = fopen (name, "rb");
		if (stream == NULL)
			return MS_GLK_FILE_ERROR;

		/* Read saved game data. */
		if (fread (ptr, 1, size, stream) != size)
		    {
			fclose (stream);
			return MS_GLK_FILE_ERROR;
		    }

		/* Close the stdio stream. */
		fclose (stream);
	    }

	/* All done. */
	return MS_GLK_FILE_SUCCESS;
}


/*---------------------------------------------------------------------*/
/*  Functions intercepted by link-time wrappers                        */
/*---------------------------------------------------------------------*/

/*
 * __wrap_toupper()
 * __wrap_tolower()
 *
 * Wrapper functions around toupper() and tolower().  The Linux linker's
 * --wrap option will convert calls to mumble() to __wrap_mumble() if we
 * give it the right options.  We'll use this feature to translate all
 * toupper() and tolower() calls in the interpreter code into calls to
 * Glk's versions of these functions.
 *
 * It's not critical that we do this.  If a linker, say a non-Linux one,
 * won't do --wrap, then just do without it.  It's unlikely that there
 * will be much noticeable difference.
 */
int
__wrap_toupper (int ch)
{
	unsigned char		uch;

	uch = glk_char_to_upper ((unsigned char) ch);
	return (int) uch;
}
int
__wrap_tolower (int ch)
{
	unsigned char		lch;

	lch = glk_char_to_lower ((unsigned char) ch);
	return (int) lch;
}


/*---------------------------------------------------------------------*/
/*  main() and options parsing                                         */
/*---------------------------------------------------------------------*/

/*
 * The following values need to be passed between the startup_code and main
 * functions.
 */
static char	*ms_glk_gamefile	= NULL;		/* Name of game file. */
static char	*ms_glk_game_message	= NULL;		/* Error message. */


/*
 * ms_glk_nuance_filenames()
 *
 * Given a game name, try to nuance three filenames from it - the main game
 * text file, the (optional) graphics data file, and the (optional) hints
 * file.  Given an input "file" X, the function looks for X.MAG or X.mag for
 * game data, X.GFX or X.gfx for graphics, and X.HNT or X.hnt for hints.
 * If the input file already ends with .MAG, .GFX, or .HNT, the extension
 * is stripped first.
 *
 * The function returns NULL for filenames not available.  It's not fatal
 * if the graphics filename or hints filename is NULL, but it is if the
 * main game filename is NULL.  Filenames are malloc'ed, and need to be
 * freed by the caller.
 *
 * The function uses fopen() rather than access() since fopen() is an
 * ANSI standard function, and access() isn't.
 */
static void
ms_glk_nuance_filenames (char *name, char **text, char **graphics, char **hints)
{
	char		*base;			/* Copy of the base string. */
	char		*text_file;		/* Game text file path. */
	char		*graphics_file;		/* Game graphics file path. */
	char		*hints_file;		/* Game hints file path. */
	FILE		*stream;		/* Test file stream. */
	assert (name != NULL && text != NULL
			&& graphics != NULL && hints != NULL);

	/* First, take a destroyable copy of the input filename. */
	base = ms_glk_malloc (strlen (name) + 1);
	strcpy (base, name);

	/* Now, if base has an extension .MAG, .GFX, or .HNT, remove it. */
	if (strlen (base) > strlen (".XXX"))
	    {
		if (!strcasecmp (base + strlen (base)
						- strlen (".MAG"), ".MAG")
				|| !strcasecmp (base + strlen (base)
						- strlen (".GFX"), ".GFX")
				|| !strcasecmp (base + strlen (base)
						- strlen (".HNT"), ".HNT"))
			base[ strlen (base) - strlen (".XXX") ] = '\0';
	    }

	/* Malloc space for the return text file. */
	text_file = ms_glk_malloc (strlen (base) + strlen (".MAG") + 1);

	/* Form a candidate text file, by adding a .MAG extension. */
	strcpy (text_file, base);
	strcat (text_file, ".MAG");
	stream = fopen (text_file, "rb");
	if (stream == NULL)
	    {
		/* Retry, using a .mag extension instead. */
		strcpy (text_file, base);
		strcat (text_file, ".mag");
		stream = fopen (text_file, "rb");
		if (stream == NULL)
		    {
			/*
			 * No access to a usable game text file.  Return
			 * immediately, without bothering to look for any
			 * associated graphics or hints files.
			 */
			*text		= NULL;
			*graphics	= NULL;
			*hints		= NULL;

			/* Free malloced memory and return. */
			free (text_file);
			free (base);
			return;
		    }
	    }
	if (stream != NULL)
		fclose (stream);

	/* Now malloc space for the return graphics file. */
	graphics_file = ms_glk_malloc (strlen (base) + strlen (".GFX") + 1);

	/* As above, form a candidate graphics file, using a .GFX extension. */
	strcpy (graphics_file, base);
	strcat (graphics_file, ".GFX");
	stream = fopen (graphics_file, "rb");
	if (stream == NULL)
	    {
		/* Retry, using a .gfx extension instead. */
		strcpy (graphics_file, base);
		strcat (graphics_file, ".gfx");
		stream = fopen (graphics_file, "rb");
		if (stream == NULL)
		    {
			/*
			 * No access to any graphics file.  In this case,
			 * free memory and reset graphics file to NULL.
			 */
			free (graphics_file);
			graphics_file = NULL;
		    }
	    }
	if (stream != NULL)
		fclose (stream);

	/* Now malloc space for the return hints file. */
	hints_file = ms_glk_malloc (strlen (base) + strlen (".HNT") + 1);

	/* As above, form a candidate graphics file, using a .HNT extension. */
	strcpy (hints_file, base);
	strcat (hints_file, ".HNT");
	stream = fopen (hints_file, "rb");
	if (stream == NULL)
	    {
		/* Retry, using a .hnt extension instead. */
		strcpy (hints_file, base);
		strcat (hints_file, ".hnt");
		stream = fopen (hints_file, "rb");
		if (stream == NULL)
		    {
			/*
			 * No access to any hints file.  In this case,
			 * free memory and reset hints file to NULL.
			 */
			free (hints_file);
			hints_file = NULL;
		    }
	    }
	if (stream != NULL)
		fclose (stream);

	/* Return the text file, and graphics and hints, which may be NULL. */
	*text		= text_file;
	*graphics	= graphics_file;
	*hints		= hints_file;

	/* Finished with base. */
	free (base);
}


/*
 * ms_glk_startup_code()
 * ms_glk_main()
 *
 * Together, these functions take the place of the original main().
 * The first one is called from glkunix_startup_code(), to parse and
 * generally handle options.  The second is called from glk_main(), and
 * does the real work of running the game.
 */
static int
ms_glk_startup_code (int argc, char *argv[])
{
	int		argv_index;			/* Argument iterator. */

	/* Handle command line arguments. */
	for (argv_index = 1;
		argv_index < argc && argv[ argv_index ][0] == '-'; argv_index++)
	    {
		if (strcmp (argv[ argv_index ], "-nc") == 0)
		    {
			ms_glk_commands_enabled = FALSE;
			continue;
		    }
		if (strcmp (argv[ argv_index ], "-na") == 0)
		    {
			ms_glk_abbreviations_enabled = FALSE;
			continue;
		    }
		if (strcmp (argv[ argv_index ], "-np") == 0)
		    {
			ms_glk_graphics_enabled = FALSE;
			continue;
		    }
		if (strcmp (argv[ argv_index ], "-ng") == 0)
		    {
			ms_glk_gamma_enabled = FALSE;
			continue;
		    }
		return FALSE;
	    }

	/*
	 * Get the name of the game file.  Since we need this in our call
	 * from glk_main, we need to keep it in a module static variable.
	 * If the game file name is omitted, then here we'll set the pointer
	 * to NULL, and complain about it later in main.  Passing the
	 * message string around like this is a nuisance...
	 */
	if (argv_index == argc - 1)
	    {
		ms_glk_gamefile = argv[ argv_index ];
		ms_glk_game_message = NULL;
	    }
	else
	    {
		ms_glk_gamefile = NULL;
		if (argv_index < argc - 1)
			ms_glk_game_message = "More than one game file"
					" was given on the command line.";
		else
			ms_glk_game_message = "No game file was given"
					" on the command line.";
	    }

	/* All startup options were handled successfully. */
	return TRUE;
}

static void
ms_glk_main (void)
{
	char		*text_file	= NULL;	/* Text file path */
	char		*graphics_file	= NULL;	/* Graphics file path */
	char		*hints_file	= NULL;	/* Hints file path */
	int		ms_init_status;		/* ms_init status code */

	/* Create the main Glk window, and set its stream as current. */
	ms_glk_main_window = glk_window_open (0, 0, 0, wintype_TextBuffer, 0);
	if (ms_glk_main_window == NULL)
	    {
		fprintf (stderr,
			"GLK INTERNAL ERROR: can't open main window\n");
		glk_exit ();
	    }
	glk_window_clear (ms_glk_main_window);
	glk_set_window (ms_glk_main_window);
	glk_set_style (style_Normal);

	/* If there's no game found on the command line, complain now. */
	if (ms_glk_gamefile == NULL)
	    {
		assert (ms_glk_game_message != NULL);
		ms_glk_message_string (ms_glk_game_message);
		ms_glk_message_char ('\n');
		glk_exit ();
	    }

	/*
	 * Given the basic game name, try to come up with usable text
	 * and graphics filenames.  The graphics file may be null, but
	 * the text file may not.
	 */
	ms_glk_nuance_filenames (ms_glk_gamefile,
				&text_file, &graphics_file, &hints_file);
	if (text_file == NULL)
	    {
		assert (graphics_file == NULL && hints_file == NULL);
		ms_glk_message_string
				("Error: Unable to find game file\n");
		glk_exit ();
	    }

	/* Set the possibility of pictures depending on graphics file. */
	if (graphics_file != NULL)
	    {
		/*
		 * Check Glk library capabilities, and note pictures are
		 * impossible if the library can't offer both graphics and
		 * timers.  We need timers to create the background
		 * "thread" for picture updates.
		 */
		if (glk_gestalt (gestalt_Graphics, 0)
					&& glk_gestalt (gestalt_Timer, 0))
			ms_glk_graphics_possible = TRUE;
		else
			ms_glk_graphics_possible = FALSE;
	    }
	else
		ms_glk_graphics_possible = FALSE;


	/*
	 * If pictures are impossible, clear pictures enabled flag.  That
	 * is, act as if -np was given on the command line, even though
	 * it may not have been.  If pictures are impossible, they can
	 * never be enabled.
	 */
	if (!ms_glk_graphics_possible)
		ms_glk_graphics_enabled = FALSE;

	/* Try to create a one-line status window.  We can live without it. */
	ms_glk_status_window = glk_window_open (ms_glk_main_window,
					winmethod_Above|winmethod_Fixed,
					1, wintype_TextGrid, 0);

	/* Seed the random number generator. */
	ms_seed (time (NULL));

	/* Print out a short banner. */
	ms_glk_banner_string ("\nMagnetic Scrolls Interpreter, version 2.1\n"
			"Written by Niclas Karlsson\n"
			"Glk interface by Simon Baldwin\n\n");

	/*
	 * Load the game.  If no graphics are possible, then passing the
	 * NULL to ms_init() runs a game without graphics.
	 */
	if (ms_glk_graphics_possible)
	    {
		/* Initialize a game with graphics, and maybe hints. */
		assert (graphics_file != NULL);
		ms_init_status = ms_init (text_file, graphics_file, hints_file);

		/*
		 * Output a warning if graphics failed, but the main game
		 * text initialized okay.
		 */
		if (ms_init_status == 1)
		    {
			ms_glk_message_string
				("Error: Unable to open graphics file\n");
			ms_glk_message_string
				("Continuing without pictures...\n\n");

			/* Reset pictures possible flag. */
			ms_glk_graphics_possible = FALSE;
		    }
	    }
	else
	    {
		/* Initialize a game without graphics, and maybe hints. */
		ms_init_status = ms_init (text_file, NULL, hints_file);
	    }

	/* Look for a complete failure to load the game. */
	if (ms_init_status == 0)
	    {
		ms_glk_message_string ("Error: Unable to open game file\n");

		/* Free the text file path, and any graphics file path. */
		free (text_file);
		if (graphics_file != NULL)
			free (graphics_file);

		/* Free interpreter allocated memory. */
		ms_freemem ();

		/* Nothing more to be done. */
		glk_exit ();
	    }

	/* Run the game opcodes. */
	while (TRUE)
	    {
		/* Execute an opcode - returns FALSE if game stops. */
		if (!ms_rungame ())
			break;
		glk_tick ();
	    }

	/* Turn off any background graphics "thread". */
	ms_glk_graphics_stop ();

	/* Free any temporary memory noted as in use by graphics and hints. */
	if (ms_glk_graphics_temporary != NULL)
	    {
		free (ms_glk_graphics_temporary);
		ms_glk_graphics_temporary = NULL;
	    }
	if (ms_glk_hint_temporary != NULL)
	    {
		free (ms_glk_hint_temporary);
		ms_glk_hint_temporary = NULL;
	    }

	/* Close any open transcript, input log, and/or read log. */
	if (ms_glk_transcript_stream != NULL)
		glk_stream_close (ms_glk_transcript_stream, NULL);
	if (ms_glk_inputlog_stream != NULL)
		glk_stream_close (ms_glk_inputlog_stream, NULL);
	if (ms_glk_readlog_stream != NULL)
		glk_stream_close (ms_glk_readlog_stream, NULL);

	/* Free interpreter allocated memory. */
	ms_freemem ();

	/* Free the text file path, and any graphics file path. */
	free (text_file);
	if (graphics_file != NULL)
		free (graphics_file);
}


/*---------------------------------------------------------------------*/
/*  Linkage between Glk entry/exit calls and the real interpreter      */
/*---------------------------------------------------------------------*/

/*
 * Safety flags, to ensure we always get startup before main, and that
 * we only get a call to main once.
 */
static int		ms_glk_startup_called	= FALSE;
static int		ms_glk_main_called	= FALSE;

/* Additional Mac variables. */
#if TARGET_OS_MAC
static strid_t		ms_glk_mac_gamefile	= NULL;
static short		ms_glk_savedVRefNum	= 0;
static long		ms_glk_savedDirID	= 0;
#endif


#if TARGET_OS_MAC
/*
 * ms_glk_mac_whenselected()
 * ms_glk_mac_whenbuiltin()
 * macglk_startup_code()
 *
 * Startup entry points for Mac versions of Glk interpreter.  Glk will
 * call macglk_startup_code() for details on what to do when the app-
 * lication is selected.  On selection, an argv[] vector is built, and
 * passed to the normal interpreter startup code, after which, Glk will
 * call glk_main().
 */
static Boolean
ms_glk_mac_whenselected (FSSpec *file, OSType filetype)
{
	static char*		argv[2];
	assert (!ms_glk_startup_called);
	ms_glk_startup_called = TRUE;

	/* Set the WD to where the file is, so later fopens work. */
	assert (!HGetVol (0, &ms_glk_savedVRefNum, &ms_glk_savedDirID));
	assert (!HSetVol (0, file->vRefNum, file->parID));

	/* Put a CString version of the PString name into argv[1]. */
	argv[1] = malloc (file->name[0] + 1);
	BlockMoveData (file->name + 1, argv[1], file->name[0]);
	argv[1][file->name[0]] = '\0';
	argv[2] = NULL;

	return ms_glk_startup_code (2, argv);
}

static Boolean
ms_glk_mac_whenbuiltin()
{
	/* Not implemented yet. */
	return true;
}

Boolean
macglk_startup_code (macglk_startup_t *data)
{
	static OSType		ms_glk_mac_gamefile_types[] = { 'MaSc' };

	data->startup_model      = macglk_model_ChooseOrBuiltIn;
	data->app_creator        = 'cAGL';
	data->gamefile_types     = ms_glk_mac_gamefile_types;
	data->num_gamefile_types = sizeof (ms_glk_mac_gamefile_types)
					/ sizeof (*ms_glk_mac_gamefile_types);
	data->savefile_type      = 'BINA';
	data->datafile_type      = 0x3F3F3F3F;
	data->gamefile           = &ms_glk_mac_gamefile;
	data->when_selected      = ms_glk_mac_whenselected;
	data->when_builtin       = ms_glk_mac_whenbuiltin;
	/* macglk_setprefs(); */
	return TRUE;
}
#else /* not TARGET_OS_MAC */


/*
 * glkunix_startup_code()
 *
 * Startup entry point for UNIX versions of Glk interpreter.  Glk will
 * call glkunix_startup_code() to pass in arguments.  On startup, we call
 * our function to parse arguments and generally set stuff up.
 */
int
glkunix_startup_code (glkunix_startup_t *data)
{
	assert (!ms_glk_startup_called);
	ms_glk_startup_called = TRUE;

	return ms_glk_startup_code (data->argc, data->argv);
}
#endif /* TARGET_OS_MAC */


/*
 * glk_main()
 *
 * Main entry point for Glk.  Here, all startup is done, and we call our
 * function to run the game.
 */
void
glk_main (void)
{
	assert (ms_glk_startup_called && !ms_glk_main_called);
	ms_glk_main_called = TRUE;

	/* Call the interpreter main function. */
	ms_glk_main ();
}
