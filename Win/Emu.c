/****************************************************************************\
*
* Magnetic Scrolls Interpreter by Niclas Karlsson 1997
*
* History:
*
* 0.1  123096 : Making the 68000 emulation work. Converting C64 6510 code to
*     -010297   C isn't all that much fun. Don't mention the word "BUG".
*
* 0.2  010397 : Debugging, debugging, debugging... Probably quite a lot left
*               as well. Haven't tried it out in practice yet.
*
* 0.3  010497 : String decoding now works. Trying out on the real thing - more
*               bugs, modest success. Implementing some very specialized
*               instructions. 0xA0FF gives the word nightmare a whole new
*               meaning.
*
* 0.4  010597 : Getting better... More specialized instructions coming up. The
*               output routines are up and running as well. Occasional emulation
*               bugs still show up due to my MS-style program developing
*               techniques ;-) First playable version [The Pawn].
*
* 0.5  010697 : Trying out with Jinxter. Sigh... Back to reverse engineering
*               (10+ new instructions, separate dictionary and two string
*               banks - these guys sure had some space problems).
*
* 0.6  010997 : Guild of Thieves seems to work now as well. Jinxter still has
*               a small problem with the parsing (I know of _one_ different
*               reaction) - probably due to the not-so-nice dict_lookup2().
*               Load/Save implemented.
*
* 0.7  011097 : The last (major) bug is now fixed (an 8-bit variable instead
*               of a 16-bit one messed up the adjectives). Small cleanup.
*
* 0.8  011397 : Now also runs Fish and Corruption. Small status-bar fix.
*
* 0.9  011497 : New single-file format. Improved user friendliness.
*
* 0.91 011597 : More cleaning. Random number range fixed. Other minor bugfixes.
*
* 0.92 011897 : Major dict_lookup() overhaul. Minor cleanups. <= 64K memory
*               blocks. Seems like the file format wasn't that great after all
*               (most suitable for one big block).
*
* 0.93 012097 : New file format. Fast restarts (no loading). UNDO implemented.
*               Subtle but fatal bug in Write_string() removed [Corruption].
*
* 0.94 012397 : Emulation bug (rotations) fixed. Prng slightly improved, or
*               at least changed. :-) Admode $xx(PC,[A|D]x) implemented.
*
* 0.95 012597 : Another one bites the dust - bit operations with register
*               source were broken. __MSDOS__ ifdef inserted (changes by
*               David Kinder). Error reporting improved (initiative by
*               Paul David Doherty).
*
* 0.96 012697 : No flag changes after MOVE xx,Ax (version>1) - Duhh...
*
* 0.97 020497 : Small output handling bug (0x7E-handling) for version>=3
*
* 0.98 040497 : Pain, agony... Another bit operation bug spotted and removed.
*               A difference between version 0 and the rest (MOVEM.W) caused
*               problems with Myth. Also, findproperties behaved badly with
*               bit 14 set (version 3 only?). dict_lookup() didn't recognise
*               composite words properly.
*
* 0.99 041697 : ADDQ/SUBQ xx,Ax doesn't set flags for (version>=3) [corruption]
*               Small dict_lookup() fix (must be checked thoroughly once more).
*               Difference between versions in findproperties caused problems
*               with Jinxter. [light match/unicorn]. Integrated gfx-handling.
*
* 0.9A 050397 : = instead of == caused problems. Sign error in do_findprop()
*                Stupid A0F1 quirk removed. SAVEMEM flag added.
*
* 0.9B 050997 : Small ms_showpic() modification.
*
* 0.9C 051297 : Fixes for running Magnetic Windows games.
*
* 0.9D 080297 : Minor improvements to save some memory. Bug fix: Last picture
*               couldn't be extracted when SAVEMEM was set. Use time() to get
*								initial random seed.
*
* 1.0  082897 : First public release.
*
\****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"

type32	dreg[8],areg[8],i_count,string_size,rseed=0,pc,arg1i,mem_size;
type16	properties,fl_sub,fl_tab,fl_size, fp_tab, fp_size;
type8		zflag,nflag,cflag,vflag,byte1,byte2,regnr,admode,opsize,*arg1,*arg2;
type8		is_reversible,running=0,tmparg[4]={0,0,0,0},lastchar=0,version=0;
type8		*decode_table,*restart=0,*code=0,*string=0,*string2=0,*dict=0;
type8		quick_flag=0,*gfx_buf=0,*gfx_data=0;
FILE		*gfx_fp=0;

static type8 undo_ok[]="\n[Previous turn undone.]";
static type8 undo_fail[]="\n[You can't \"undo\" what hasn't been done!]";
type32   undo_regs[2][18],undo_pc, undo_size;
type8		*undo[2]={0,0},undo_stat[2]={0,0};

/* a few prototypes - more to come... */
type32 read_reg(int, int);
void write_reg(int, int, type32);
void internal(type8 *);

/* Convert virtual pointer to effective pointer */

type8 *effective(type32 ptr) {
	if (version<4) return &(code[ptr & 0xffff]);
	if (ptr>=mem_size) internal("Outside memory experience");
	return &(code[ptr]);
}

type32 read_l(type8 *ptr) {
	return (type32)ptr[0]<<24|(type32)ptr[1]<<16|(type32)ptr[2]<<8|(type32)ptr[3];
}

type16 read_w(type8 *ptr) {
	return (type16)(ptr[0]<<8 | ptr[1]);
}

void write_l(type8 *ptr, type32 val) {
	ptr[3]=(type8)val; val>>=8;
	ptr[2]=(type8)val; val>>=8;
	ptr[1]=(type8)val; val>>=8;
	ptr[0]=(type8)val;
}

void write_w(type8 *ptr, type16 val) {
	ptr[1]=(type8)val; val>>=8;
	ptr[0]=(type8)val;
}

/* Standard rand - for equal cross-platform behaviour */

void srand_emu(type32 seed) {
	rseed=seed;
}

type32 rand_emu(void) {
	rseed=1103515245*rseed+12345;
	return rseed & 0x7fffffff;
}

void ms_freemem(void) {
	if (code) free(code);
	if (string) free(string);
	if (string2) free(string2);
	if (dict) free(dict);
	if (undo[0]) free(undo[0]);
	if (undo[1]) free(undo[1]);
	if (restart) free(restart);
	code=string=string2=dict=undo[0]=undo[1]=restart=0;
	if (gfx_data) free(gfx_data);
	if (gfx_buf) free(gfx_buf);
	if (gfx_fp) fclose(gfx_fp);
	gfx_data=gfx_buf=0;
	gfx_fp=0;
}

/* zero all registers and flags and load the game */

type8 ms_init(type8 *name,type8 *gfxname) {
	FILE		*fp;
	type8		header[42],header2[8];
	type32	i,dict_size,string2_size,code_size,dec;

	running=0;
	if (!name) {
		if (!restart) return 0;
		else {
			memcpy(code,restart,undo_size);
			undo_stat[0]=undo_stat[1]=0;
			ms_showpic(0,0);
		}
	} else {
		srand_emu(time(0));
		if (!(fp=fopen(name,"rb"))) return 0;
		if ((fread(header,1,42,fp)!=42) || (read_l(header)!=0x4d615363)) {
			fclose(fp);
			return 0;
		}
		if (read_l(header+8)!=42) {			/* Bad style, but it works for now */
			fclose(fp);	
			return 0;
		}
		ms_freemem();
		version=header[13];
		code_size=read_l(header+14);
		string_size=read_l(header+18);
		string2_size=read_l(header+22);
		dict_size=read_l(header+26);
		undo_size=read_l(header+34);
		undo_pc=read_l(header+38);

		mem_size=(version<4)?65536:code_size;
#ifdef __MSDOS__
		if (code_size == 65536) code_size--;
		if (mem_size == 65536) mem_size--;
#endif
		if (!(code=malloc(mem_size)) || !(string=malloc(string_size)) ||
			 !(string2=malloc(string2_size)) || !(restart=malloc(undo_size)) ||
			 ((version>1) && !(dict=malloc(dict_size)))) {
			ms_freemem();
			fclose(fp);
			return 0;
		}
		if (!(undo[0]=malloc(undo_size)) || !(undo[1]=malloc(undo_size))) {
			ms_freemem();
			fclose(fp);
			return 0;
		}
		if (fread(code,1,code_size,fp)!=code_size) {
			ms_freemem();
			fclose(fp);
			return 0;
		}
		if (code_size == 65535) fgetc(fp);
		memcpy(restart,code,undo_size);				/* fast restarts */
		if (fread(string,1,string_size,fp)!=string_size) {
			ms_freemem();
			fclose(fp);
			return 0;
		}
		if (fread(string2,1,string2_size,fp)!=string2_size) {
			ms_freemem();
			fclose(fp);
			return 0;
		}
		if ((version>1) && fread(dict,1,dict_size,fp)!=dict_size) {
			ms_freemem();
			fclose(fp);
			return 0;
		}
		dec=read_l(header+30);
		if (dec>=string_size) decode_table=string2+dec-string_size;
		else decode_table=string+dec;
	}

	for (i=0;i<8;i++) dreg[i]=areg[i]=0;
	write_reg(8+7,2,0xfffe);		/* Stack-pointer, -2 due to MS-DOS segments */
	pc=0;
	zflag=nflag=cflag=vflag=0;
	i_count=0;
	running=1;

	if (!name) return (type8)(gfx_buf ? 2 : 1);			/* Restarted */

	if (!gfxname || !(gfx_fp=fopen(gfxname,"rb"))) return 1;
	if (fread(&header2,1,8,gfx_fp)!=8 || read_l(header2)!=0x4D615069) {
		fclose(gfx_fp);
		gfx_fp=0;
		return 1;
	}
	if (!(gfx_buf=malloc(51200))) {
		fclose(gfx_fp);
		gfx_fp=0;
		return 1;
	}
#ifdef SAVEMEM
	if (!(gfx_data=malloc(128))) {
#else
	if (!(gfx_data=malloc(read_l(header2+4)-8))) {
#endif
		free(gfx_buf);
		fclose(gfx_fp);
		gfx_buf=0;
		gfx_fp=0;
		return 1;
	}
#ifdef SAVEMEM
	if (!fread(gfx_data,128,1,gfx_fp)) {
#else
	if (!fread(gfx_data,read_l(header2+4)-8,1,gfx_fp)) {
#endif
		free(gfx_data);
		free(gfx_buf);
		fclose(gfx_fp);
		gfx_data=gfx_buf=0;
		gfx_fp=0;
		return 1;
	}

#ifdef SAVEMEM
	for (i=0;i<128;i+=4)
		if (!read_l(gfx_data+i))
			write_l(gfx_data+i,read_l(header2+4));
#else
	fclose(gfx_fp);
	gfx_fp=0;
#endif

	return 2;
}

type8 is_blank(type16 line, type16 width) {
	type16 i;

	for (i=line*width;i<(line+1)*width;i++) if (gfx_buf[i]) return 0;
	return 1;
}

type8 *ms_extract(type8 pic,type16 *w, type16 *h, type16 *pal) {
	type8  *decode_table,c,*data,bit,val,*buffer;
	type16 tablesize,count;
	type32 i,j,datasize,upsize,offset;

	if (!gfx_buf) return 0;

	offset=read_l(gfx_data+4*pic);
#ifdef SAVEMEM
	if (fseek(gfx_fp,offset,SEEK_SET)<0) return 0;
	datasize=read_l(gfx_data+4*(pic+1))-offset;
	if (!(buffer=malloc(datasize))) return 0;
	if (fread(buffer,1,datasize,gfx_fp)!=datasize) return 0;
#else
	buffer=gfx_data+offset-8;
#endif

	for (i=0;i<16;i++) pal[i]=read_w(buffer+0x1c+2*i);
	w[0]=read_w(buffer+4)-read_w(buffer+2);
	h[0]=read_w(buffer+6);

	tablesize=read_w(buffer+0x3c);
	datasize=read_l(buffer+0x3e);
	decode_table=buffer+0x42;
	data=decode_table+tablesize*2+2;
	upsize=h[0]*w[0];

	for (i=0,j=0,count=0,val=0,bit=7;i<upsize;i++,count--) {
		if (!count) {
			count=tablesize;
			while (count<0x80) {
				if (data[j] & (1<<bit)) count=decode_table[2*count];
				else count=decode_table[2*count+1];
				if (!bit) j++;
				bit=bit?bit-1:7;
			}
			count&=0x7f;
			if (count>=0x10) count-=0x10;
			else {
				val=count;
				count=1;
			}
		}
		gfx_buf[i]=val;
	}
	for (j=w[0];j<upsize;j++) gfx_buf[j]^=gfx_buf[j-w[0]];

#ifdef SAVEMEM
	free(buffer);
#endif
	for (;h[0]>0 && is_blank(h[0]-1,w[0]);h[0]--);
	for (i=0;h[0]>0 && is_blank(i,w[0]);h[0]--,i++);
	return gfx_buf+i*w[0];
}

void save_undo(void) {
	type8 *tmp,i;
	type32 tmp32;

	tmp=undo[0];						/* swap buffers */
	undo[0]=undo[1];
	undo[1]=tmp;

	for (i=0;i<18;i++) {
		tmp32=undo_regs[0][i];
		undo_regs[0][i]=undo_regs[1][i];
		undo_regs[1][i]=tmp32;
	}

	memcpy(undo[1],code,undo_size);
	for (i=0;i<8;i++) {
		undo_regs[1][i]=dreg[i];
		undo_regs[1][8+i]=areg[i];
	}
	undo_regs[1][16]=i_count;
	undo_regs[1][17]=pc;				/* status flags intentionally omitted */

	undo_stat[0]=undo_stat[1];
	undo_stat[1]=1;
}

type8 ms_undo(void) {
	type8 i;

	ms_flush();
	if (!undo_stat[0]) return 0;

	undo_stat[0]=undo_stat[1]=0;
	memcpy(code,undo[0],undo_size);
	for (i=0;i<8;i++) {
		dreg[i]=undo_regs[0][i];
		areg[i]=undo_regs[0][8+i];
	}
	i_count=undo_regs[0][16];
	pc=undo_regs[0][17];				/* status flags intentionally omitted */
	return 1;
}

void status(void) {
	int j;

#ifndef blah
	fprintf(stderr,"D0:");
	for (j=0;j<8;j++) fprintf(stderr," %8.8x",read_reg(j,3));
	fprintf(stderr,"\nA0:");
	for (j=0;j<8;j++) fprintf(stderr," %8.8x",read_reg(8+j,3));
	fprintf(stderr,"\nPC=%5.5x (%8.8x) ZCNV=%d%d%d%d - %d instructions\n",
				pc,code,zflag&1,cflag&1,nflag&1,vflag&1,i_count);
#else
	printf("D0:");
	for (j=0;j<8;j++) printf(" %8.8x",read_reg(j,3));
	printf("\nA0:");
	for (j=0;j<8;j++) printf(" %8.8x",read_reg(8+j,3));
	printf("\nPC=%5.5x (%8.8x) ZCNV=%d%d%d%d - %d instructions\n",
				pc,code,zflag&1,cflag&1,nflag&1,vflag&1,i_count);
#endif
}

#ifndef __MSDOS__

void internal(type8 *txt) {

	fprintf(stderr,"[** Internal Error%s%s **]\n\n",txt?": ":"!",txt?txt:(type8 *)"");
	status();
	running=0;
}

#endif

/* align register pointer for word/byte accesses */

type8 *reg_align(type8 *ptr, type8 size) {
	if (size==1) ptr+=2;
	if (size==0) ptr+=3;
	return ptr;
}

type32 read_reg(int i, int s) {
	type8 *ptr;

	if (i>15) {
		internal("invalid register in read_reg");
		return 0;
	}
	if (i<8) ptr=(type8 *)&dreg[i];
	else ptr=(type8 *)&areg[i-8];

	switch (s) {
		case 0:	return reg_align(ptr,0)[0];
		case 1:	return read_w(reg_align(ptr,1));
		default:	return read_l(ptr);
	}	
}

void write_reg(int i, int s, type32 val) {
	type8 *ptr;

	if (i>15) {
		internal("invalid register in write_reg");
		return;
	}
	if (i<8) ptr=(type8 *)&dreg[i];
	else ptr=(type8 *)&areg[i-8];

	switch (s) {
		case 0:	reg_align(ptr,0)[0]=val;
					break;
		case 1:	write_w(reg_align(ptr,1),val);
					break;
		default:	write_l(ptr,val);
					break;
	}	
}

/* [35c4] */

void char_out(type8 c) {
	static type8 big=0,period=0,pipe=0;

	if (c==0xff) {
		big=1;
		return;
	}
	c&=0x7f;
	if (read_reg(3,0)) {
		if (c==0x5f) c=0x20;
		if ((c>='a') && (c<='z')) c&=0xdf;
		ms_statuschar(c);
		return;
	}
	if (c==0x5e) c=0x0a;
	if (c==0x40) {
		if (read_reg(2,0)) return;
		else c=0x73;
	}
	if (version<3 && c==0x7e) {
		lastchar=0x7e;
		c=0x0a;
	}
	if (((c>0x40) && (c<0x5b)) || ((c>0x60) && (c<0x7b))) {
		if (big) {
			c&=0xdf;
			big=0;
		}
		if (period) char_out(0x20);
	}
	period=0;
	if ((c==0x2e) || (c==0x3f) || (c==0x21) || (c==0x0a)) big=1;
	if (((c==0x20) || (c==0x0a)) && (c==lastchar)) return;
	if (version<3) {
		if (pipe) {
			pipe=0;
			return;
		}
		if (c==0x7c) {
			pipe=1;
			return;
		}
	} else {
		if (c==0x7e) {
			c=0x0a;
			if (lastchar!=0x0a) char_out(0x0a);
		}
	}
	lastchar=c;
	if (c==0x5f) c=0x20;
	if ((c==0x2e)||(c==0x2c)||(c==0x3b)||(c==0x3a)||(c==0x21)||(c==0x3f)) {
		period=1;
	}
	ms_putchar(c);
}


/* extract addressing mode information [1c6f] */

void set_info(type8 b) {
	regnr=b & 0x07;
	admode=(b>>3) & 0x07;
	opsize=(b>>6);
}

/* read a word and increase pc */

void read_word(void) {
	type8 *epc;

	epc=effective(pc);
	byte1=epc[0];
	byte2=epc[1];
	pc+=2;
}

/* get addressing mode and set arg1 [1c84] */

void set_arg1(void) {
	type8    tmp[2],l1c;

	is_reversible=1;
	switch (admode) {
case 0:	arg1=reg_align((type8 *)&dreg[regnr],opsize);	/* Dx */
			is_reversible=0;
			break;
case 1:	arg1=reg_align((type8 *)&areg[regnr],opsize);	/* Ax */
			is_reversible=0;
			break;
case 2:	arg1i=read_reg(8+regnr,2);							/* (Ax) */
			break;
case 3:	arg1i=read_reg(8+regnr,2);							/* (Ax)+ */
			write_reg(8+regnr,2,read_reg(8+regnr,2)+(1<<opsize));
			break;
case 4:	write_reg(8+regnr,2,read_reg(8+regnr,2)-(1<<opsize));
			arg1i=read_reg(8+regnr,2);							/* -(Ax) */
			break;
case 5:	arg1i=read_reg(8+regnr,2)+(type16s)read_w(effective(pc));
			pc+=2;												/* offset.w(Ax) */
			break;
case 6:  tmp[0]=byte1; tmp[1]=byte2;
			read_word();							/* offset.b(Ax, Dx/Ax) [1d1c] */
			arg1i=read_reg(regnr+8,2)+(type8s)byte2;
			if (byte1 & 0x08) arg1i+=(type32s)read_reg((byte1>>4),2);
			else arg1i+=(type16s)read_reg((byte1>>4),1);
			byte1=tmp[0]; byte2=tmp[1];
			break;
case 7:												/* specials */
		switch(regnr) {

	case 0:	arg1i=read_w(effective(pc));				/* $xxxx.W */
				pc+=2;
				break;
	case 1:	arg1i=read_l(effective(pc));				/* $xxxx */
				pc+=4;
				break;
	case 2:	arg1i=(type16s)read_w(effective(pc))+pc;		/* $xxxx(PC) */
				pc+=2;
				break;
	case 3:	l1c=effective(pc)[0];						/* $xx(PC,A/Dx) */
				if (l1c & 0x08) arg1i=pc+(type32s)read_reg((l1c>>4),2);
				else arg1i=pc+(type16s)read_reg((l1c>>4),1);
				l1c=effective(pc)[1];
				pc+=2;
				arg1i+=(type8s)l1c;
				break;
	case 4:	arg1i=pc;										/* #$xxxx */
				if (opsize==0) arg1i+=1;
				pc+=2;
				if (opsize==2) pc+=2;
				break;
		}
			break;
	}
	if (is_reversible) arg1=effective(arg1i);
}

/* get addressing mode and set arg2 [1bc5] */

void set_arg2(int use_dx, type8 b) {		/* c=0 corresponds to use_dx */

	if (use_dx) arg2=(type8 *)dreg;
	else	arg2=(type8 *)areg;
	arg2+=(b & 0x0e) << 1;
	arg2=reg_align(arg2,opsize);
}

/* [1b9e] */

void swap_args(void) {
	type8 *tmp;

	tmp=arg1;
	arg1=arg2;
	arg2=tmp;
}

/* [1cdc] */

void push(type32 c) {

	write_reg(15,2,read_reg(15,2)-4);
	write_l(effective(read_reg(15,2)),c);
}

/* [1cd1] */

type32 pop(void) {
	type32 c;

	c=read_l(effective(read_reg(15,2)));
	write_reg(15,2,read_reg(15,2)+4);
	return c;
}

/* check addressing mode and get argument [2e85] */

void get_arg(void) {

	set_info(byte2);
	arg2=effective(pc);
	pc+=2;
	if (opsize==2) pc+=2;
	if (opsize==0) arg2+=1;
	set_arg1();
}

void set_flags(void) {
	type16 i;
	type32 j;

	zflag=nflag=0;
	switch (opsize) {

case 0:	if (arg1[0]>127) nflag=0xff;
			if (arg1[0]==0) zflag=0xff;
			break;
case 1:	i=read_w(arg1);
			if (i==0) zflag=0xff;
			if ((i>>15)>0) nflag=0xff;
			break;
case 2:	j=read_l(arg1);
			if (j==0) zflag=0xff;
			if ((j>>31)>0) nflag=0xff;
			break;
	}
}

/* [263a] */

int condition(type8 b) {
	switch (b & 0x0f) {
		case 0:	return 0xff;
		case 1:	return 0x00;
		case 2:	return (zflag | cflag) ^ 0xff;
		case 3:	return (zflag | cflag);
		case 4:	return cflag ^ 0xff;
		case 5:	return cflag;
		case 6:	return zflag ^ 0xff;
		case 7:	return zflag;
		case 8:	return vflag ^ 0xff;
		case 9:	return vflag;
		case 10:
		case 12: return nflag ^ 0xff;
		case 11:
		case 13: return nflag;
		case 14: return (zflag | nflag) ^ 0xff;
		case 15: return (zflag | nflag);
	}
}

/* [26dc] */

void branch(type8 b) {
	if (b==0) pc+=(type16s)read_w(effective(pc));
	else pc+=(type8s)b;
}

/* [2869] */

void do_add(void) {
	cflag=0;
	if (opsize==0) {
		arg1[0]+=arg2[0];
		if (arg2[0] > arg1[0]) cflag=0xff;
	}
	if (opsize==1) {
		write_w(arg1,read_w(arg1)+read_w(arg2));
		if (read_w(arg2) > read_w(arg1)) cflag=0xff;
	}
	if (opsize==2) {
		write_l(arg1,read_l(arg1)+read_l(arg2));
		if (read_l(arg2) > read_l(arg1)) cflag=0xff;
	}
	if (version<3 || !quick_flag) {			/* Corruption onwards */
		vflag=0;
		set_flags();
	}
}

/* [2923] */

void do_sub(void) {
	cflag=0;
	if (opsize==0) {
		if (arg2[0] > arg1[0]) cflag=0xff;
		arg1[0]-=arg2[0];
	}
	if (opsize==1) {
		if (read_w(arg2) > read_w(arg1)) cflag=0xff;
		write_w(arg1,read_w(arg1)-read_w(arg2));
	}
	if (opsize==2) {
		if (read_l(arg2) > read_l(arg1)) cflag=0xff;
		write_l(arg1,read_l(arg1)-read_l(arg2));
	}
	if (version<3 || !quick_flag) {			/* Corruption onwards */
		vflag=0;
		set_flags();
	}
}

/* [283b] */

void do_eor(void) {
	if (opsize==0) arg1[0]^=arg2[0];
	if (opsize==1) write_w(arg1,read_w(arg1)^read_w(arg2));
	if (opsize==2) write_l(arg1,read_l(arg1)^read_l(arg2));
	cflag=vflag=0;
	set_flags();
}

/* [280d] */

void do_and(void) {
	if (opsize==0) arg1[0]&=arg2[0];
	if (opsize==1) write_w(arg1,read_w(arg1)&read_w(arg2));
	if (opsize==2) write_l(arg1,read_l(arg1)&read_l(arg2));
	cflag=vflag=0;
	set_flags();
}

/* [27df] */

void do_or(void) {
	if (opsize==0) arg1[0]|=arg2[0];
	if (opsize==1) write_w(arg1,read_w(arg1)|read_w(arg2));
	if (opsize==2) write_l(arg1,read_l(arg1)|read_l(arg2));
	cflag=vflag=0;
	set_flags();				/* [1c2b] */
}

/* [289f] */

void do_cmp(void) {
	type8 *tmp;

	tmp=arg1;
	tmparg[0]=arg1[0];
	tmparg[1]=arg1[1];
	tmparg[2]=arg1[2];
	tmparg[3]=arg1[3];
	arg1=tmparg;
	quick_flag=0;
	do_sub();
	arg1=tmp;
}

/* [2973] */

void do_move(void) {

	if (opsize==0) arg1[0]=arg2[0];
	if (opsize==1) write_w(arg1,read_w(arg2));
	if (opsize==2) write_l(arg1,read_l(arg2));
	if (version<2 || admode!=1) {			/* Jinxter: no flags if destination Ax */
		cflag=vflag=0;
		set_flags();
	}
}

type8 do_btst(type8 a) {

	a&=admode?0x7:0x1f;
	while (admode==0 && a>=8) {
		a-=8;
		arg1-=1;
	}
	zflag=0;
	if ((arg1[0] & (1<<a))==0) zflag=0xff;
	return a;
}

/* bit operation entry point [307c] */

void do_bop(type8 b, type8 a) {

	b=b & 0xc0;
	a=do_btst(a);
	if (b==0x40) arg1[0]^=(1<<a);				/* bchg */
	if (b==0x80) arg1[0]&=((1<<a)^0xff);	/* bclr */
	if (b==0xc0) arg1[0]|=(1<<a);				/* bset */
}

void check_btst(void) {

	set_info(byte2 & 0x3f);
	set_arg1();
	set_arg2(1,byte1);
	do_bop(byte2,arg2[0]);	
}

void check_lea(void) {

	if ((byte2 & 0xc0) == 0xc0) {
		set_info(byte2);
		opsize=2;
		set_arg1();
		set_arg2(0,byte1);
		write_w(arg2,0);
		if (is_reversible) write_l(arg2,arg1i);
		else internal("illegal addressing mode for LEA");
	} else {
		internal("unimplemented instruction CHK");
	}
}

/* [33cc] */

void check_movem(void) {
	type8 l1c;

	set_info(byte2-0x40);
	read_word();
	for (l1c=0;l1c<8;l1c++) {
		if (byte2 & 1<<l1c ) {
			set_arg1();
			if (opsize==2) write_l(arg1,read_reg(15-l1c,2));
			if (opsize==1) write_w(arg1,read_reg(15-l1c,1));
		}
	}
	for (l1c=0;l1c<8;l1c++) {
		if (byte1 & 1<<l1c ) {
			set_arg1();
			if (opsize==2) write_l(arg1,read_reg(7-l1c,2));
			if (opsize==1) write_w(arg1,read_reg(7-l1c,1));
		}
	}
}

/* [3357] */

void check_movem2(void) {
	type8 l1c;

	set_info(byte2-0x40);
	read_word();
	for (l1c=0;l1c<8;l1c++) {
		if (byte2 & 1<<l1c ) {
			set_arg1();
			if (opsize==2) write_reg(l1c,2,read_l(arg1));
			if (opsize==1) write_reg(l1c,1,read_w(arg1));
		}
	}
	for (l1c=0;l1c<8;l1c++) {
		if (byte1 & 1<<l1c ) {
			set_arg1();
			if (opsize==2) write_reg(8+l1c,2,read_l(arg1));
			if (opsize==1) write_reg(8+l1c,1,read_w(arg1));
		}
	}
}

/* [30e4] in Jinxter, ~540 lines of 6510 spaghetti-code */
/* The mother of all bugs, but hey - no gotos used :-) */

void dict_lookup(void) {

type16	dtab,doff,output,output_bak,bank,word,output2;
type16	tmp16,i, obj_adj, adjlist, adjlist_bak;
type8		c,c2,flag,matchlen,longest,flag2;
type8		sd,restart=0,accept=0;

/*
	dtab=A5.W		;dict_table offset <L22>
	output=output_bak=A2.W	;output <L24>
	A5.W=A6.W		;input word
	doff=A3.W		;lookup offset (doff) <L1C>
	adjlist=A0.W	;adjlist <L1E>
*/

	dtab=read_reg(8+5,1);			/* used by version>0 */
	output=read_reg(8+2,1);
	write_reg(8+5,1,read_reg(8+6,1));
	doff=read_reg(8+3,1);
	adjlist=read_reg(8+0,1);
	sd=(version>1)?1:0;				/* if (sd) => separate dict */

	bank=read_reg(6,0);	/* l2d */
	flag=0;					/* l2c */
	word=0;					/* l26 */
	matchlen=0;				/* l2e */
	longest=0;				/* 30e2 */
	write_reg(0,1,0);		/* apostroph */

	while ((c=sd?dict[doff]:effective(doff)[0])!=0x81) {
		if (c>=0x80) {
			if (c==0x82) {
				flag=matchlen=0;
				word=0;
				write_reg(8+6,1,read_reg(8+5,1));
				bank++;
				doff++;
				continue;
			}
			c&=0x5f; c2=effective(read_reg(8+6,1))[0] & 0x5f;
			if (c2==c) {
				write_reg(8+6,1,read_reg(8+6,1)+1);
				c=effective(read_reg(8+6,1))[0];
				if ((!c) || (c==0x20) || (c==0x27) || (!version && (matchlen>6))) {
					if (c==0x27) {
						write_reg(8+6,1,read_reg(8+6,1)+1);
						write_reg(0,1,0x200+effective(read_reg(8+6,1))[0]);
					}
					accept=1;
				} else restart=1;
			} else if (!version && matchlen>6 && !c2) accept=1;
			else restart=1;
		} else {
			c&=0x5f; c2=effective(read_reg(8+6,1))[0] & 0x5f;
			if ((c2==c && c) || (version && !c2 && (c==0x5f))) {
				if (version && !c2 && (c==0x5f)) flag=0x80;
				matchlen++;
				write_reg(8+6,1,read_reg(8+6,1)+1);
				doff++;
			} else if (!version && matchlen>6 && !c2) accept=1;
			else restart=1;
		}
		if (accept) {
			effective(read_reg(8+2,1))[0]=(version) ? flag : 0;
			effective(read_reg(8+2,1))[1]=bank;
			write_w(effective(read_reg(8+2,1)+2),word);
			write_reg(8+2,1,read_reg(8+2,1)+4);
			if (matchlen >= longest) longest=matchlen;
			restart=1; accept=0;
		}
		if (restart) {
			write_reg(8+6,1,read_reg(8+5,1));
			flag=matchlen=0;
			word++;
			if (sd) while (dict[doff++]<0x80);
			else while(effective(doff++)[0]<0x80);
			restart=0;
		}
	}
	write_w(effective(read_reg(8+2,1)),0xffff);

	if (version) {									/* version > 0 */
		output_bak=output;						/* check synonyms */
		while ((c=effective(output)[1])!=0xff) {
			if (c==0x0b) {
				if (sd) tmp16=read_w(&dict[dtab+read_w(effective(output+2))*2]);
				else tmp16=read_w(effective(dtab+read_w(effective(output+2))*2));
				effective(output)[1]=tmp16 & 0x1f;
				write_w(effective(output+2),tmp16>>5);
			}
			output+=4;
		}
		output=output_bak;
	}

/* l22 = output2,     l1e = adjlist, l20 = obj_adj, l26 = word, l2f = c2 */
/* l1c = adjlist_bak, 333C = i,      l2d = bank,    l2c = flag, l30e3 = flag2 */

	write_reg(1,1,0);						/* D1.W=0  [32B5] */
	flag2=0;
	output_bak=output;
	output2=output;
	while ((bank=effective(output2)[1])!=0xff) {
		obj_adj=read_reg(8+1,1);		/* A1.W - obj_adj, ie. adjs for this word */
		write_reg(1,0,0);							/*	D1.B=0 */
		flag=effective(output2)[0];				/* flag */
		word=read_w(effective(output2+2));		/* wordnumber */
		output2+=4;										/* next match */
		if ((read_w(effective(obj_adj))) && (bank==6)) {	/* Any adjectives? */
			if (i=word) {										/* Find list of valid adjs */
				do {
					while (effective(adjlist++)[0]);
				} while (--i>0);
			}
			adjlist_bak=adjlist;
			do {
				adjlist=adjlist_bak;
				c2=effective(obj_adj)[1];					/* given adjective */
				if (tmp16=read_w(effective(obj_adj))) {
					obj_adj+=2;
					while ((c=effective(adjlist++)[0]) && (c-3!=c2));
					if (c-3!=c2) write_reg(1,0,1);		/* invalid adjective */
				}
			} while (tmp16 && !read_reg(1,0));
			adjlist=read_reg(8+0,1);
		}
		if (!read_reg(1,0)) {				/* invalid_flag */
			flag2|=flag;
			effective(output)[0]=flag2;
			effective(output)[1]=bank;
			write_w(effective(output+2),word);
			output+=4;
		}
	}
	write_reg(8+2,1,output);
	output=output_bak;

	if (flag2 & 0x80) {
		tmp16=output;
		output-=4;
		do {
			output+=4;
			c=effective(output)[0];
		} while (!(c & 0x80));
		write_l(effective(tmp16),read_l(effective(output))&0x7fffffff);
		write_reg(8+2,2,tmp16+4);
		if (longest>1) {
			write_reg(8+5,1,read_reg(8+5,1)+longest-2);
		}
	}
	write_reg(8+6,1,read_reg(8+5,1)+1);
}

/* A0=findproperties(D0) [2b86], properties_ptr=[2b78] A0FE */

void do_findprop(void) {
	type16 tmp;

	if ((version>2) && ((read_reg(0,1) & 0x3fff)>fp_size)) {
		tmp=((fp_size-(read_reg(0,1) & 0x3fff)) ^ 0xffff) << 1;
		tmp+=fp_tab;
		tmp=read_w(effective(tmp));
	} else {
		if (version<2) write_reg(0,2,read_reg(0,2) & 0x7fff);
		else write_reg(0,1,read_reg(0,1) & 0x7fff);
		tmp=read_reg(0,1);
	}
	tmp&=0x3fff;
	write_reg(8+0,2,tmp * 14 + properties);
}

void write_string(void) {
	static type32	offset_bak;
	static type8	mask_bak;
	type8  c,b,mask;
	type16 ptr;
	type32 offset;

	if (!cflag) {			/* new string */
		ptr=read_reg(0,1);
		if (!ptr) offset=0;
		else {
			offset=read_w(&decode_table[0x100+2*ptr]);
			if (read_w(&decode_table[0x100])) {
				if (ptr>=read_w(&decode_table[0x100])) offset+=string_size;
			}
		}
		mask=1;
	} else {
		offset=offset_bak;
		mask=mask_bak;
	}
	do {
		c=0;
		while (c<0x80) {
			if (offset>=string_size) b=string2[offset-string_size];
			else b=string[offset];
			if (b & mask) c=decode_table[0x80+c];
			else c=decode_table[c];
			mask<<=1;
			if (!mask) {
				mask=1;
				offset++;
			}
		}
		c&=0x7f;
		if (c && ((c!=0x40) || (lastchar!=0x20))) char_out(c);
	} while (c && ((c!=0x40) || (lastchar!=0x20)));
	cflag=c ? 0xff : 0;
	if (c) {
		offset_bak=offset;
		mask_bak=mask;
	}
}


void do_line_a(void) {
	type8  l1c,*str;
	type16 ptr,ptr2,tmp16;
	type32 tmp32;

	if ((byte2<0xdd) || (version<4 && byte2<0xe4) || (version<2 && byte2<0xed)) {
		ms_flush();											/* flush output-buffer */
		rand_emu();											/* Increase game randomness */
		l1c=ms_getchar();									/* 0 means UNDO */
		if (l1c) write_reg(1,2,l1c);					/* d1=getkey() */
		else {
			if (l1c=ms_undo()) str=undo_ok;
			else str=undo_fail;
			for (tmp16=0;str[tmp16];tmp16++) ms_putchar(str[tmp16]);
			if (!l1c) write_reg(1,2,'\n');
		}
	} else switch (byte2-0xdd) {

case 0:		/* A0DD - Won't probably be needed at all */
				break;
case 1:		/* A0DE */
				write_reg(1,0,1);				/* Should remove the manual check */
				break;
case 2:		/* A0DF */
				/* printf("A0DF stubbed\n"); */
				break;
case 3:		/* A0E0 */
				/* printf("A0E0 stubbed\n"); */
				break;
case 4:		/* A0E1 Read from keyboard to (A1), status in D1 (0 for ok) */
				ms_flush();
				rand_emu();
				tmp32=read_reg(8+1,2);
				str=effective(tmp32);
				tmp16=0;
				do {
					if (!(l1c=ms_getchar())) {
						if (l1c=ms_undo()) str=undo_ok;
						else str=undo_fail;
						for (tmp16=0;str[tmp16];tmp16++) ms_putchar(str[tmp16]);
						if (!l1c) l1c='\n';
					}
					str[tmp16++]=l1c;
				} while (l1c!='\n' && tmp16<256);
				write_reg(8+1,2,tmp32+tmp16-1);
				if (tmp16!=256 && tmp16!=1) write_reg(1,1,0);
				else write_reg(1,1,1);
				break;
case 5:		/* A0E2 */
				/* printf("A0E2 stubbed\n"); */
				break;
case 6:		/* A0E3 */
				/* printf("\nMoves: %u\n",read_reg(0,1)); */
				break;

case 7:		/* A0E4 sp+=4, RTS */
				write_reg(8+7,1,read_reg(8+7,1)+4);
				pc=pop();
				break;

case 8:		/* A0E5 set z, RTS */
case 9:		/* A0E6 clear z, RTS */
				pc=pop();
				zflag=(byte2==0xe5)?0xff:0;
				break;

case 10:		/* A0E7 set z */
				zflag=0xff;
				break;

case 11:		/* A0E8 clear z */
				zflag=0;
				break;

case 12:		/* A0E9 [3083 - j] */
				ptr=read_reg(8+0,1);
				ptr2=read_reg(8+1,1);
				do {
					l1c=effective(ptr++)[0];
					dict[ptr2]=l1c;
				} while (l1c!=0);
				write_reg(8+0,1,ptr);
				write_reg(8+1,1,ptr2);
				break;

case 13:		/* A0EA A1=write_dictword(A1,D1=output_mode) */
				ptr=read_reg(8+1,1);
				tmp32=read_reg(3,0);
				write_reg(3,0,read_reg(1,0));
				do {
					l1c=dict[ptr++];
					char_out(l1c);
				} while (l1c<0x80);
				write_reg(8+1,1,ptr);
				write_reg(3,0,tmp32);
				break;

case 14:		/* A0EB [3037 - j] */
				dict[read_reg(8+1,1)]=read_reg(1,0);
				break;

case 15:		/* A0EC */
				write_reg(1,0,dict[read_reg(8+1,1)]);
				break;

case 16:		running=0;				/* infinite loop A0ED */
				break;
case 17:		if (!ms_init(0,0)) running=0; /* restart game ie. pc, sp etc. A0EE */
				break;
case 18:		internal("undefined op 11 in do_line_a");		/* undefined A0EF */
				break;
case 19:		ms_showpic(read_reg(0,0),read_reg(1,0));	/* Do_picture(D0) A0F0 */
				break;
case 20:		ptr=read_reg(8+1,1);			/* A1=nth_string(A1,D0) A0F1 */
				tmp32=read_reg(0,1);
				while (tmp32-->0) {
					while (effective(ptr++)[0]);
				}
				write_reg(8+1,1,ptr);
				break;

case 21:		/* [2a43] A0F2 */
				cflag=0;
				write_reg(0,1,read_reg(2,1));
				do_findprop();
				ptr=read_reg(8+0,1);
				while (read_reg(2,1)>0) {
					if (read_w(effective(ptr+12)) & 0x3fff) {
						cflag=0xff;
						break;
					}
					if (read_reg(2,1)==(read_reg(4,1) & 0x7fff)) {
						cflag=0xff;
						break;
					}
					ptr-=0x0e;
					write_reg(2,1,read_reg(2,1)-1);
				}
				break;

case 22:		char_out(read_reg(1,0));						/* A0F3 */
				break;

case 23:		/* D7=Save_(filename A0) D1 bytes starting from A1     A0F4 */
				str=(version<4)?effective(read_reg(8+0,1)):0;
				write_reg(7,0,ms_save_file(str,effective(read_reg(8+1,1)),
								read_reg(1,1)));
				break;

case 24:		/* D7=Load_(filename A0) D1 bytes starting from A1     A0F5 */
				str=(version<4)?effective(read_reg(8+0,1)):0;
				write_reg(7,0,ms_load_file(str,effective(read_reg(8+1,1)),
								read_reg(1,1)));
				break;

case 25:		/* D1=Random(0..D1-1) [3748] A0F6 */
				l1c=read_reg(1,0);
				write_reg(1,1,rand_emu()%(l1c?l1c:1));
				break;

case 26:		/* D0=Random(0..255) [3742] A0F7 */
				tmp16=rand_emu();
				write_reg(0,0,tmp16+(tmp16>>8));
				break;

case 27:		/* write string [D0] [2999] A0F8 */
				write_string();
				break;

case 28:		/* Z,D0=Get_inventory_item(D0) [2a9e] A0F9 */
				zflag=0;
				ptr=read_reg(0,1);
				do {
					write_reg(0,1,ptr);
					do {
						do_findprop();
						ptr2=read_reg(8+0,1);						/* object properties */
						if ((effective(ptr2)[5]) & 1) break;	/* is_described or so */
						l1c=effective(ptr2)[6];						/* some_flags */
						tmp16=read_w(effective(ptr2+8));			/* parent_object */
						if (!l1c) {						/* ordinary object? */
							if (!tmp16) zflag=0xff;	/* return if parent()=player */
							break;						/* otherwise try next */
						}
						if (l1c & 0xcc) break;	/* skip worn, bodypart, room, hidden*/
						if (tmp16==0) {			/* return if parent()=player? */
							zflag=0xff;
							break;
						}
						write_reg(0,1,tmp16);	/* else look at parent() */
					} while (1);
					ptr--;
				} while ((!zflag) && ptr);
				write_reg(0,1,ptr+1);
				break;

case 29:		/* [2b18] A0FA */
				ptr=read_reg(8,1);
				do {
					if (read_reg(5,0)) {
						l1c=((read_w(effective(ptr))&0x3fff)==read_reg(2,1));
					} else {
						l1c=(effective(ptr)[0]==read_reg(2,0));
					}
					if (read_reg(3,1)==read_reg(4,1)) {
						cflag=0;
						write_reg(8,1,ptr);
					} else {
						write_reg(3,1,read_reg(3,1)+1);
						ptr+=0x0e;
						if (l1c) {
							cflag=0xff;
							write_reg(8,1,ptr);
						}
					}
				} while ((!l1c) && (read_reg(3,1)!=read_reg(4,1)));
				break;

case 30:		/* [2bd1] A0FB */
				ptr=read_reg(8+1,1);
				do {
					if (dict) while (dict[ptr++]<0x80);
					else while (effective(ptr++)[0]<0x80);
					write_reg(2,1,read_reg(2,1)-1);
				} while (read_reg(2,1));
				write_reg(8+1,1,ptr);
				break;

case 31:		/* [2c3b] A0FC */
				ptr=read_reg(8+0,1);
				ptr2=read_reg(8+1,1);
				do {
					if (dict) while (dict[ptr++]<0x80);
					else while (effective(ptr++)[0]<0x80);
					while (effective(ptr2++)[0]);
					write_reg(0,1,read_reg(0,1)-1);
				} while (read_reg(0,1));
				write_reg(8+0,1,ptr);
				write_reg(8+1,1,ptr2);
				break;

case 32:		/* Set properties pointer from A0 [2b7b] A0FD */
				properties=read_reg(8+0,1);
				if (version>0) fl_sub=read_reg(8+3,1);
				if (version>1) {
					fl_tab=read_reg(8+5,1);
					fl_size=read_reg(7,1)+1;
					/* A3 [routine], A5 [table] and D7 [table-size] */
				}
				if (version>2) {
					fp_tab=read_reg(8+6,1);
					fp_size=read_reg(6,1);
				}
				break;

case 33:		do_findprop();		/* A0FE */
				break;

case 34:		/* Dictionary_lookup A0FF */
				dict_lookup();
				break;
				}
}

/* emulate an instruction [1b7e] */

type8	ms_rungame(void) {
	type8  l1c;
	type16 ptr;
	type32 tmp32;

	if (!running) return running;
	if (pc==undo_pc) save_undo();

	i_count++;
	read_word();
	switch(byte1 >> 1) {

/* 00-0F */
case 0x00:	if (byte1==0x00) {
					if (byte2==0x3c || byte2==0x7c) {
						/* OR immediate to CCR (30D9) */
						read_word();
						if (byte2 & 0x01) cflag=0xff;
						if (byte2 & 0x02) vflag=0xff;
						if (byte2 & 0x04) zflag=0xff;
						if (byte2 & 0x08) nflag=0xff;
					} else {  /* OR [27df] */
						get_arg();
						do_or();
					}
				}	else check_btst();
				break;

case 0x01:	if (byte1==0x02) {
					if (byte2==0x3c || byte2==0x7c) {
						/* AND immediate to CCR */
						read_word();
						if (!(byte2 & 0x01)) cflag=0;
						if (!(byte2 & 0x02)) vflag=0;
						if (!(byte2 & 0x04)) zflag=0;
						if (!(byte2 & 0x08)) nflag=0;
					} else {  /* AND */
						get_arg();
						do_and();
					}
				}	else check_btst();
				break;

case 0x02:	if (byte1==0x04) {	/* SUB */
					get_arg();
					do_sub();
				}	else check_btst();
				break;

case 0x03:	if (byte1==0x06) {	/* ADD */
					get_arg();
					do_add();
				}	else check_btst();
				break;

case 0x04:	if (byte1==0x08) {	/* bit operations (immediate) */
					set_info(byte2 & 0x3f);
					l1c=(effective(pc))[1];
					pc+=2;
					set_arg1();
					do_bop(byte2,l1c);
				} else check_btst();
				break;

case 0x05:	if (byte1==0x0a) {
					if (byte2==0x3c || byte2==0x7c) {
						/* EOR immediate to CCR */
						read_word();
						if (byte2 & 0x01) cflag^=0xff;
						if (byte2 & 0x02) vflag^=0xff;
						if (byte2 & 0x04) zflag^=0xff;
						if (byte2 & 0x08) nflag^=0xff;
					} else {  /* EOR */
						get_arg();
						do_eor();
					}
				}	else check_btst();
				break;

case 0x06:	if (byte1==0x0c) {			/* CMP */
					get_arg();
					do_cmp();
				} else check_btst();
				break;

case 0x07:	check_btst();
				break;

/* 10-1F [3327] MOVE.B */
case 0x08: case 0x09: case 0x0a: case 0x0b:
case 0x0c: case 0x0d: case 0x0e: case 0x0f:

				set_info(byte2 & 0x3f);
				set_arg1();
				swap_args();
				l1c=(byte1>>1 & 0x07) | (byte2>>3 & 0x18) | (byte1<<5 & 0x20);
				set_info(l1c);
				set_arg1();
				do_move();
				break;

/* 20-2F [32d1] MOVE.L */
case 0x10: case 0x11: case 0x12: case 0x13:
case 0x14: case 0x15: case 0x16: case 0x17:

				set_info(byte2 & 0x3f | 0x80);
				set_arg1();
				swap_args();
				l1c=(byte1>>1 & 0x07) | (byte2>>3 & 0x18) | (byte1<<5 & 0x20);
				set_info(l1c | 0x80);
				set_arg1();
				do_move();
				break;

/* 30-3F [3327] MOVE.W */
case 0x18: case 0x19: case 0x1a: case 0x1b:
case 0x1c: case 0x1d: case 0x1e: case 0x1f:

				set_info(byte2 & 0x3f | 0x40);
				set_arg1();
				swap_args();
				l1c=(byte1>>1 & 0x07) | (byte2>>3 & 0x18) | (byte1<<5 & 0x20);
				set_info(l1c | 0x40);
				set_arg1();
				do_move();
				break;

/* 40-4F various commands */

case 0x20:	if (byte1==0x40) {						/* [31d5] */
					internal("unimplemented instructions NEGX and MOVE SR,xx");
				} else check_lea();
				break;

case 0x21:	if (byte1==0x42) {						/* [3188] */
					if ((byte2 & 0xc0)==0xc0) {
						internal("unimplemented instruction MOVE CCR,xx");
					} else {									/* CLR */
						set_info(byte2);
						set_arg1();
						if (opsize==0) arg1[0]=0;
						if (opsize==1) write_w(arg1,0);
						if (opsize==2) write_l(arg1,0);
						nflag=cflag=0;
						zflag=0xff;
					}
				} else check_lea();
				break;

case 0x22:  if (byte1==0x44) {				/* [31a0] */
					if ((byte2 & 0xc0)==0xc0) {			/* MOVE to CCR */
						zflag=nflag=cflag=vflag=0;
						set_info(byte2 & 0x7f);
						set_arg1();
						byte2=arg1[1];
						if (byte2 & 0x01) cflag=0xff;
						if (byte2 & 0x02) vflag=0xff;
						if (byte2 & 0x04) zflag=0xff;
						if (byte2 & 0x08) nflag=0xff;
					} else {
						set_info(byte2);					/* NEG */
						set_arg1();
						cflag=0xff;
						if (opsize==0) {
							arg1[0]=-arg1[0];
							cflag=arg1[0] ? 0xff : 0;
						}
						if (opsize==1) {
							write_w(arg1,-read_w(arg1));
							cflag=read_w(arg1) ? 0xff : 0;
						}
						if (opsize==2) {
							write_l(arg1,-read_l(arg1));
							cflag=read_l(arg1) ? 0xff : 0;
						}
						vflag=0;
						set_flags();
					}
				} else check_lea();
				break;

case 0x23:	if (byte1==0x46) {
					if ((byte2 & 0xc0)==0xc0) {
						internal("unimplemented instruction MOVE xx,SR");
					} else {
						set_info(byte2);					/* NOT */
						set_arg1();
						tmparg[0]=tmparg[1]=tmparg[2]=tmparg[3]=0xff;
						arg2=tmparg;
						do_eor();
					}
				} else check_lea();
				break;

case 0x24:	if (byte1==0x48) {
					if ((byte2 & 0xf8)==0x40) {
						opsize=2;							/* SWAP */
						admode=0;
						regnr=byte2 & 0x07;
						set_arg1();
						tmp32=read_w(arg1);
						write_w(arg1,read_w(arg1+2));
						write_w(arg1+2,tmp32);
						set_flags();
					} else if ((byte2 & 0xf8)==0x80) {
						opsize=1;							/* EXT.W */
						admode=0;
						regnr=byte2 & 0x07;
						set_arg1();
						if (arg1[1]>0x7f) arg1[0]=0xff;
						else arg1[0]=0;
						set_flags();
					} else if ((byte2 & 0xf8)==0xc0) {
						opsize=2;							/* EXT.L */
						admode=0;
						regnr=byte2 & 0x07;
						set_arg1();
						if (read_w(arg1+2)>0x7fff) write_w(arg1,0xffff);
						else write_w(arg1,0);
						set_flags();
					} else if ((byte2 & 0xc0)==0x40) {
						set_info(byte2 & 0x3f | 0x80);		/* PEA */
						set_arg1();
						if (is_reversible) push(arg1i);
						else internal("illegal addressing mode for PEA");
					} else {
						check_movem();							/* MOVEM */
					}
				} else check_lea();
				break;

case 0x25:	if (byte1==0x4a) {							/* [3219] TST */
					if ((byte2 & 0xc0) == 0xc0) {
						internal("unimplemented instruction TAS");
					} else {
						set_info(byte2);
						set_arg1();
						cflag=vflag=0;
						set_flags();
					}
				} else check_lea();
				break;

case 0x26:	if (byte1==0x4c) check_movem2();	/* [3350] MOVEM.L (Ax)+,A/Dx */
				else check_lea();								/* LEA */
				break;

case 0x27:	if (byte1==0x4e) {						/* [3290] */
					if (byte2==0x75)	{							/* RTS */
						pc=pop();
					} else if (byte2==0x71) {					/* NOP */
					} else if ((byte2 & 0xc0) == 0xc0) {		/* indir JMP */
						set_info(byte2 | 0xc0);
						set_arg1();
						if (is_reversible) pc=arg1i;
						else internal("illegal addressing mode for JMP");
					} else if ((byte2 & 0xc0) == 0x80) {
						set_info(byte2 | 0xc0);					/* indir JSR */
						set_arg1();
						push(pc);
						if (is_reversible) pc=arg1i;
						else internal("illegal addressing mode for JSR");
					} else {
						internal("unimplemented instructions 0x4EXX");
					}
				} else check_lea();							/* LEA */
				break;

/* 50-5F [2ed5] ADDQ/SUBQ/Scc/DBcc */
case 0x28: case 0x29: case 0x2a: case 0x2b:
case 0x2c: case 0x2d: case 0x2e: case 0x2f:

				if ((byte2 & 0xc0)==0xc0) {
					set_info(byte2 & 0x3f);
					set_arg1();
					if (admode==1) {								/* DBcc */
						if (condition(byte1)==0) {
							arg1=(arg1-(type8 *)areg)+(type8 *)dreg-1;	/* nasty */
							write_w(arg1,read_w(arg1)-1);
							if (read_w(arg1)!=0xffff) branch(0);
							else pc+=2;
						} else pc+=2;
					} else {											/* Scc */
						arg1[0]=condition(byte1) ? 0xff : 0;
					}
				} else {
					set_info(byte2);
					set_arg1();
					quick_flag=(admode==1)?0xff:0;
					l1c=byte1 >> 1 & 0x07;
					tmparg[0]=tmparg[1]=tmparg[2]=0;
					tmparg[3]=l1c ? l1c : 8;
					arg2=reg_align(tmparg,opsize);
					if ((byte1 & 0x01) == 1) do_sub();			/* SUBQ */
					else do_add();									/* ADDQ */
				}
				break;

/* 60-6F [26ba] Bcc */

case 0x30:	if (byte1==0x61) {				/* BRA, BSR */
					if (byte2==0) push(pc+2);
					else push(pc);
				}
				if ((byte1==0x60) && (byte2==0xfe)) {
					ms_flush();										/* flush stdout */
					running=0;							/* infinite loop - just exit */
				}
				branch(byte2);
				break;

case 0x31: case 0x32: case 0x33:
case 0x34: case 0x35: case 0x36: case 0x37:

				if (condition(byte1)==0) {
					if (byte2==0) pc+=2;
				} else branch(byte2);
				break;


/* 70-7F [260a] MOVEQ */
case 0x38: case 0x39: case 0x3a: case 0x3b:
case 0x3c: case 0x3d: case 0x3e: case 0x3f:

				arg1=(type8 *)&dreg[byte1>>1 & 0x07];
				if (byte2>127) nflag=arg1[0]=arg1[1]=arg1[2]=0xff;
				else nflag=arg1[0]=arg1[1]=arg1[2]=0;
				arg1[3]=byte2;
				zflag=byte2 ? 0 : 0xff;
				break;

/* 80-8F [2f36] */
case 0x40: case 0x41: case 0x42: case 0x43:
case 0x44: case 0x45: case 0x46: case 0x47:

				if ((byte2 & 0xc0)==0xc0) {
					internal("unimplemented instructions DIVS and DIVU");
				} else if (((byte2 & 0xf0)==0) && ((byte1 & 0x01)!=0)) {
					internal("unimplemented instruction SBCD");
				} else {
					set_info(byte2);
					set_arg1();
					set_arg2(1,byte1);
					if ((byte1 & 0x01)==0) swap_args();
					do_or();
				}
				break;

/* 90-9F [3005] SUB */
case 0x48: case 0x49: case 0x4a: case 0x4b:
case 0x4c: case 0x4d: case 0x4e: case 0x4f:

				quick_flag=0;
				if ((byte2 & 0xc0) == 0xc0) {
					if ((byte1 & 0x01) ==1) set_info(byte2 & 0xbf);
					else set_info(byte2 & 0x7f);
					set_arg1();											
					set_arg2(0,byte1);
					swap_args();
				} else {
					set_info(byte2);
					set_arg1();
					set_arg2(1,byte1);
					if ((byte1 & 0x01)==0) swap_args();
				}
				do_sub();
				break;

/* A0-AF various special commands [LINE_A] */

case 0x50:
case 0x56:												/* [2521] */
case 0x57:	do_line_a();
				break;

case 0x51:	pc=pop();									/* RTS */
				break;

case 0x52:	if (byte2==0) push(pc+2);			/* BSR */
				else push(pc);
				branch(byte2);
				break;

case 0x53:	if ((byte2 & 0xc0)==0xc0) {			/* TST [321d] */
					internal("unimplemented instructions LINE_A #$6C0-#$6FF");
				} else {
					set_info(byte2);
					set_arg1();
					cflag=vflag=0;
					set_flags();
				}
				break;

case 0x54:	check_movem();
				break;

case 0x55:	check_movem2();
				break;

/* B0-BF [2fe4] */
case 0x58: case 0x59: case 0x5a: case 0x5b:
case 0x5c: case 0x5d: case 0x5e: case 0x5f:

				if ((byte2 & 0xc0)==0xc0) {
					if ((byte1 & 0x01) ==1) set_info(byte2 & 0xbf);
					else set_info(byte2 & 0x7f);
					set_arg1();											
					set_arg2(0,byte1);
					swap_args();
					do_cmp();							/* CMP */
				} else {
					if ((byte1 & 0x01)==0) {
						set_info(byte2);
						set_arg1();
						set_arg2(1,byte1);
						swap_args();
						do_cmp();						/* CMP */
					} else {
						set_info(byte2);
						set_arg1();
						set_arg2(1,byte1);
						do_eor();						/* EOR */
					}
				}
				break;

/* C0-CF [2f52] */
case 0x60: case 0x61: case 0x62: case 0x63:
case 0x64: case 0x65: case 0x66: case 0x67:

				if ((byte1 & 0x01) == 0) {
					if ((byte2 & 0xc0) == 0xc0) {
						internal("unimplemented instruction MULU");
					} else {									/* AND */
						set_info(byte2);
						set_arg1();
						set_arg2(1,byte1);
						if ((byte1 & 0x01)==0) swap_args();
						do_and();
					}
				} else {
					if ((byte2 & 0xf8)==0x40) {
						opsize=2;							/* EXG Dx,Dx */
						set_arg2(1,byte2<<1);
						swap_args();
						set_arg2(1,byte1);
						tmp32=read_l(arg1);
						write_l(arg1,read_l(arg2));
						write_l(arg2,tmp32);
					} else if ((byte2 & 0xf8)==0x48) {
						opsize=2;							/* EXG Ax,Ax */
						set_arg2(0,byte2<<1);
						swap_args();
						set_arg2(0,byte1);
						tmp32=read_l(arg1);
						write_l(arg1,read_l(arg2));
						write_l(arg2,tmp32);
					} else if ((byte2 & 0xf8)==0x88) {
						opsize=2;							/* EXG Dx,Ax */
						set_arg2(0,byte2<<1);
						swap_args();
						set_arg2(1,byte1);
						tmp32=read_l(arg1);
						write_l(arg1,read_l(arg2));
						write_l(arg2,tmp32);
					} else {
						if ((byte2 & 0xc0) == 0xc0) {
							internal("unimplemented instruction MULS");
						} else {
							set_info(byte2);
							set_arg1();
							set_arg2(1,byte1);
							if ((byte1 & 0x01)==0) swap_args();
							do_and();
						}
					}
				}
				break;

/* D0-DF [2fc8] ADD */
case 0x68: case 0x69: case 0x6a: case 0x6b:
case 0x6c: case 0x6d: case 0x6e: case 0x6f:

				quick_flag=0;
				if ((byte2 & 0xc0) == 0xc0) {
					if ((byte1 & 0x01)==1) set_info(byte2 & 0xbf);
					else set_info(byte2 & 0x7f);
					set_arg1();
					set_arg2(0,byte1);
					swap_args();
				} else {
					set_info(byte2);
					set_arg1();
					set_arg2(1,byte1);
					if ((byte1 & 0x01)==0) swap_args();
				}
				do_add();
				break;

/* E0-EF [3479] LSR ASL ROR ROL */
case 0x70: case 0x71: case 0x72: case 0x73:
case 0x74: case 0x75: case 0x76: case 0x77:

				if ((byte2 & 0xc0) == 0xc0) {
					set_info(byte2 & 0xbf);			/* OP Dx */
					set_arg1();
					l1c=1;								/* steps=1 */
					byte2=(byte1>>1) & 0x03;
				} else {
					set_info(byte2 & 0xc7);
					set_arg1();
					if ((byte2 & 0x20) == 0) {		/* immediate */
						l1c=(byte1 >> 1) & 0x07;
						if (l1c==0) l1c=8;
					} else {
						l1c=read_reg(byte1>>1 & 0x07,0);
					}
					byte2=(byte2>>3) & 0x03;
				}			
				if ((byte1 & 0x01)==0) {			/* right */
					while (l1c-->0) {					
						if (opsize==0) {
							cflag=arg1[0] & 0x01 ? 0xff : 0;
							arg1[0]>>=1;
							if (cflag && (byte2==3)) arg1[0]|=0x80;
						}
						if (opsize==1) {
							cflag=read_w(arg1) & 0x01 ? 0xff : 0;
							write_w(arg1,read_w(arg1)>>1);
							if (cflag && (byte2==3)) write_w(arg1,read_w(arg1)|((type16)1<<15));
						}
						if (opsize==2) {
							cflag=read_l(arg1) & 0x01 ? 0xff : 0;
							write_l(arg1,read_l(arg1)>>1);
							if (cflag && (byte2==3)) write_l(arg1,read_l(arg1)|((type32)1<<31));
						}
					}
				} else {								/* left */
					while (l1c-->0) {
						if (opsize==0) {
							cflag=arg1[0] & 0x80 ? 0xff : 0;			/* [3527] */
							arg1[0]<<=1;
							if (cflag && (byte2==3)) arg1[0]|=0x01;
						}
						if (opsize==1) {
							cflag=read_w(arg1) & ((type16)1<<15) ? 0xff : 0;
							write_w(arg1,read_w(arg1)<<1);
							if (cflag && (byte2==3)) write_w(arg1,read_w(arg1) | 0x01);
						}
						if (opsize==2) {
							cflag=read_l(arg1) & ((type32)1<<31) ? 0xff : 0;
							write_l(arg1,read_l(arg1)<<1);
							if (cflag && (byte2==3)) write_l(arg1,read_l(arg1) | 0x01);
						}
					}
				}
				set_flags();
				break;

/* F0-FF [24f3] LINE_F */
case 0x78: case 0x79: case 0x7a: case 0x7b:
case 0x7c: case 0x7d: case 0x7e: case 0x7f:

				if (version==0) {				/* hardcoded jump */
					char_out(l1c=read_reg(1,0));
				} else if (version==1) {	/* single programmable shortcut */
					push(pc);
					pc=fl_sub;
				} else {							/* programmable shortcuts from table */
					ptr=(byte1 & 7)<<8 | byte2;
					if (ptr>=fl_size) {		
						if ((byte1 & 8)==0) push(pc);
						ptr=byte1<<8 | byte2 | 0x0800;
						ptr=fl_tab+2*(ptr^0xffff);
						pc=ptr+(type16s)read_w(effective(ptr));
					} else {
						push(pc);
						pc=fl_sub;
					}
				}
				break;

default:		internal("Constants aren't and variables don't");
				break;
	}
	return running;
}
