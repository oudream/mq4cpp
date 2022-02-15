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

#include "MemoryChannel.h"
#include "Logger.h"
#include <string>
#include <strstream>
#include <string.h>
using namespace std;
static int len=0;

class MyClient : public MemoryChannelClient
{
protected:
	int itsCnt;
	
public:
	MyClient(const char* theName, char* theHost,int thePort, const char* theTarget) 
		: MemoryChannelClient(theName,theHost,thePort,theTarget)
	{
		itsCnt=0;
		generate();
	};
	
	virtual ~MyClient() 
	{
	};

protected:
	virtual void generate()
	{
		unsigned long len=(rand()&0xfffff)/2;
		len*=2;
		len+=2;
		setSize(len);
		
		for(unsigned long cnt=0; cnt < len/2; cnt++)
			set(cnt*2,(short)cnt);

		TRACE("Message=" << itsCnt++ << " Size=" << itsSize << " Blocks=" << itsBlocks)
		commit();
	};

	virtual void onCompletion()
	{
		if(fail())
		{
			TRACE("Test failed")
			::exit(0);	
		}				
					
		generate();

		if(fail())
		{
			TRACE("Test failed")
			::exit(0);	
		}				
	};	
};

class MyServer : public MemoryChannelServer
{
protected:
	
public:
	MyServer(const char* theName) : MemoryChannelServer(theName) 
	{
	};
		
	virtual ~MyServer() 
	{
	};

protected:
	virtual void onCompletion()
	{
		for(int cnt=0; cnt < len/2; cnt++)
			if(get16(cnt*2)!=cnt)
			{
				TRACE("Test failed")
				::exit(0);	
			}				
		TRACE("Test OK with Size=" << itsSize)
	};	
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example12.cpp")
	DISPLAY("This example shows how to implement a memory channel")	
	
	bool client=false;
	char* host=NULL;
	int hport=0;
	
	if(argv < 3)
	{
		DISPLAY("Client usage: example12 -c hostip port")
		DISPLAY("Server usage: example12 -s port")
		return 0;	
	}
	if(string(argc[1]).compare("-c")==0 && argv==4)
	{
		client=true;
		DISPLAY("Default host name=" << argc[2])
		host=argc[2];
		DISPLAY("Default host port=" << argc[3])
		hport=atoi(argc[3]);
	}	
	else if(string(argc[1]).compare("-s")==0 && argv==3)
	{
		client=false;
		DISPLAY("Server port=" << argc[2])
		hport=atoi(argc[2]);
	}	
	else
	{
		DISPLAY("Client usage: example12 -c hostip port")
		DISPLAY("Server usage: example12 -s port")
		return 0;	
	}

	try
	{
		if(client==true)
		{
	    	DISPLAY("Starting client threads...")
			STARTLOGGER("client.log")
			LOG("!!!!!!! example12.cpp - client !!!!!!!")
			MyClient* aClientA=new MyClient("MyClientA",host,hport,"MyServerA");
			main_sleep(1000);			
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
			STARTLOGGER("server.log")
			LOG("!!!!!!! example12.cpp - server !!!!!!!")
			MessageProxyFactory aFactory("MyFactory",hport);
			MyServer* aServerA=new MyServer("MyServerA");
			main_sleep(1000);			
		}

	    DISPLAY("...stopping threads...")
	    Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(Exception& ex) 
	{
		TRACE(ex.getMessage().c_str())
	}
	catch(...)
	{
		TRACE("Unhandled exception")	
	}		

    DISPLAY("...done!")
	if(client)    
    	DISPLAY("See client.log for details")
    else	
    	DISPLAY("See server.log for details")
	return 0;
}
