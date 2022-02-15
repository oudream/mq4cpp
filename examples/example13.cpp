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

#include "LockManager.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;

class MyClient : public LockManagerClient
{
protected:
	enum MyClientEnum
	{
		SEND_LOCK,
		ACQUIRING_LOCK,
		SEND_UNLOCK,
		ACQUIRING_UNLOCK		
	} itsState;
	
public:
	MyClient(const char* theName, char* theHost,int thePort, const char* theTarget,Encription* theEncr) 
		: LockManagerClient(theName,theHost,thePort,theTarget,theEncr)
	{
	};
	
	virtual ~MyClient() 
	{
	};

protected:
	
	virtual void onWakeup(Wakeup* theMessage)
	{
		switch(itsState)
		{
			case SEND_LOCK:
				if(!lock("MyLock")) exit(-1);
				itsState=ACQUIRING_LOCK;
				TRACE("Requesting lock")
				break;

			case SEND_UNLOCK:
				if(!unlock("MyLock")) exit(-1);
				itsState=ACQUIRING_UNLOCK;
				TRACE("Requesting unlock")
				break;			
		}				
		
		LockManagerClient::onWakeup(theMessage);
	};
	
	virtual void onCompletion(LockManagerSession& theSession)
	{
		TRACE("Client=" << theSession.client << " Token=" << theSession.token)
		switch(itsState)
		{
			case ACQUIRING_LOCK:
				switch(theSession.state)
				{
					case LM_STOP:
						itsState=SEND_UNLOCK;
						TRACE("Lock acquired.")
						break;				
						
					case LM_FAILED:
						itsState=SEND_LOCK;
						TRACE("Lock not acquired. Retry.")
						break;				
				}
				break;

			case ACQUIRING_UNLOCK:		
				switch(theSession.state)
				{
					case LM_STOP:
						itsState=SEND_LOCK;
						TRACE("Unlock acquired")
						break;				
						
					case LM_FAILED:
						itsState=SEND_UNLOCK;
						TRACE("Unlock not acquired. Retry.")
						break;				
				}
				break;		
		}				
	};	
	
	virtual void fail(string theError)
	{
		DISPLAY("Got error")
		exit(0);
	};	
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example13.cpp")
	DISPLAY("This example shows how to implement a distributed lock manager")	
	
	bool client=false;
	char* host=NULL;
	int hport=0;
	
	try
	{
		if(argv < 3)
		{
			DISPLAY("Client usage: example13 -c hostip port")
			DISPLAY("Server usage: example13 -s port")
			return 0;	
		}

		if(string(argc[1]).compare("-c")==0 && argv==4)
		{
			client=true;
			DISPLAY("Host name=" << argc[2])
			host=argc[2];
			DISPLAY("Host port=" << argc[3])
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
			DISPLAY("Client usage: example13 -c hostip port filename")
			DISPLAY("Server usage: example13 -s port")
			return 0;	
		}

		Encription* anEncr=new Rijndael128("M355493qu3u1n9f0"); 

		if(client==true)
		{
			STARTLOGGER("client.log")
	    	LOG("Starting client threads...")

			MyClient* aClient=new MyClient("MyClient",host,hport,"MyServer",anEncr);
			main_sleep(100);
		}
		else
		{			
			STARTLOGGER("server.log")
	    	LOG("Starting server threads...")

			MessageProxyFactory aFactory("MyFactory",hport);
			LockManagerServer* aServer=new LockManagerServer("MyServer",anEncr);
			main_sleep(100);
		}

	    LOG("...stopping threads")
	    Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(Exception& ex) 
	{
		LOG(ex.getMessage().c_str())
	    Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(...)
	{
		LOG("Unhandled exception")	
	    Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}		

	return 0;
}
