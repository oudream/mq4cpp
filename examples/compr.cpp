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

#include "Logger.h"
#include "Timer.h"
#include "Compression.h"
#define PCKSIZE 65535
#define CACHE true
#define MASK 0x1F
#define ITERATIONS 1000

int main(int argv,char* argc[])
{
	DISPLAY("MQ4CPP compr.cpp")
	DISPLAY("This example shows how packet compression works")

	_TIMEVAL startTime;	
	_TIMEVAL endTime;	
	PacketCompression compression(CACHE);
	unsigned int bytesCompressed=0;
	unsigned int bytesDecompressed=0;
	startTime=Timer::timeExt();
	
	for(int i=0; i < ITERATIONS; i++)
	{
		string original;
		original.reserve(PCKSIZE);

		for(int cnt=0; cnt < PCKSIZE ; cnt++)
			original += (unsigned char)(rand() & MASK);
		
		try
		{
			string compressed=compression.deflate(original);	
			bytesCompressed+=compressed.size();
			string decompressed=compression.inflate(compressed);
			bytesDecompressed+=decompressed.size();

			if(decompressed.compare(original)!=0)
			{
				DISPLAY("Test NOK " << i << " Size=" << decompressed.size())
				DISPLAY("Original='" << original << "'")
				DISPLAY("Decompre='" << decompressed << "'")
				exit(-1);
			}
		}
		catch(...)
		{
			DISPLAY("Unhandled exception")	
			exit(-1);
		}
	}
	
	endTime=Timer::timeExt();
	long deltaTime=Timer::subtractMillisecs(&startTime,&endTime);
	DISPLAY("Elapsed=" << deltaTime << " ms")
	float comprRatio=(1-(float)bytesCompressed/(float)bytesDecompressed)*100.0;
	DISPLAY("Compression=" << comprRatio << "%")
	float packetspersecond=(float)ITERATIONS/((float)deltaTime/1000.0);
	DISPLAY("Compression+decompression per seconds=" << packetspersecond)
	float duration=(float)deltaTime/(float)ITERATIONS;
	DISPLAY("Compression+decompression duration=" << duration << " ms")
	return 0;
}
