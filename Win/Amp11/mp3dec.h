#ifndef __MP3DEC_H
#define __MP3DEC_H

#include "mpdecode.h"

class ampegdecoderl3 : public ampegdecoderbase
{
private:
  struct grsistruct
  {
    int gr;
    int ch;

    int blocktype;
    int mixedblock;

    int grstart;
    int tabsel[4];
    int regionend[4];
    int grend;

    int subblockgain[3];
    int preflag;
    int sfshift;
    int globalgain;

    int sfcompress;
    int sfsi[4];

    int ktabsel;
  };

  static int htab00[],htab01[],htab02[],htab03[],htab04[],htab05[],htab06[],htab07[];
  static int htab08[],htab09[],htab10[],htab11[],htab12[],htab13[],htab14[],htab15[];
  static int htab16[],htab24[];
  static int htaba[],htabb[];
  static int *htabs[34];
  static int htablinbits[34];
  static int sfbtab[7][3][5];
  static int slentab[2][16];
  static int sfbands[3][3][14];
  static int sfbandl[3][3][23];
  static float citab[8];
  static float csatab[8][2];
  static float sqrt05;
  static float ktab[3][32][2];
  static float sec12[3];
  static float sec24wins[6];
  static float cos6[3];
  static float sec36[9];
  static float sec72winl[18];
  static float cos18[9];
  static float winsqs[3];
  static float winlql[9];
  static float winsql[12];
  static int pretab[22];
  static float pow2tab[65];
  static float pow43tab[8206];
  static float ggaintab[256];

  inline int hgetbit();
  inline unsigned long hgetbits(int n);
  inline int huffmandecoder(int *valt);
  static void imdct(float *out, float *in, float *prev, int blocktype);
  static void fdctd6(float *out, float *in);
  static void fdctd18(float *out, float *in);
  void readgrsi(grsistruct &si, int &bitpos);
  void jointstereo(grsistruct &si, float (*xr)[576], int *scalefacl);
  void readhuffman(grsistruct &si, float *xr);
  void scale(grsistruct &si, float *xr, int *scalefacl);
  void readscalefac(grsistruct &si, int *scalefacl);
  void hybrid(grsistruct &si, float hout[18][2][32], float prev[2][32][18], float *xr);
  void readsfsi(grsistruct &si);
  int readmain(grsistruct (*si)[2]);

  int rotab[3][576];
  float l3equall[576];
  float l3equals[192];
  int l3equalon;

  float prevblck[2][32][18];
  unsigned char huffbuf[4096];
  int huffoffset;
  int huffbit;

  float hybrid0[18][2][32];
  int ispos[576];
  int scalefac0[2][39];
  float xr0[2][576];

protected:
  virtual void seekinit(int);
  virtual int decode(void *);
  virtual int openlayer(int,int,int,int);
  virtual void closelayer();
  void setl3equal(const float *);

public:
  ampegdecoderl3();
  virtual ~ampegdecoderl3();

  virtual binfilepos ioctl(intm code, void *buf, binfilepos len);
  binfilepos ioctl(intm code) { return ioctl(code,0,0); }
  binfilepos ioctl(intm code, binfilepos par) { return ioctl(code,0,par); }
};

#endif
