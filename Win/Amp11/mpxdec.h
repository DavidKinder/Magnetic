#ifndef __MPXDEC_H
#define __MPXDEC_H

#include "binfile.h"
#include "mpdecode.h"

class ampegdecoder : public binfile
{
public:
  enum
  {
    ioctlsetstereo=ampegdecoderbase::ioctlsetstereo,
    ioctlsetvol=ampegdecoderbase::ioctlsetvol,
    ioctlsetequal32=ampegdecoderbase::ioctlsetequal32,
    ioctlsetequal576=ampegdecoderbase::ioctlsetequal576
  };

  ampegdecoder();
  virtual ~ampegdecoder();

  int open(binfile &f, int &rate, int &stereo, int fmt, int down, int chn);
};

#endif
