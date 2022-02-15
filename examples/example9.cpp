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

#include "MessageProxy.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;

class MyClient : public Observer
{
private:
	unsigned itsCnt;
	char* itsHost; 

public:
	MyClient(const char* theName, char* theHost) : Observer(theName)
	{
		itsCnt=0;
		itsHost=theHost;
		setEncription(new Rijndael128("MyVerySecretCode")); 
		SCHEDULE(this,100);
	};
	
	virtual ~MyClient() {};

protected:
	virtual void onWakeup(Wakeup* theMessage)
	{
		MessageProxyFactory::ping(itsHost,9000,this);			

		ostrstream aStream;
		aStream << "MyClient(" << getName() << ") message n." << ++itsCnt << ends;
		char* aString=aStream.str();
		if(itsCnt%2)
			publish("Sport",aString);
		else
			publish("News",aString);

		delete [] aString;
		TRACE("Message published n." << itsCnt)
	};
};

class MyServer : public Observer
{
public:
	MyServer(const char* theTopic) : Observer(theTopic) 
	{
		subscribe(theTopic);
		setEncription(new Rijndael128("MyVerySecretCode")); 
	};
	
	~MyServer() {};

protected:
	virtual void onBroadcast(NetworkMessage* theMessage)
	{
		ostrstream aRxStream;
		theMessage->toStream(aRxStream);
		aRxStream << ends;
		char* aRxString=aRxStream.str();
		TRACE("MyServer(" << getName() << ") receive='" << aRxString << "'")
		LOG(aRxString)
		delete [] aRxString;
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example9.cpp")
	DISPLAY("This example shows how to implement publish/subscribe")	

	bool client=false;
	char* host="127.0.0.1";
	
	if(argv==2)
	{
		DISPLAY("Host name=" << argc[1])
		host=argc[1];	
		client=true;
	}
	else if(argv!=1)
	{
		DISPLAY("Usage: example9 [hostip]")
		return 0;	
	}

	try
	{
		if(client==true)
		{
	    	DISPLAY("Starting client and server threads...")
	    	STARTLOGGER("client.log")
			LOG("!!!!!!! example9.cpp - client !!!!!!!")
			MyClient* aClientA=new MyClient("Publisher",host);
			MyServer* aServerA=new MyServer("Sport");
			MyServer* aServerB=new MyServer("News");
			main_sleep(100);			
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
			STARTLOGGER("server.log")
			LOG("!!!!!!! example9.cpp - server !!!!!!!")
			MessageProxyFactory aFactory("MyFactory",9000);
			MyServer* aServerA=new MyServer("Sport");
			MyServer* aServerB=new MyServer("News");
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

