/* Rijndael Cipher

   Written by Mike Scott 21st April 1999
   Copyright (c) 1999 Mike Scott
   See rijndael documentation

   Permission for free direct or derivative use is granted subject 
   to compliance with any conditions that the originators of the 
   algorithm place on its exploitation.  

   Inspiration from Brian Gladman's implementation is acknowledged.

   Written for clarity, rather than speed.
   Full implementation. 
   Endian indifferent.
*/

#ifdef WIN32
#include <windows.h>
#undef word32
typedef UINT32 word32;
typedef INT32 sword32;
typedef unsigned short word16;
typedef unsigned char byte; 
#else
#include <stdlib.h>
#include <string.h>

#ifdef __FreeBSD__
#include <sys/types.h>
typedef uint32_t word32;
typedef int32_t sword32;
#else
#include <asm/types.h>
typedef __u32 word32;
typedef __s32 sword32;
#endif

typedef unsigned short word16;
typedef unsigned char byte; 
#endif

typedef struct rijndael_instance 
{
	int Nk,Nb,Nr;
	byte fi[24],ri[24];
	word32 fkey[120];
	word32 rkey[120];
} RI;

#ifndef WIN32
#ifdef __cplusplus
extern "C" 
{
#endif
        int rijndael_128_LTX__mcrypt_set_key(RI * rinst, byte * key, int nk);
        void rijndael_128_LTX__mcrypt_encrypt(RI * rinst, byte * buff);
        void rijndael_128_LTX__mcrypt_decrypt(RI * rinst, byte * buff);
        int rijndael_256_LTX__mcrypt_set_key(RI * rinst, byte * key, int nk);
        void rijndael_256_LTX__mcrypt_encrypt(RI * rinst, byte * buff);
        void rijndael_256_LTX__mcrypt_decrypt(RI * rinst, byte * buff);
#ifdef __cplusplus
}
#endif
#endif
