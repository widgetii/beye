#ifndef __BSWAP_H__
#define __BSWAP_H__

#include "bconfig.h"

#include "libbeye/sysdep/__config.h"

// be2me ... BigEndian to MachineEndian
// le2me ... LittleEndian to MachineEndian

#if __BYTE_ORDER != __LITTLE_ENDIAN
#define be2me_16(x) (x)
#define be2me_32(x) (x)
#define be2me_64(x) (x)
#define me2be_16(x) (x)
#define me2be_32(x) (x)
#define me2be_64(x) (x)
#define le2me_16(x) ByteSwapS(x)
#define le2me_32(x) ByteSwapL(x)
#define le2me_64(x) ByteSwapLL(x)
#define me2le_16(x) ByteSwapS(x)
#define me2le_32(x) ByteSwapL(x)
#define me2le_64(x) ByteSwapLL(x)
#else
#define be2me_16(x) ByteSwapS(x)
#define be2me_32(x) ByteSwapL(x)
#define be2me_64(x) ByteSwapLL(x)
#define me2be_16(x) ByteSwapS(x)
#define me2be_32(x) ByteSwapL(x)
#define me2be_64(x) ByteSwapLL(x)
#define le2me_16(x) (x)
#define le2me_32(x) (x)
#define le2me_64(x) (x)
#define me2le_16(x) (x)
#define me2le_32(x) (x)
#define me2le_64(x) (x)
#endif

#endif
