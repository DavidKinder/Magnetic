#define PRGNAME "magjoin"
#define PRGVERSION "1.0"

/*
   magjoin.c

   - composes MAG datafile from 3 input files: CODE, TEXT1, TEXT2
   - Speccy Tape Pawn: use order 9-10-3-4-5-6-7-8, code-str2-str1

   Author: Paul David Doherty <h0142kdd@rz.hu-berlin.de>

   v1.0:  3 Nov 1999  quick hack for Speccy games
 */

/* The Pawn/C64 */
#define UNDOPC 0x003fb0

#if defined(AZTEC_C) || defined(LATTICE)
#define AMIGA
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef __TURBOC__
#include <io.h>
#include <sys\stat.h>
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#endif
#ifdef __GNUC__
#include <sys/stat.h>
#endif
#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#endif

#if defined(__TURBOC__) || defined(__GNUC__) && !defined(__unix__)
#define O_SCRIVERE O_WRONLY|O_BINARY
#define O_LEGGERE O_RDONLY|O_BINARY
#else
#define O_SCRIVERE O_WRONLY
#define O_LEGGERE O_RDONLY
#endif

#define TRUE 1
#define FALSE 0

typedef unsigned char type8;
typedef unsigned short type16;
typedef unsigned long int type32;

int fdi, fdo;
type8 inputbuf[256];
type8 endfile_reached = FALSE;
type16 buflength;

void
write_32 (type32 numb)
{
  inputbuf[3] = numb & 0xff;
  numb >>= 8;
  inputbuf[2] = numb & 0xff;
  numb >>= 8;
  inputbuf[1] = numb & 0xff;
  numb >>= 8;
  inputbuf[0] = numb;
  write (fdo, inputbuf, 4);
}

void
write_16 (type32 numb)
{
  inputbuf[1] = numb & 0xff;
  numb >>= 8;
  inputbuf[0] = numb;
  write (fdo, inputbuf, 2);
}

void
ex (char *error)
{
  fprintf (stderr, PRGNAME ": %s\n", error);
  close (fdi);
  exit (1);
}

void
usage (void)
{
  fprintf (stderr, PRGNAME " v" PRGVERSION ": ");
  fprintf (stderr,
	   "builds Magnetic Scrolls datafile from CODE/STR2/STR1/DICT.\n");
  fprintf (stderr, "(c) 1999 by Paul David Doherty <h0142kdd@rz.hu-berlin.de>\n\n");
  fprintf (stderr, "Usage: " PRGNAME " version\n");
  fprintf (stderr, "  versions: 0 = The Pawn\n");
  fprintf (stderr, "            1 = Guild of Thieves\n");
  fprintf (stderr, "            2 = Jinxter\n");
  fprintf (stderr, "            3 = Corruption, Fish!, Myth\n");
  exit (1);
}

type32 
calc_restart_size (void)
{
  /* this is a rough estimation; the actual restart_size is lower */
  FILE *in;
  type32 rest;

  if ((in = fopen ("code", "rb")) == NULL)
    ex ("could not reopen CODE");

  fseek (in, 6L, SEEK_SET);
  rest = fgetc (in) << 8;
  rest += fgetc (in);
  fclose (in);

  return rest;
}

int
main (int argc, char **argv)
{
  FILE *infile, *outfile;
  type32 codelen, textlen, indexlen, dictlen, maglen;
  type32 stringlen, str1len, str2len, restart_size, undo_pc;
  int version, nodict = 0;

  if (argc != 2)
    usage ();

  version = atoi (argv[1]);

/* get file lengths */
  if ((infile = fopen ("code", "rb")) == NULL)
    ex ("could not open CODE");
  fseek (infile, 0L, SEEK_END);
  codelen = ftell (infile);
  fclose (infile);

  if ((infile = fopen ("str1", "rb")) == NULL)
    ex ("could not open STR1(TEXT)");
  fseek (infile, 0L, SEEK_END);
  textlen = ftell (infile);
  fclose (infile);

  if ((infile = fopen ("str2", "rb")) == NULL)
    ex ("could not open STR2(INDEX)");
  fseek (infile, 0L, SEEK_END);
  indexlen = ftell (infile);
  fclose (infile);

  if ((infile = fopen ("dict", "rb")) == NULL)
    {
      dictlen = 0L;
      nodict = 1;

      fprintf (stderr, "No DICT found, assuming version 0/1\n");
    }
  else
    {
      fseek (infile, 0L, SEEK_END);
      dictlen = ftell (infile);
      fclose (infile);
    }

/* print file length */
  printf ("CODE length:  %08lx ", codelen);
  printf ("(%ld)\n", codelen);
  printf ("STR1 length:  %08lx ", textlen);
  printf ("(%ld)\n", textlen);
  printf ("STR2 length: %08lx ", indexlen);
  printf ("(%ld)\n", indexlen);
  printf ("DICT length: %08lx ", dictlen);
  printf ("(%ld)\n", dictlen);

/* open file */

  if ((fdo = open ("game.mag", O_SCRIVERE | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
    ex ("could not create GAME.MAG");

/* write header */
  write (fdo, "MaSc", 4);

  maglen = 42L + codelen + textlen + indexlen + dictlen;
  write_32 (maglen);		/* datafile size */

  write_32 (42L);		/* header size */
  write_16 ((unsigned long) version);	/* datafile version */
  write_32 (codelen);		/* CODE length */

  write_32 ((textlen > 0x10000) ? 0x10000 : textlen);
  write_32 ((textlen > 0x10000) ? (textlen + indexlen - 0x10000) : indexlen);

  write_32 (dictlen);

  write_32 (textlen);		/* TEXT length */

  restart_size = calc_restart_size ();
  write_32 (restart_size);
/*
   undo_pc=calc_undo_pc();
   write_32 (undo_pc);
 */
  write_32 (UNDOPC);

/* copy files over */

  if ((fdi = open ("code", O_LEGGERE, 0)) == -1)
    ex ("could not reopen CODE");
  endfile_reached = FALSE;
  while (endfile_reached == FALSE)
    {
      buflength = read (fdi, inputbuf, 256);
      if (buflength != 256)
	endfile_reached = TRUE;
      write (fdo, inputbuf, (int) buflength);
    }
  close (fdi);

  if ((fdi = open ("str1", O_LEGGERE, 0)) == -1)
    ex ("could not reopen STR1(TEXT)");
  endfile_reached = FALSE;
  while (endfile_reached == FALSE)
    {
      buflength = read (fdi, inputbuf, 256);
      if (buflength != 256)
	endfile_reached = TRUE;
      write (fdo, inputbuf, (int) buflength);
    }
  close (fdi);

  if ((fdi = open ("str2", O_LEGGERE, 0)) == -1)
    ex ("could not reopen STR2(INDEX)");
  endfile_reached = FALSE;
  while (endfile_reached == FALSE)
    {
      buflength = read (fdi, inputbuf, 256);
      if (buflength != 256)
	endfile_reached = TRUE;
      write (fdo, inputbuf, (int) buflength);
    }
  close (fdi);

  if (nodict == 0)
    {
      if ((fdi = open ("dict", O_LEGGERE, 0)) == -1)
	ex ("could not reopen DICT");
      endfile_reached = FALSE;
      while (endfile_reached == FALSE)
	{
	  buflength = read (fdi, inputbuf, 256);
	  if (buflength != 256)
	    endfile_reached = TRUE;
	  write (fdo, inputbuf, (int) buflength);
	}
      close (fdi);
    }

  close (fdo);
  return 0;
}
