typedef unsigned char  type8;
typedef signed   char  type8s;
typedef unsigned short type16;
typedef signed   short type16s;
typedef unsigned long  type32;
typedef signed   long  type32s;

/*#define SAVEMEM*/	/* Doesn't keep the whole gfx-file in memory */

/* Functions which need to be implemented in main */

/* Load and Save operations. 'name' is a zero terminated string of the
   filename typed by the player, 'ptr' is a pointer to the data to save
   and 'size' is the number of bytes to save. Should return zero in case
   of success. Creative people may put in file requesters. */

type8 ms_load_file(type8 *name, type8 *ptr, type16 size);
type8 ms_save_file(type8 *name, type8 *ptr, type16 size);

/* This function outputs a character as is on the status bar, with the
   following exceptions: 0x0a resets the x position to zero and 0x09
   moves the cursor to the right half of the bar, ie. to 'width'-11 */

void ms_statuschar(type8 c);

/* This function outputs a char. The output should preferably be buffered */

void ms_putchar(type8 c);

/* This function flushes the output buffer for ms_putchar() */

void ms_flush(void);

/* This function is for buffered input. The first time it is called a
   string should be read and then given back one byte at a time (ie. one
   for each call) until a '\n' is reached (which will be the last byte
   sent back before it all restarts) - returning a zero means 'undo' and
   the rest of the line must then be ignored */

type8 ms_getchar(void);

/* Very optional, very obvious. c is the number of the picture */
/* mode==0 means gfx off, mode==1 gfx on thumbnails, mode==2 gfx on normal */

void ms_showpic(type8 c,type8 mode);

/* Extract a picture and return a pointer to a raw bitmap. The arguments
   are: a picture number c, pointers to type16 variables for width and height
   and an array for the palette (type16 pal[16]). Returns false if the operation
   failed or if gfx is disabled. All pictures have 16 colours and the palette
   entries use 4-bit RGB (ie. 0x0RGB). The bitmap is one byte per pixel and
   the data is given starting from the upper left corner. Oh, and the buffer
   will be trashed when the next picture is requested so take care! */

type8 *ms_extract(type8 c, type16 *w, type16 *h, type16 *pal);

/* Functions available from main */

type8 ms_init(type8 *,type8 *);
/* pointers the name of gamefile/gfxfile. Second pointer=0 means no gfx
   returns false if failure, 1 if ok without gfx (or gfx failed) and 2
   with gfx */
type8 ms_rungame(void); /* true if successful */
void ms_freemem(void);	/* free allocated resources */
void status(void);	/* register dump to stderr */
