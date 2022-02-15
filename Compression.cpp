///////////////////////////////////////////////////////////////////////////////
// MQ4CPP - Message queuing for C++
// Copyright (C) 2004-2007  Riccardo Pompeo (Italy)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// WARNING: PacketCompression works using a cache mechanism to avoid to send 
// dictionary information and reducing the bandwidth needed. This mechanism works 
// only in a peer-to-peer transmition. If you plan to use multicast packets or 
// to have a single server with multiple client, the cache mechanism doesn't work
// correctly because PacketCompression isn't able to determine the packet source.  
// In this case you should disable caching calling PacketCompression(false)
// WARNING: compression is a cpu-consuming process. Use only if you have a low 
// bandwidth connectivity with your peer.
//

#define SILENT
#include "Trace.h"
#include "Logger.h"
#include "Compression.h"
#include "MergeSort.h"
#include "Timer.h"
#include <cmath>
#define SYMBOLS 256

PacketCompression::PacketCompression(bool theUseCacheMechanism)
{
	TRACE("PacketCompression::PacketCompression - start")
	itsUseCacheMechanism=theUseCacheMechanism;
	stream_length=0;
	shift_count=0;
	accumulator=0;
	bit_count=0;
	percent=0;
	itsSendPeerReset=true;
	reset();
	TRACE("PacketCompression::PacketCompression - end")	
}

PacketCompression::~PacketCompression()
{
	TRACE("PacketCompression::~PacketCompression - start")	

	TRACE("PacketCompression::~PacketCompression - end")	
}

void PacketCompression::reset()
{
	TRACE("PacketCompression::reset - start")	

	itsDeflateCacheIndex=0;

	for(unsigned cnt=0; cnt < 8; cnt++)
	{
		itsDeflateCacheSchema[cnt]=0;
		itsInflateCacheSchema[cnt]=0;
		itsDeflateCacheCheckBit[cnt]=0;
		itsInflateCacheCheckBit[cnt]=0;
	}	

	for(unsigned cnt=0; cnt < 8; cnt++)
		for(unsigned cnt1=0; cnt1 < 128; cnt1++)
		{
			itsDeflateCacheDictionary[cnt][cnt1]=0;
			itsInflateCacheDictionary[cnt][cnt1]=0;
		}	

	TRACE("PacketCompression::reset - end")	
}

void PacketCompression::evaluateDictionary(vector< pair<int,unsigned char> >& occurrence,unsigned* evaluator)
{
	TRACE("PacketCompression::evaluateDictionary - start")	

	evaluator[0]=(1+0)*8;
	evaluator[1]=(1+2)*8;
	evaluator[2]=(1+4)*8;
	evaluator[3]=(1+8)*8;
	evaluator[4]=(1+16)*8;
	evaluator[5]=(1+32)*8;
	evaluator[6]=(1+64)*8;
	evaluator[7]=(1+128)*8;

	for(unsigned cnt=0;cnt < SYMBOLS; cnt++)
	{
		evaluator[0]+=occurrence[cnt].first * 8;	

		if(cnt < 2)
			evaluator[1]+=occurrence[cnt].first * 2;
		else			
			evaluator[1]+=occurrence[cnt].first * 9;

		if(cnt < 4)
			evaluator[2]+=occurrence[cnt].first * 3;
		else			
			evaluator[2]+=occurrence[cnt].first * 9;

		if(cnt < 8)
			evaluator[3]+=occurrence[cnt].first * 4;
		else			
			evaluator[3]+=occurrence[cnt].first * 9;

		if(cnt < 16)
			evaluator[4]+=occurrence[cnt].first * 5;
		else			
			evaluator[4]+=occurrence[cnt].first * 9;
		
		if(cnt < 32)
			evaluator[5]+=occurrence[cnt].first * 6;
		else			
			evaluator[5]+=occurrence[cnt].first * 9;
		
		if(cnt < 64)
			evaluator[6]+=occurrence[cnt].first * 7;
		else			
			evaluator[6]+=occurrence[cnt].first * 9;

		if(cnt < 128)
			evaluator[7]+=occurrence[cnt].first * 8;
		else			
			evaluator[7]+=occurrence[cnt].first * 9;
	}

	TRACE("PacketCompression::evaluateDictionary - end")	
}

void PacketCompression::putBits(string& stream,unsigned int nb_bits,unsigned int data) 
{
	TRACE("PacketCompression::putBits - start")	

	if (nb_bits==0)
		return;

	/* merge the input data and the accumulator */
	accumulator |= (data << shift_count);
	shift_count += nb_bits;
	bit_count += nb_bits;

	/* sends the bit accumulator, byte per byte */
	while (shift_count >= 8) 
	{
		stream += (unsigned char)(accumulator & 0xff);
		TRACE("Buffer size=" << stream.size())
		accumulator >>= 8;
		shift_count -= 8;
	}

	/* remove some eventual garbage */
	accumulator &= (1U << shift_count) - 1;

	TRACE("PacketCompression::putBits - end")	
}

void PacketCompression::flush(string& stream)
{
	TRACE("PacketCompression::flush - start")	
	if(shift_count > 0)
		stream += (char)(accumulator & 0xff);
	TRACE("PacketCompression::flush - end")	
}

unsigned char PacketCompression::computeCheckBit(unsigned char theSchema,unsigned char* theArray)
{
	TRACE("PacketCompression::computeCheckBit - start")	
	unsigned char ret=0;
	unsigned dictlen=(unsigned)pow(2.0,(int)theSchema);

	for(unsigned cnt=0; cnt < dictlen; cnt++)
	{
		unsigned char sym=theArray[cnt];
		unsigned char bit1=(sym & 0x02) >> 1;
		unsigned char bit3=(sym & 0x08) >> 3;
		unsigned char bit5=(sym & 0x20) >> 5;
		unsigned char bit7=(sym & 0x80) >> 7;
		ret^=((bit1 ^ bit3) ^ bit5) ^ bit7;	// b1 xor b2 xor b3 xor b4	
	}	

	ret <<= 7; // move from bit 0 to 7
	ret &= 0x80; // mask 7 bit only

	TRACE("PacketCompression::computeCheckBit - end")
	return ret;	
}

string PacketCompression::deflate(string& theBuffer)
{
	TRACE("PacketCompression::deflate - start")	
	DUMP("Original buffer",(char*)theBuffer.data(), theBuffer.size());
	//TIME_POINT
	
	string ret;

	vector< pair<int,unsigned char> > occurrence;
	occurrence.resize(SYMBOLS);

	TRACE("Init buffer")	
	for(unsigned cnt=0;cnt < SYMBOLS; cnt++)
	{
		occurrence[cnt].first=0;	
		occurrence[cnt].second=cnt;	
	}

	TRACE("Compute symbols occurrences")	
	for(unsigned cnt=0;cnt < theBuffer.size(); cnt++)
		occurrence[(unsigned char)theBuffer[cnt]].first++;	

	TRACE("Quick sort symbols occurrences")	
	MergeSort< vector< pair<int,unsigned char> > >(occurrence.begin(), occurrence.end() -1);

	TRACE("Evaluate dictionary dimension in bits")	
	unsigned evaluator[8];
	evaluateDictionary(occurrence,evaluator);

	TRACE("Find best dictionary dimension")	
	unsigned char schema=0;
	unsigned min=evaluator[0];
	bool useCache=false;

	for(unsigned char indx=1; indx < 8; indx++)
	{
		if(evaluator[indx] < min)
		{
			schema=indx;	
			min=evaluator[indx];
		}
	}

	TRACE("Schema=" << (int)schema)
	percent=(1.0F-(float)evaluator[schema]/(float)evaluator[0])*100.0F;
	TRACE("Compression=" << percent << "%")
	TRACE("Original buffer size=" << theBuffer.size())
	TRACE("Buffer size without compression=" << evaluator[0])
	TRACE("Buffer size with compression=" << min)

	if(schema==0)
	{
		TRACE("No compression")
		ret='0';
		ret+=theBuffer;
	}
	else
	{
		TRACE("Start compression")	

		struct _SymbolTranslation
		{
			bool compressed;
			char symbol;
		} translation[SYMBOLS];

		unsigned dictlen=(unsigned)pow(2.0,(int)schema);
		TRACE("Dictionary length=" << dictlen)
		unsigned buffer_size=evaluator[schema]/8 + ((evaluator[schema]%8==0)? 0:1);
		TRACE("Buffer size=" << buffer_size)
		ret.reserve(buffer_size);		
		
		bool cacheTest=false;
		unsigned char cacheIndex=0;

		if(itsUseCacheMechanism)
		{
			TRACE("Test current cache")
			cacheTest=true;

			for(unsigned cnt=0; cnt < 8; cnt++)
			{			
				if(schema==itsDeflateCacheSchema[cnt])
				{
					cacheTest=true;
	
					for(unsigned cnt1=0; cnt1 < dictlen; cnt1++)
					{
						if(occurrence[cnt1].second!=itsDeflateCacheDictionary[cnt][cnt1])
						{
							cacheTest=false;
							break;
						}
					}
					
					if(cacheTest==true)
					{
						TRACE("Found a valid dictionary at index=" << cnt)
						cacheIndex=cnt;
						break;	
					}
				}
				else
					cacheTest=false;			
			}
		}

		TRACE("Reset translation array")
		for(unsigned cnt=0; cnt < SYMBOLS; cnt++)
		{
			translation[cnt].compressed=false;
			translation[cnt].symbol=0;
		}		

		TRACE("Fill translation array with dictionary infos")
		for(unsigned cnt=0; cnt < dictlen; cnt++)
		{
			unsigned char sym=occurrence[cnt].second;
			translation[sym].compressed=true;
			translation[sym].symbol=cnt;
		}		

		TRACE("Set compression schema")
		stream_length=0;
		shift_count=0;
		accumulator=0;
		bit_count=(1+dictlen)*8;
		
		// !  7  !  6  !  5  !  4  !  3  !  2  !  1  !  0  !
		// +-----+-----+-----+-----+-----+-----+-----+-----+
		// |Check|   Cache index   |Cache|      Schema     |
		// +-----+-----+-----+-----+-----+-----+-----+-----+	
		
		if(cacheTest==false)
		{
			ret=(unsigned char)(schema | ((itsSendPeerReset) ? 0x80 : 0x00) | ((itsDeflateCacheIndex & 0x07) << 4));
			itsSendPeerReset=false;
			TRACE("Add dictionary")
			for(unsigned cnt=0; cnt < dictlen; cnt++)
				ret+=occurrence[cnt].second;	
		}
		else
		{
			ret=(unsigned char)(schema | 0x08 | ((cacheIndex & 0x07) << 4) | itsDeflateCacheCheckBit[cacheIndex]);
			TRACE("Request peer to use cached dictionary")
		}

		TRACE("Create compressed buffer")
		
		for(unsigned cnt=0;cnt < theBuffer.size(); cnt++)
		{
			unsigned char sym=theBuffer[cnt];
			if(translation[sym].compressed==false)
			{
				putBits(ret,1,0); 
				putBits(ret,8,sym); 
			}
			else
			{
				putBits(ret,1,1); 
				putBits(ret,schema,translation[sym].symbol); 
			}
		}
		
		flush(ret);
		TRACE("Total bits in the buffer=" << bit_count)

		if(cacheTest==false)
		{
			TRACE("Save cache")
			itsDeflateCacheSchema[itsDeflateCacheIndex]=schema;

			for(unsigned cnt=0; cnt < dictlen; cnt++)
				itsDeflateCacheDictionary[itsDeflateCacheIndex][cnt]=occurrence[cnt].second; // Save symbol
			
			itsDeflateCacheCheckBit[itsDeflateCacheIndex]=computeCheckBit(schema,&itsDeflateCacheDictionary[itsDeflateCacheIndex][0]);
			itsDeflateCacheIndex= ++itsDeflateCacheIndex % 8;
		}
		else
		{
			percent=(1.0F-(float)ret.size()/(float)theBuffer.size())*100.0F;
		}
	}

	//DISPLAY_ELAPSED
	TRACE("Final buffer length=" << ret.size())
	DUMP("Compressed buffer",(char*)ret.data(), ret.size());
	TRACE("PacketCompression::deflate - end")	
	return ret;
}

unsigned int PacketCompression::getBits(string& stream,unsigned int nb_bits,bool& eof) 
{
	TRACE("PacketCompression::getBits - start")	

	unsigned int result;
	eof=false;

	if (nb_bits==0) 
	    return 0;

	while (shift_count < nb_bits) 
	{
		if(stream_length>=stream.size())
		{
			eof=true;
			TRACE("PacketCompression::getBits - Reached EOF")
			return 0;			
		}
		
		TRACE("Fetched value=" << (int)stream[stream_length] << " '" << (char)stream[stream_length] << "'")
		accumulator |= (unsigned char)stream[stream_length++] << shift_count;
		shift_count += 8;
	}

	result = accumulator & ((1U << nb_bits) - 1);
	accumulator >>= nb_bits;
	shift_count -= nb_bits;
	bit_count += nb_bits;

	TRACE("PacketCompression::getBits - end")	
	return result;
}

string PacketCompression::inflate(string& theBuffer)
{
	TRACE("PacketCompression::inflate - start")
	//TIME_POINT
		
	string ret;
	ret.reserve(theBuffer.size()*2);
	stream_length=0;
	shift_count=0;
	accumulator=0;

	if(theBuffer[0]=='0')
	{
		DUMP("No compressed buffer",(char*)theBuffer.data(), theBuffer.size());
		ret=theBuffer.substr(1,theBuffer.size()-1);		
	}
	else
	{
		DUMP("Compressed buffer",(char*)theBuffer.data(), theBuffer.size());

		// !  7  !  6  !  5  !  4  !  3  !  2  !  1  !  0  !
		// +-----+-----+-----+-----+-----+-----+-----+-----+
		// |Check|   Cache index   |Cache|      Schema     |
		// +-----+-----+-----+-----+-----+-----+-----+-----+

		unsigned schema=theBuffer[0] & 0x07;
		TRACE("Schema=" << schema)	
		unsigned cacheIndex=(theBuffer[0] >> 4) & 0x07;
		TRACE("Cache index=" << cacheIndex)	
		unsigned checkBit=theBuffer[0] & 0x80;
		TRACE("Check bit=" << ((checkBit) ? "Y" : "N"))	
		bool useCache = ((theBuffer[0] & 0x08)? true : false);
		
		if(useCache==false && checkBit!=0)
		{
			WARNING("Cache reset requested from peer")
			reset();	
		}
		
		bool testCache = (schema==itsInflateCacheSchema[cacheIndex]) && (checkBit==itsInflateCacheCheckBit[cacheIndex]);	
		
		if(useCache==true && testCache==false)
		{
			itsSendPeerReset=true;
			reset();
			WARNING("Invalid buffer during inflating. Forcing peer cache to reset.");
			TRACE("PacketCompression::inflate - end with error")	
			return "";	
		}

		useCache= useCache && testCache;
		TRACE("Cache=" << ((useCache) ? "Y" : "N"))	
	
		unsigned dictlen, bufsize;
		
		if(useCache==false)
		{
			dictlen=(unsigned)pow(2.0,(int)schema);
			bufsize=theBuffer.size()-dictlen-1;
			stream_length=1+dictlen;
		}
		else
		{
			dictlen=(unsigned)pow(2.0,(int)itsInflateCacheSchema[cacheIndex]);
			bufsize=theBuffer.size()-1;
			stream_length=1;
		}
		
		bit_count=stream_length*8;
		bool eof=false;
	
		while(!eof)
		{
			unsigned char compressed=(unsigned char)getBits(theBuffer,1,eof);
			if(eof)
				break;
	
			TRACE("Compressed flag=" << (int)compressed)
	
			if(compressed==1)
			{
				unsigned char sym=(unsigned char)getBits(theBuffer,schema,eof);
				if(eof)
					break;
	
				unsigned char dec;
				
				if(useCache==false)
					dec=theBuffer[sym+1];
				else
					dec=itsInflateCacheDictionary[cacheIndex][sym];
				
				TRACE("Symbol=" << (int)sym << " Value=" << (int)dec << " '" << dec << "'")
				ret+=dec; 			
			}
			else
			{
				unsigned char sym=(unsigned char)getBits(theBuffer,8,eof);
				if(eof)
					break;
	
				TRACE("Value=" << (int)sym << " '" << sym << "'") 			
				ret+=sym;			
			}
		}
		
		TRACE("Total bits in the buffer=" << bit_count)
		
		if(useCache==false)
		{
			TRACE("Save cache")
			itsInflateCacheSchema[cacheIndex]=schema;

			for(unsigned cnt=0; cnt < dictlen; cnt++)
				itsInflateCacheDictionary[cacheIndex][cnt]=theBuffer[cnt+1];

			itsInflateCacheCheckBit[cacheIndex]=computeCheckBit(schema,&itsInflateCacheDictionary[cacheIndex][0]);
		}		
	}

	//DISPLAY_ELAPSED			
	TRACE("PacketCompression::inflate - end")	
	return ret;
}
