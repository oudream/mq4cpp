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

#include "StoreForward.h"
#include "Logger.h"

class MyClient : public MessageStorer
{
private:
	unsigned itsCnt;

public:
	MyClient(const char* theName, const char* theWorkingPath, const char* theHost, short thePort,const char* theRemoteService)
		:MessageStorer(theName,theWorkingPath,theHost,thePort,theRemoteService)
	{
		itsCnt=0;
		SCHEDULE(this,500);
	};
	
	virtual ~MyClient() {};

protected:
	virtual void onWakeup(Wakeup* theMessage)
	{
		ostrstream aStream;
		aStream << "MyClient message " << ++itsCnt << ends;
		const char* aString=aStream.str();
		TRACE(aString)
		LOG(aString)
		send(aString);
		delete aString;
	};
};

class MyServer : public Server
{
public:
	MyServer(const char* theName) : Server(theName) 
	{
	};	
	
	virtual ~MyServer() 
	{
	};

protected:
	string service(string theBuffer)
	{
		ostrstream aStream;
		aStream << "MyServer(" << getName() << ") receive='" << theBuffer.c_str() << "'" << ends; 
		char* aString=aStream.str();
		TRACE(aString)
		LOG(aString)
		return "OK";
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example10.cpp")
	DISPLAY("This example shows how to implement store&forward")	

	bool client=false;
	char* host=NULL;
	int hport=0;
	
	if(argv < 3)
	{
		DISPLAY("Client usage: example10 -c hostip port")
		DISPLAY("Server usage: example10 -s port")
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
	else if(string(argc[1]).compare("-s")==0 && argv==3)
	{
		client=false;
		DISPLAY("Server port=" << argc[2])
		hport=atoi(argc[2]);
	}	
	else
	{
		DISPLAY("Client usage: example10 -c hostip port")
		DISPLAY("Server usage: example10 -s port")
		return 0;	
	}

	try
	{
		if(client==true)
		{
	    	DISPLAY("Starting client threads...")
			STARTLOGGER("client.log")
			LOG("!!!!!!! example10.cpp - client !!!!!!!")
			MessageForwarder* aForwarder=new MessageForwarder("MyForwarder","tlog");
			MyClient* aStorer=new MyClient("MyClient" , "tlog", host, hport, "MyServer");
			main_sleep(100);			
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
			STARTLOGGER("server.log")
			LOG("!!!!!!! example10.cpp - server !!!!!!!")
			MessageProxyFactory aFactory("MyFactory",hport);
			MyServer* aServer=new MyServer("MyServer");
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

