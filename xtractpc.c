#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char  type8;
typedef unsigned long  type32;

FILE   *fpin=0, *fpout=0;
type8  decode_table[256],table_size=0,*code=0,*dict=0,*str1=0,*str2=0;
type32 code_size=0,dict_size=0,str1_size=0,str2_size=0;

type32 read_w(type8 *ptr) {
	return (type32)(ptr[0]<<8 | ptr[1]);
}

void write_l(type8 *ptr, type32 val) {
	ptr[3]=val & 0xff; val>>=8;
	ptr[2]=val & 0xff; val>>=8;
	ptr[1]=val & 0xff; val>>=8;
	ptr[0]=val;
}

void cleanup(char *errormsg, type32 status) {
	fprintf(stderr,errormsg);
	if (fpin)	fclose(fpin);
	if (fpout)	fclose(fpout);
	if (code)	free(code);
	if (dict)	free(dict);
	if (str1)	free(str1);
	if (str2)	free(str2);
	exit(status);
}

type8 getbits(type8 init) {
	static type8 mask,c;
	type8 b;

	if (init) {
		mask=0;
		return 0;
	}
	for (b=0;b<0x80;) {
		b<<=1;
		mask>>=1;
		if (!mask) {
			mask=0x80;
			c=fgetc(fpin);
		}
		if (!(c&mask)) b++;
		b=decode_table[b];
	}
	return (type8)(b&0x3f);
}

type32 read_and_unpack(type8 *name, type8 **ptr, type8 unpack) {
	type8  dat[4],*ptr2;
	type32 i,size,loop=0;

	if (!(fpin=fopen(name,"rb"))) cleanup("Couldn't open input file\n",1);
	if (unpack) {
		getbits(1);
		table_size=fgetc(fpin);
		for (i=0;i<table_size;i++) decode_table[i]=fgetc(fpin);
		loop=fgetc(fpin)<<8 | fgetc(fpin);
		if (!(ptr2=(type8 *)malloc(loop*3))) cleanup("Not enough memory\n",1);
		size=0; *ptr=ptr2;
		while (loop--) {
			for (i=0;i<4;i++) dat[i]=getbits(0);
			for (i=0;i<3;i++) ptr2[size++]=dat[i]|((dat[3]<<(2*i+2))&0xc0);
		}
	} else {
		if (fseek(fpin,0,SEEK_END)<0) cleanup("File error\n",1);
		if ((size=ftell(fpin))<0) cleanup("File error\n",1);
		rewind(fpin);
		ptr2=malloc(size);
		*ptr=ptr2;
		if (fread(ptr2,1,size,fpin)!=size) cleanup("File error\n",1);
	}
	fclose(fpin); fpin=0;
	return size;
}

main(int argc, char **argv) {
	type32 i,version;
	type8	name[64],header[42];

	if (sizeof(type8)!=1 || sizeof(type32)!=4) {
		cleanup("Wrong typesizes - edit the typedefs and recompile!\n",1);
	}
	if (argc!=3) cleanup("Usage: basename versioncode\n"
								"\twhere \"basename\" is the part of the filename "
								"preceding the number and\n\tversioncode is 0 for "
								"Pawn, 1 for GoT, 2 for Jinxter and 3 for "
								"the rest.\n\tA file called \"basename.mag\" will be "
								"created\n\n",1);
	version=atoi(argv[2]);
	if (version>1) {
		strcpy(name,argv[1]);
		strcat(name,"0");
		dict_size=read_and_unpack(name,&dict,1);
	}
	strcpy(name,argv[1]);
	strcat(name,"1");
	code_size=read_and_unpack(name,&code,1);
	strcpy(name,argv[1]);
	strcat(name,"2");
	str2_size=read_and_unpack(name,&str2,0);
	strcpy(name,argv[1]);
	strcat(name,"3");
	str1_size=read_and_unpack(name,&str1,0);
	write_l(header,0x4D615363);				/* magic number MaSc */
	write_l(header+4,code_size+dict_size+str1_size+str2_size+42);
	write_l(header+8,42);						/* header size */
	header[13]=version;
	write_l(header+14,code_size);
	write_l(header+18,str1_size<0x10000?str1_size:0x10000);
	write_l(header+22,str1_size<0x10000?str2_size:str1_size-0x10000+str2_size);
	write_l(header+26,dict_size);
	write_l(header+30,str1_size);				/* decoding offset */


	write_l(header+34,read_w(code+6));		/* restart offset (rough estimate) */
	for (i=0;i<=code_size-16;i+=2) {
		if (read_w(code+i)==0xa62c && read_w(code+i+4)==0x6600 &&
			 read_w(code+i+8)==0x536c && read_w(code+i+12)==0x526c) {
			while (read_w(code+i-2)!=0xa200 && read_w(code+i-2)) i-=2;
			write_l(header+38,i);				/* undo-pc */
			break;
		}
	}
	if (i>code_size-16) printf("Undo will for some reason not work...\n");

	strcpy(name,argv[1]);
	strcat(name,".mag");
	if (!(fpout=fopen(name,"wb"))) cleanup("Couldn't open output file\n",1);

	if (fwrite(header,1,42,fpout)!=42) cleanup("File error\n",1);
	if (fwrite(code,1,code_size,fpout)!=code_size) cleanup("File error\n",1);
	if (fwrite(str1,1,str1_size,fpout)!=str1_size) cleanup("File error\n",1);
	if (fwrite(str2,1,str2_size,fpout)!=str2_size) cleanup("File error\n",1);
	if (version>1) {
		if (fwrite(dict,1,dict_size,fpout)!=dict_size) cleanup("File error\n",1);
	}
	cleanup("Operation Successful\n",0);
}
