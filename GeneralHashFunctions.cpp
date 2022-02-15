/*
 **************************************************************************
 *                                                                        *
 *          General Purpose Hash Function Algorithms Library              *
 *                                                                        *
 * Author: Arash Partow - 2002                                            *
 * URL: http://www.partow.net                                             *
 * URL: http://www.partow.net/programming/hashfunctions/index.html        *
 *                                                                        *
 * Copyright notice:                                                      *
 * Free use of the General Purpose Hash Function Algorithms Library is    *
 * permitted under the guidelines and in accordance with the most current *
 * version of the Common Public License.                                  *
 * http://www.opensource.org/licenses/cpl.php                             *
 *                                                                        *
 **************************************************************************
*/

#include "GeneralHashFunctions.h"

unsigned int RSHash(const std::string& str)
{

   unsigned int b    = 378551;
   unsigned int a    = 63689;
   unsigned int hash = 0;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash = hash * a + str[i];
      a    = a * b;
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of RS Hash Function */


unsigned int JSHash(const std::string& str)
{

   unsigned int hash = 1315423911;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash ^= ((hash << 5) + str[i] + (hash >> 2));
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of JS Hash Function */


unsigned int PJWHash(const std::string& str)
{

   unsigned int BitsInUnsignedInt = (unsigned int)(sizeof(unsigned int) * 8);
   unsigned int ThreeQuarters     = (unsigned int)((BitsInUnsignedInt  * 3) / 4);
   unsigned int OneEighth         = (unsigned int)(BitsInUnsignedInt / 8);
   unsigned int HighBits          = (unsigned int)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
   unsigned int hash              = 0;
   unsigned int test              = 0;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash = (hash << OneEighth) + str[i];

      if((test = hash & HighBits)  != 0)
      {
         hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
      }
   }

 return (hash & 0x7FFFFFFF);

}
/* End Of  P. J. Weinberger Hash Function */


unsigned int ELFHash(const std::string& str)
{

   unsigned int hash = 0;
   unsigned int x    = 0;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash = (hash << 4) + str[i];
      if((x = hash & 0xF0000000L) != 0)
      {
         hash ^= (x >> 24);
         hash &= ~x;
      }
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of ELF Hash Function */


unsigned int BKDRHash(const std::string& str)
{

   unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
   unsigned int hash = 0;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash = (hash * seed) + str[i];
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of BKDR Hash Function */


unsigned int SDBMHash(const std::string& str)
{

   unsigned int hash = 0;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash = str[i] + (hash << 6) + (hash << 16) - hash;
   }

   return (hash & 0x7FFFFFFF);

}
/* End Of SDBM Hash Function */


unsigned int DJBHash(const std::string& str)
{

   unsigned int hash = 5381;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash = ((hash << 5) + hash) + str[i];
   }

   return (hash & 0x7FFFFFFF);


}
/* End Of DJB Hash Function */


unsigned int APHash(const std::string& str)
{

   unsigned int hash = 0;

   for(unsigned int i = 0; i < str.length(); i++)
   {
      hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ str[i] ^ (hash >> 3)) :
                               (~((hash << 11) ^ str[i] ^ (hash >> 5)));
   }

   return (hash & 0x7FFFFFFF);


}
/* End Of AP Hash Function */
