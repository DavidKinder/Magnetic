#ifndef __MPDECODE_H
#define __MPDECODE_H

#include "binfile.h"

#ifdef __WATCOMC__
void __fistp(long &i, long double x);
#pragma aux __fistp parm [eax] [8087] = "fistp dword ptr [eax]"
unsigned int __swap32(unsigned int);
#pragma aux __swap32 parm [eax] value [eax] = "bswap eax"
#endif
#ifdef GNUCI486
static inline unsigned int __swap32(unsigned int x)
{
  asm("bswapl %0\n":"=r"(x):"0"(x));
  return x;
}
#endif

class ampegdecoderbits
{
private:
  enum
  {
    bufmin=2048,
    bufmax=16384
  };
  binfile *pt;
  unsigned char buf[bufmax];
  int bufsize;
  int buflow;
  int buflen;
  int bufpos;

  void setbufsize(int size, int min);

protected:
  int openbits(binfile &fil);
  void closebits();
  void seekbits(int p);
  void refillbits();
  long mpgetbits(int n);
  void getbytes(void *buf2, int n);
  int sync7FF();

  unsigned int convbe32(unsigned int x)
  {
  #ifdef BIGENDIAN
    return x;
  #elif defined(__WATCOMC__)||defined(GNUCI486)
    return __swap32(x);
  #else
    return ((x<<24)&0xFF000000)|((x<<8)&0xFF0000)|((x>>8)&0xFF00)|((x>>24)&0xFF);
  #endif
  }

  ampegdecoderbits();
  virtual ~ampegdecoderbits();
};

class ampegdecodersynth
{
private:
  static float dwin[1024];
  static float dwin2[512];
  static float dwin4[256];
  static float sectab[32];

  int synbufoffset;
  float synbuf[2048];
  void *synthdest;

  inline float muladd16a(float *a, float *b);
  inline float muladd16b(float *a, float *b);

  int convle16(int x)
  {
  #ifdef BIGENDIAN
    return ((x>>8)&0xFF)|((x<<8)&0xFF00);
  #else
    return x;
  #endif
  }
  int cliptoshort(float x)
  {
  #ifdef __WATCOMC__
    long foo;
    __fistp(foo,x);
  #else
    int foo=(int)x;
  #endif
    return (foo<-32768)?convle16(-32768):(foo>32767)?convle16(32767):convle16(foo);
  }
  static void fdctb32(float *out1, float *out2, float *in);
  static void fdctb16(float *out1, float *out2, float *in);
  static void fdctb8(float *out1, float *out2, float *in);

protected:
  int tomono;
  int tostereo;
  int deststereo;
  int ratereduce;
  int usevoltab;
  int srcstereo;
  int dctstereo;
  int samplesize;
  float stereotab[3][3];
  float equal[32];
  int equalon;
  float volume;

  int opensynth(int,int,int,int);
  void closesynth();
  void synth(float*);
  void synthsetbuf(void *);

  ampegdecodersynth();
  virtual ~ampegdecodersynth();

  void setstereo(const float *);
  void setvol(float);
  void setequal(const float *);
};

class ampegdecoderbase : public ampegdecoderbits, public ampegdecodersynth, public binfile
{
private:
  static int vertab[4];
  static int freqtab[4];

protected:
  static int ratetab[2][3][16];

  int hdrlay;
  int hdrcrc;
  int hdrbitrate;
  int hdrfreq;
  int hdrpadding;
  int hdrmode;
  int hdrmodeext;
  int hdrversion;
  int decodehdr();

  int orglay;
  int orgsamprateidx;
  int orgversion;
  int orgstereo;

  int stream;
  int slotsize;
  int nslots;
  int fslots;
  int slotdiv;
  int seekinitframes;
  char framebuf[2304*4];
  int curframe;
  int framepos;
  int nframes;
  int framesize;
  int samprate;

protected:
  virtual int decode(void *)=0;
  virtual void seekinit(int)=0;
  virtual int openlayer(int,int,int,int)=0;
  virtual void closelayer()=0;

  ampegdecoderbase();

  virtual errstat rawclose();
  virtual binfilepos rawseek(binfilepos pos);
  virtual binfilepos rawread(void *buf, binfilepos len);
  virtual binfilepos rawpeek(void *buf, binfilepos len);
  virtual binfilepos rawioctl(intm code, void *buf, binfilepos len);

public:
  enum
  {
    ioctlsetvol=ioctluser, ioctlsetstereo, ioctlsetequal32, ioctlsetequal576
  };

  int open(binfile &in, int &freq, int &stereo, int fmt, int down, int chn);
  static int getheader(binfile &in, int &layer, int &ver, int &freq, int &stereo, int &rate);

  virtual ~ampegdecoderbase();
};

#endif
