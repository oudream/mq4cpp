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

#include "Trace.h"

#ifdef WIN32	
void trace_dump(const char* descr,char* theString,int theLen)
{
	char buffer[1024];
	ostrstream aStream(&buffer[0],sizeof(buffer));
	int cnt=0;
	aStream << "Start dump of '"  << descr << "'" << endl; 
	while(cnt < theLen)
	{
		for(int cnt1=0; (cnt1 <  8) && (cnt < theLen); cnt++,cnt1++)
		{
			char high=0x30 + ((theString[cnt] >> 4) & 0x0f);
			if (high > 0x39) high+=7;
			char low =0x30 + (theString[cnt] & 0x0f);	
			if (low > 0x39) low+=7;
			aStream << high << low << " ";
			if(theString[cnt] >= 0x20  && theString[cnt] <= 0x7E)
				aStream << "'" << theString[cnt] << "'" << " ";
		    else
				aStream << "' ' ";		    		
		}
		aStream << endl;
	}
	aStream << "End dump of '" << descr << "'" << endl << ends;
	
	if(IsDebuggerPresent())
	{
		Sleep(0);
		OutputDebugString(buffer); 	
		Sleep(0);
	} 
	else 
	{ 
		cout << buffer;
	}	
}
#else
void trace_dump(const char* descr,char* theString,int theLen)
{
	int cnt=0;
	cout << "Start dump of '"  << descr << "'" << endl; 
	while(cnt < theLen)
	{
		for(int cnt1=0; (cnt1 <  8) && (cnt < theLen); cnt++,cnt1++)
		{
			char high=0x30 + ((theString[cnt] >> 4) & 0x0f);
			if (high > 0x39) high+=7;
			char low =0x30 + (theString[cnt] & 0x0f);	
			if (low > 0x39) low+=7;
			cout << high << low << " ";
			if(theString[cnt] >= 0x20  && theString[cnt] <= 0x7E)
				cout << "'" << theString[cnt] << "'" << " ";
		    else
				cout << "' ' ";		    		
		}
		cout << endl;
	}
	cout << "End dump of '" << descr << "'" << endl;
}
#endif



