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

#include "RequestReply.h"
#include "Logger.h"
#include "Compression.h"
#include <string>
#include <strstream>
using namespace std;

class MyClient : public Client
{
protected:
	unsigned long itsCnt;	
	
public:
	MyClient(const char* theName, char* theHost,int thePort, const char* theTarget) 
		: Client(theName,theHost,thePort,theTarget)
	{
		setEncription(new Rijndael256(Encription::generateKey256("MyVerySecretPassword"))); 
		setCompression(new PacketCompression(false));
		itsCnt=0;
		send(generate());		
	};
	
	virtual ~MyClient() {};

protected:
	virtual string generate()
	{
		char buffer[256];
		ostrstream aStream(buffer,sizeof(buffer));
		aStream << "MyClient(" << getName() << ") message n." << ++itsCnt << ends;
		TRACE("Send: " << buffer)		
		return buffer;				
	};
	
	void success(string theBuffer)
	{
		TRACE("Message successfully sent. Received='" << theBuffer.c_str() << "'")
		send(generate());		
		TRACE("Sending another.")
	};
	
	void fail(string theError)
	{
		LOG("MyClient - Service failed")		
	};
};

class MyServer : public Server
{
public:
	MyServer(const char* theName) : Server(theName) 
	{
		setEncription(new Rijndael256(Encription::generateKey256("MyVerySecretPassword"))); 
		setCompression(new PacketCompression());
	};	
	
	virtual ~MyServer() {};

protected:
	string service(string theBuffer)
	{
		ostrstream aStream;
		aStream << "MyServer(" << getName() << ") receive='" << theBuffer.c_str() << "'" << ends; 
		char* aString=aStream.str();
		TRACE(aString)
		LOG(aString)
		delete [] aString;
		return "Message received";
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example7.cpp")
	DISPLAY("This example shows how to send request/reply network messages")	
	
	bool client=false;
	char* host=NULL;
	int hport=0;
	char* fhost=NULL;
	int fport=0;
	
	if(argv < 3)
	{
		DISPLAY("Client usage: example7 -c hostip port [failover_hostip] [port]")
		DISPLAY("Server usage: example7 -s port")
		return 0;	
	}
	else if(string(argc[1]).compare("-c")==0 && argv==4)
	{
		client=true;
		DISPLAY("Default host name=" << argc[2])
		host=argc[2];
		DISPLAY("Default host port=" << argc[3])
		hport=atoi(argc[3]);
	}	
	else if(string(argc[1]).compare("-c")==0 && argv==6)
	{
		client=true;
		DISPLAY("Default host name=" << argc[2])
		host=argc[2];
		DISPLAY("Default host port=" << argc[3])
		hport=atoi(argc[3]);
		DISPLAY("Failover host name=" << argc[4])
		fhost=argc[4];
		DISPLAY("Failover host port=" << argc[5])
		fport=atoi(argc[5]);
	}	
	else if(string(argc[1]).compare("-s")==0 && argv==3)
	{
		client=false;
		DISPLAY("Server port=" << argc[2])
		hport=atoi(argc[2]);
	}	
	else
	{
		DISPLAY("Client usage: example7 -c hostip port [failover_hostip] [port]")
		DISPLAY("Server usage: example7 -s port")
		return 0;	
	}

	try
	{
		if(client==true)
		{
	    	DISPLAY("Starting client threads...")
			STARTLOGGER("client.log")
			LOG("!!!!!!! example7.cpp - client !!!!!!!")
			MyClient* aClientA=new MyClient("MyClientA",host,hport,"MyServerA");
			if(fhost!=NULL)
				aClientA->addFailoverHost(fhost,fport);
				
			main_sleep(20);			
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
			STARTLOGGER("server.log")
			LOG("!!!!!!! example7.cpp - server !!!!!!!")
			MessageProxyFactory aFactory("MyFactory",hport);
			MyServer* aServerA=new MyServer("MyServerA");
			main_sleep(100);			
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
