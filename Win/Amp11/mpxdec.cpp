// amp11 - an Audio-MPEG decoder - layer 1/2/3 decoder
// Copyright (c) 1997 Niklas Beisert
// See COPYING (GNU General Public License 2) for license

#include "mpxdec.h"
//#include "mp1dec.h"
//#include "mp2dec.h"
#include "mp3dec.h"

ampegdecoder::ampegdecoder()
{
}

ampegdecoder::~ampegdecoder()
{
}

int ampegdecoder::open(binfile &f, int &freq, int &stereo, int fmt, int downsample, int destchn)
{
  close();
  int layer,a,b,c,d;
  ampegdecoderbase *fil;
  if (!ampegdecoderbase::getheader(f, layer, a,b,c,d))
    return -1;
  switch (layer)
  {
//  case 0: fil=new ampegdecoderl1; break;
//  case 1: fil=new ampegdecoderl2; break;
  case 2: fil=new ampegdecoderl3; break;
  default: return -1;
  }
  int r=fil->open(f,freq,stereo,fmt,downsample,destchn);
  if (r<0)
  {
    delete fil;
    return r;
  }

  openpipe(*fil,1,0,-1,-1,-1);
  return 0;
}
