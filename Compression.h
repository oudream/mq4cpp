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

#ifndef __COMPRESSION__
#define __COMPRESSION__

#include <vector>
#include <string>

class Compression
{
public:
	virtual string inflate(string& theBuffer)=0;
	virtual string deflate(string& theBuffer)=0;
};

class PacketCompression : public Compression
{
protected:
	unsigned int stream_length; // number of bytes
	unsigned int shift_count;   // number of bits in the accumulator
	unsigned int accumulator;   // bit accumulator
	unsigned int bit_count;
	float percent;

	bool itsUseCacheMechanism;
	bool itsSendPeerReset;
	unsigned char itsDeflateCacheIndex;
	unsigned char itsDeflateCacheSchema[8];
	unsigned char itsDeflateCacheCheckBit[8];
	unsigned char itsDeflateCacheDictionary[8][128];

	unsigned char itsInflateCacheSchema[8];
	unsigned char itsInflateCacheCheckBit[8];
	unsigned char itsInflateCacheDictionary[8][128];

public:
	PacketCompression(bool theUseCacheMechanism=true);
	virtual ~PacketCompression();
	virtual string inflate(string& theBuffer);
	virtual string deflate(string& theBuffer);
	virtual float getCompressionPercent() { return percent; };
	virtual const char* getName() { return "PacketCompression"; };
	
protected:
	virtual void evaluateDictionary(vector< pair<int,unsigned char> >& occurrence,unsigned* evaluator);
	virtual void putBits(string& stream,unsigned int nb_bits,unsigned int data);
	virtual void flush(string& stream); 
	virtual unsigned int getBits(string& stream,unsigned int nb_bits,bool& eof);
	virtual unsigned char computeCheckBit(unsigned char theSchema,unsigned char* theArray);	
	virtual void reset();
};

#endif
