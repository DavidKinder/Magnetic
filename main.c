/****************************************************************************\
*
* Magnetic Scrolls Interpreter by Niclas Karlsson 1997
*
* (Simple) ANSI interface main.c
*
\****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"

#define width   78

extern type32 i_count;

type8		buffer[80],xpos=0,bufpos=0,log_on=0,ms_gfx_enabled,filename[128];
FILE		*log=0,*log2=0;

type8 ms_load_file(type8 *name, type8 *ptr, type16 size) {
	FILE *fh;
	type8 *realname;

	if (name) realname=name;
	else {
		do {
			printf("Filename: ");
		}	while (!gets(filename));
		realname=filename;
	}
	if (!(fh=fopen(realname,"rb"))) return 1;
	if (fread(ptr,1,size,fh)!=size) return 1;
	fclose(fh);
	return 0;
}

type8 ms_save_file(type8 *name, type8 *ptr, type16 size) {
	FILE *fh;
	type8 *realname;

	if (name) realname=name;
	else {
		do {
			printf("Filename: ");
		}	while (!gets(filename));
		realname=filename;
	}
	if (!(fh=fopen(realname,"wb"))) return 1;
	if (fwrite(ptr,1,size,fh)!=size) return 1;
	fclose(fh);
	return 0;
}

void script_write(type8 c) {
	if (log_on==2 && fputc(c,log)==EOF) {
		printf("[Problem with script file - closing]\n");
		fclose(log);
		log_on=0;
	}
}

void transcript_write(type8 c) {
	if (log2 && c==0x08 && ftell(log2)>0) fseek(log2,-1,SEEK_CUR);
	else if (log2 && fputc(c,log2)==EOF) {
		printf("[Problem with transcript file - closing]\n");
		fclose(log2);
		log2=0;
	}
}

void ms_statuschar(type8 c) {
	static type8 x=0;

	if (c==0x09) {
		while (x+11<width) {
			putchar(0x20);
			x++;
		}
		return;
	}
	if (c==0x0a) {
		x=0;
		putchar(0x0a);
		return;
	}
	printf("\x1b[32m%c\x1b[31m",c);
	x++;
}

void ms_flush(void) {
	type8 j;

	if (!bufpos) return;
	if (xpos+bufpos > width) {
		putchar(0x0a);
		transcript_write(0x0a);
		xpos=0;
	}
	for (j=0;j<bufpos;j++) {
		if (buffer[j]==0x0a) xpos=0;
		if (buffer[j]==0x08) xpos-=2;
		putchar(buffer[j]);
		transcript_write(buffer[j]);
		xpos++;
	}
	bufpos=0;
}

void ms_putchar(type8 c) {
	buffer[bufpos++]=c;
/*	if ((c==0x08) && (bufpos>0)) bufpos--; */
	if ((c==0x20) || (c==0x0a) || (bufpos>=80)) ms_flush();
}

type8 ms_getchar(void) {
	static type8  buf[256];
	static type16 pos=0;
	int c;
	type8  i;

	if (!pos) {				/* Read new line? */
		i=0;
		while (1) {
			if (log_on==1) {								/* Reading from logfile */
				if ((c=fgetc(log))==EOF) {				/* End of log? - turn off */
					log_on=0;
					fclose(log);
					c=getchar();
				} else printf("%c",c);					/* print the char as well */
			} else {
				c=getchar();
				if (c=='#' && !i) {						/* Interpreter command? */
					while((c=getchar())!='\n' && c!=EOF && i<255) buf[i++]=c;
					buf[i]=0;
					c='\n';									/* => Prints new prompt */
					i=0;
					if (!strcmp(buf,"logoff") && log_on==2) {
						printf("[Closing script file]\n");
						log_on=0;
						fclose(log);
					} else if (!strcmp(buf,"undo")) {
						c=0;
					} else {
						printf("[Nothing done]\n");
					}
				}
			}
			script_write(c);
			if (c!='\n') transcript_write(c);
			if (c=='\n' || c==EOF || i==255) break;
			buf[i++]=c;
			if (!c) break;
		}
		buf[i]='\n';
	}
	if ((c=buf[pos++])=='\n' || !c) pos=0;
	return (type8)c;
}

void ms_showpic(type8 c,type8 mode) {
	/*	printf("Display picture [%d]\n",c); */
	/* Insert your favourite picture viewing code here */
	/* mode: 0 gfx off, 1 gfx on (thumbnails), 2 gfx on (normal) */

/*	Small bitmap retrieving example
	type16 w,h,pal[16];
	type8 *raw=0,i;

	raw=ms_extract(c,&w,&h,pal);
	printf("\n\nExtract: [%d] %dx%d",c,w,h);
	for (i=0;i<16;i++) printf(", %3.3x",pal[i]); printf("\n");
	printf("Bitmap at: %8.8x\n",raw);
*/
}

main(int argc, char **argv) {
	type8  running, i, *gamename=0, *gfxname=0;
	type32 dlimit, slimit;

	if (sizeof(type8)!=1 || sizeof(type16)!=2 || sizeof(type32)!=4) {
		fprintf(stderr,"You have incorrect typesizes, please edit "
							"the typedefs and recompile\nor proceed on your"
							"own risk...\n");
		exit(1);
	}
	dlimit=slimit=0xffffffff;
	for (i=1;i<argc;i++) {
		if (argv[i][0]=='-') {
			switch (tolower(argv[i][1])) {
				case 'd':	if (strlen(argv[i])>2) dlimit=atoi(&argv[i][2]);
								else dlimit=0;
								break;
				case 's':	if (strlen(argv[i])>2) slimit=atoi(&argv[i][2]);
								else slimit=655360;
								break;
				case 't':	if (!(log2=fopen(&argv[i][2],"w"))) {
								printf("Failed to open \"%s\" for writing.\n",&argv[i][2]);
						}
								break; 
				case 'r':	if (log=fopen(&argv[i][2],"r")) log_on=1;
								else printf("Failed to open \"%s\" for reading.\n",&argv[i][2]);
								break;
				case 'w':	if (log=fopen(&argv[i][2],"w")) log_on=2;
								else printf("Failed to open \"%s\" for writing.\n",&argv[i][2]);
								break;
				default:		printf("Unknown option -%c, ignoring.\n",argv[i][1]);
			}
		} else if (!gamename) gamename=argv[i];
		else gfxname=argv[i];
	}
	if (!gamename) {
		printf("\nMS-Interpreter - Magnetic Scrolls interpreter V1.0\n\n");
		printf("Usage: %s [options] game [gfxfile]\n\n"
				 "Where the options are:\n"
				 " -dn    activate registerdump (after n instructions)\n"
				 " -rname read script file\n"
				 " -sn    safety mode, exits automatically (after n instructions)\n"
				 " -tname write transcript file\n"
				 " -wname write script file\n\n"
				 "The interpreter commands are:\n"
				 " #undo   obvious - don't use it near are_you_sure prompts\n"
				 " #logoff to turn off script writing\n\n",argv[0]);
		exit(1);
	}

	if (!(ms_gfx_enabled=ms_init(gamename,gfxname))) {
		printf("Couldn't start up game \"%s\".\n",gamename);
		exit(1);
	}
	ms_gfx_enabled--;
	running=1;
	while ((i_count<slimit) && running) {
		if (i_count>=dlimit) status();
		running=ms_rungame();
	}
	if (i_count==slimit) {
		printf("\n\nSafety limit (%d) reached.\n",slimit);
		status();
	}
	ms_freemem();
	if (log_on) fclose(log);
	if (log2) fclose(log2);
	log2=0;
	printf("\nExiting.\n");
	exit(0);
}
