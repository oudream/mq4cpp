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
    
 RS Hash Function
 A simple hash function from Robert Sedgwicks Algorithms in C book. 
 I've added some simple optimizations to the algorithm in order to speed up its hashing process.
    
 JS Hash Function
 A bitwise hash function written by Justin Sobel
    
 PJW Hash Function
 This hash algorithm is based on work by Peter J. Weinberger of AT&T Bell Labs.
    
 ELF Hash Function
 Similar to the PJW Hash function, but tweaked for 32-bit processors. 
 Its the hash function widely used on most UNIX systems.
    
 BKDR Hash Function
 This hash function comes from Brian Kernighan and Dennis Ritchie's book 
 "The C Programming Language". It is a simple hash function using a strange 
 set of possible seeds which all constitute a pattern of 31....31...31 etc, 
 it seems to be very similar to the DJB hash function.
    
 SDBM Hash Function
 This is the algorithm of choice which is used in the open source SDBM project. 
 The hash function seems to have a good over-all distribution for many different data sets. 
 It seems to work well in situations where there is a high variance in the MSBs of the elements in a data set.
    
 DJB Hash Function
 An algorithm produced by Daniel J. Bernstein and shown first to the world 
 on the comp.lang.c newsgroup. Its efficient as far as processing is concerned.
    
 DEK Hash Function
 An algorithm proposed by Donald E. Knuth in The Art Of Computer Programming 
 Volume 3, under the topic of sorting and search chapter 6.4.
    
 AP Hash Function
 An algorithm produced by me Arash Partow. I took ideas from all of the 
 above hash functions making a hybrid rotative and additive hash function 
 algorithm based around four primes 3,5,7 and 11. There isn't any real mathematical 
 analysis explaining why one should use this hash function instead of the others 
 described above other than the fact that I tired to resemble the design as 
 close as possible to a simple LFSR. An empirical result which demonstrated 
 the distributive abilities of the hash algorithm was obtained using a hash-table 
 with 100003 buckets, hashing The Project Gutenberg Etext of Webster's Unabridged Dictionary, 
 the longest encountered chain length was 7, the average chain length was 2, 
 the number of empty buckets was 4579.
 
*/

#ifndef __GENERALHASHFUNCTION__
#define __GENERALHASHFUNCTION__

#include <string>

typedef unsigned int (*HashFunction)(const std::string&);

unsigned int RSHash  (const std::string& str);
unsigned int JSHash  (const std::string& str);
unsigned int PJWHash (const std::string& str);
unsigned int ELFHash (const std::string& str);
unsigned int BKDRHash(const std::string& str);
unsigned int SDBMHash(const std::string& str);
unsigned int DJBHash (const std::string& str);
unsigned int APHash  (const std::string& str);

#endif
