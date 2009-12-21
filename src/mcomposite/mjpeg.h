#ifndef __MCOMPOSITE_MJPEG_H__
#define __MCOMPOSITE_MJPEG_H__

// :KLUDGE: в версиях mjpegtools 1.8.x файл mjpeg_types.h
// явно определял константы вроде uint64_t (для Cygwin), 
// а теперь это плющит 64-битную сборку
#ifdef HAVE_STDINT_H
# define _WAS_HAVE_STDINT_H
#else
# define HAVE_STDINT_H 1
#endif

#include <mpegconsts.h>
#include <yuv4mpeg.h>

#ifndef _WAS_HAVE_STDINT_H
# undef HAVE_STDINT_H
#endif
#undef _WAS_HAVE_STDINT_H

#endif // #ifndef __MCOMPOSITE_MJPEG_H__

