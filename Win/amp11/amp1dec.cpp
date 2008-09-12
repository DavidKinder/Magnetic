// amp11 - an Audio-MPEG decoder - layer 1 decoder
// Copyright (c) 1997-2000 Niklas Beisert
// See COPYING (GNU General Public License 2) for license

#include <math.h>
#include "ampdec.h"

void ampegdecoder::openlayer1(int rate)
{
  if (rate)
  {
    slotsize=4;
    slotdiv=freqtab[orgfreq]>>orglsf;
    nslots=(36*rate)/(freqtab[orgfreq]>>orglsf);
    fslots=(36*rate)%slotdiv;
  }
}

void ampegdecoder::decode1()
{
  int i,j,q,fr;
  for (fr=0; fr<3; fr++)
  {
    if (fr)
      decodehdr(0);
    if (!hdrbitrate)
    {
      for (q=0; q<12; q++)
        for (j=0; j<2; j++)
          for (i=0; i<32; i++)
            fraction[j][12*fr+q][i]=0;
      continue;
    }

    int bitend=mainbufpos-32-(hdrcrc?16:0)+(hdrpadding?32:0)+12*1000*ratetab[hdrlsf?1:0][0][hdrbitrate]/(freqtab[hdrfreq]>>hdrlsf)*32;

    int jsbound=(hdrmode==1)?(hdrmodeext+1)*4:(hdrmode==3)?0:32;
    int stereo=(hdrmode==3)?1:2;

    for (i=0;i<32;i++)
      for (j=0;j<((i<jsbound)?2:1);j++)
      {
        bitalloc1[j][i] = mpgetbits(4);
        if (i>=jsbound)
          bitalloc1[1][i] = bitalloc1[0][i];
      }

    for (i=0;i<32;i++)
      for (j=0;j<stereo;j++)
        if (bitalloc1[j][i])
          scale1[j][i]=multiple[mpgetbits(6)]*rangefac[bitalloc1[j][i]];

    for (q=0;q<12;q++)
      for (i=0;i<32;i++)
        for (j=0;j<((i<jsbound)?2:1);j++)
          if (bitalloc1[j][i])
          {
            int s=mpgetbits(bitalloc1[j][i]+1)-(1<<bitalloc1[j][i])+1;
            fraction[j][12*fr+q][i]=scale1[j][i]*s;
            if (i>=jsbound)
              fraction[1][12*fr+q][i]=scale1[1][i]*s;
          }
          else
          {
            fraction[j][12*fr+q][i]=0;
            if (i>=jsbound)
              fraction[1][12*fr+q][i]=0;
          }

    mpgetbits(bitend-mainbufpos);
  }
}
