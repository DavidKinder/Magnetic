// binfile - class library for files/streams - standard disk files
// Copyright (c) 1997-2000 Niklas Beisert
// See COPYING (GNU General Public License 2) for license

#ifndef NOUNISTD
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(WIN32)||defined(UNIX)
#include <io.h>
#define ftruncate chsize
#endif
#ifdef SUNOS4
extern "C" extern int ftruncate(int, size_t);
#endif
#include "binfstd.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

sbinfile::sbinfile()
{
}

sbinfile::~sbinfile()
{
  close();
}

errstat sbinfile::open(const char *name, int type)
{
  close();
  intm omode=O_BINARY;
  intm fmode=0;
  switch (type&openiomode)
  {
  case openis: fmode=moderead; omode|=O_RDONLY; break;
  case openos: fmode=modewrite; omode|=O_WRONLY; break;
  case openro: fmode=moderead|modeseek; omode|=O_RDONLY; break;
  case openrw: fmode=moderead|modewrite|modeseek|modeappend; omode|=O_RDWR; break;
  default: return -1;
  }
  switch (type&opencrmode)
  {
  case openex: omode|=0; break;
  case opencr: omode|=O_CREAT; break;
  case opentr: omode|=O_CREAT|O_TRUNC; break;
  case opencn: omode|=O_CREAT|O_EXCL; break;
  default: return -1;
  }
  handle=::open(name, omode, S_IREAD|S_IWRITE);
  if (handle<0)
    return -1;
  binfilepos len;
  if (fmode!=modewrite)
  {
    len=lseek(handle, 0, SEEK_END);
    lseek(handle, 0, SEEK_SET);
  }
  else
  {
    len=0;
    lseek(handle, 0, SEEK_END);
  }
  openmode(fmode, 0, len);
  trunc=0;
  return 0;
}

errstat sbinfile::rawclose()
{
  closemode();
  if (trunc)
    ftruncate(handle, lseek(handle, 0, SEEK_CUR));
  ::close(handle);
  return 0;
}

binfilepos sbinfile::rawread(void *buf, binfilepos len)
{
  return ::read(handle, buf, len);
}

binfilepos sbinfile::rawpeek(void *buf, binfilepos len)
{
  binfilepos l=::read(handle, buf, len);
  ::lseek(handle, -l, SEEK_CUR);
  return l;
}

binfilepos sbinfile::rawwrite(const void *buf, binfilepos len)
{
  return ::write(handle, buf, len);
}

binfilepos sbinfile::rawseek(binfilepos len)
{
  return ::lseek(handle, len, SEEK_SET);
}

binfilepos sbinfile::rawioctl(intm code, void *buf, binfilepos len)
{
  binfilepos ret;
  switch (code)
  {
  case ioctltrunc: ret=trunc; trunc=((mode&modeappend)&&len)?1:0; break;
  case ioctltruncget: ret=trunc; break;
  default: return binfile::rawioctl(code, buf, len);
  }
  return ret;
}

#else

#include <stdio.h>
#include "binfstd.h"

sbinfile::sbinfile()
{
}

sbinfile::~sbinfile()
{
  close();
}

errstat sbinfile::open(const char *name, int type)
{
  close();
  intm omode=0;
  intm fmode=0;
  char str[4];
  char *strp=str;
  switch (type&openiomode)
  {
  case openis: fmode=moderead; break;
  case openos: fmode=modewrite; break;
  case openro: fmode=moderead|modeseek; break;
  case openrw: fmode=moderead|modewrite|modeseek|modeappend; break;
  default: return -1;
  }
  switch (type&(opencrmode|openiomode))
  {
  case openis|openex: *strp++='r'; break;
  case openis|opencr: *strp++='a'; *strp++='+'; break;
  case openis|opentr: *strp++='w'; break;
  case openis|opencn: return -1;
  case openos|openex: return -1;
  case openos|opencr: *strp++='a'; break;
  case openos|opentr: *strp++='w'; break;
  case openos|opencn: return -1;
  case openro|openex: *strp++='r'; break;
  case openro|opencr: *strp++='a'; *strp++='+'; break;
  case openro|opentr: *strp++='w'; break;
  case openro|opencn: return -1;
  case openrw|openex: *strp++='r'; *strp++='+'; break;
  case openrw|opencr: return -1;
  case openrw|opentr: *strp++='w'; *strp++='+'; break;
  case openrw|opencn: return -1;
  default: return -1;
  }
  *strp++='b';
  file=fopen(name, str);
  if (!file)
    return -1;
  binfilepos len;
  if (fmode!=modewrite)
  {
    fseek(file, 0, SEEK_END);
    len=ftell(file);
    fseek(file, 0, SEEK_SET);
  }
  else
    len=0;
  openmode(fmode, 0, len);
  return 0;
}

errstat sbinfile::rawclose()
{
  closemode();
  fclose(file);
  return 0;
}

binfilepos sbinfile::rawread(void *buf, binfilepos len)
{
  return fread(buf, 1, len, file);
}

binfilepos sbinfile::rawpeek(void *buf, binfilepos len)
{
  binfilepos l=fread(buf, 1, len, file);
  fseek(file, -l, SEEK_CUR);
  return l;
}

binfilepos sbinfile::rawwrite(const void *buf, binfilepos len)
{
  return fwrite(buf, 1, len, file);
}

binfilepos sbinfile::rawseek(binfilepos len)
{
  fseek(file, len, SEEK_SET);
  return ftell(file);
}

binfilepos sbinfile::rawioctl(intm code, void *buf, binfilepos len)
{
  switch (code)
  {
  default: return binfile::rawioctl(code, buf, len);
  }
}

#endif
