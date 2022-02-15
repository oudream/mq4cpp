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

//#define SILENT
#include "MessageProxy.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;

class MyClient : protected Observer
{
private:
	MQHANDLE itsProxy;	
	MQHANDLE itsServer;
	unsigned itsCnt;
	char* itsHost; 
	const char* itsTarget;

public:
	MyClient(const char* theName, char* theHost, const char* theTarget) : Observer(theName)
	{
		itsCnt=0;
		itsHost=theHost;
		itsTarget=theTarget;
		TRACE("Run " << getName())
		MessageProxyFactory::lookupAt(itsHost,9000,itsTarget,this);
		setEncription(new Rijndael256()); // ++v1.3			
		LOG("Start to lookup MyServer network service")
	};
	
	virtual ~MyClient() {};

protected:
	virtual void onLookup(LookupReplyMessage* theMessage)
	{
		if(!theMessage->isFailed())
		{
			itsServer=theMessage->getHandle();
			itsProxy=theMessage->getSender();
			sendMessage();
			TRACE("Lookup ok!")
			LOG("Lookup ok!")
		}
		else
		{
			TRACE("Lookup failed. Retry!")	
			LOG("Lookup failed. Retry!")	
			MessageProxyFactory::lookupAt(itsHost,9000,itsTarget,this);			
		}
	};

	virtual NetworkMessage* generateMessage()
	{
		ostrstream aStream;
		aStream << "MyClient(" << getName() << ") message n." << ++itsCnt << ends;
		char* aString=aStream.str();
		int aLen=strlen(aString)+1;
		LOG(aString)
		NetworkMessage* aMessage=new NetworkMessage(aString,aLen);
		aMessage->setSender(getID());
		aMessage->setTarget(itsServer);
		delete [] aString;
		return aMessage;				
	};
	
	void sendMessage()
	{
		if(isStillAvailable(itsProxy))
			post(itsProxy,generateMessage());
		else
			MessageProxyFactory::lookupAt(itsHost,9000,itsTarget,this);									
	};

	virtual NetworkMessage* onRequest(NetworkMessage* theMessage)
	{
		ostrstream aRxStream;
		theMessage->toStream(aRxStream);
		aRxStream << ends;
		char* aRxString=aRxStream.str();
		TRACE("MyClient(" << getName() << ") receive='" << aRxString << "'")
		LOG(aRxString)
		NetworkMessage* aMessage=generateMessage();
		delete [] aRxString;
		return aMessage;
	};
};

class MyServer : protected Observer
{
protected:
	unsigned itsCnt; 

public:
	MyServer(const char* theName) : Observer(theName)
	{
		itsCnt=0;
		setEncription(new Rijndael256()); // ++v1.3			
	};
	
	virtual ~MyServer() {};

protected:
	virtual NetworkMessage* onRequest(NetworkMessage* theMessage)
	{
		ostrstream aRxStream;
		theMessage->toStream(aRxStream);
		aRxStream << ends;
		char* aRxString=aRxStream.str();
		TRACE("MyServer(" << getName() << ") receive='" << aRxString << "'")
		LOG(aRxString)
		ostrstream aStream;
		aStream << "MyServer(" << getName() << ") ACK n." << ++itsCnt << ends;
		char* aString=aStream.str();
		int aLen=strlen(aString)+1;
		NetworkMessage* aMessage=new NetworkMessage(aString,aLen);
		LOG(aString)
		delete [] aRxString;
		delete [] aString;
		return aMessage;
	};
};

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("MQ4CPP example6.cpp")
	DISPLAY("This example shows how to implement a network conversation")	
	
	bool client=false;
	char* host="127.0.0.1";
	
	if(argv==2)
	{
		TRACE("Host name=" << argc[1])
		host=argc[1];	
		client=true;
	}
	else if(argv!=1)
	{
		DISPLAY("Usage: example6 [hostip]")
		return 0;	
	}

	try
	{
		if(client==true)
		{
	    	DISPLAY("Starting client threads...")
			STARTLOGGER("client.log")
			LOG("!!!!!!! example6.cpp - client !!!!!!!")
			MyClient* aClientA=new MyClient("MyClientA",host,"MyServerA");
			MyClient* aClientB=new MyClient("MyClientB",host,"MyServerA");
			MyClient* aClientD=new MyClient("MyClientC",host,"MyServerB");
			MyClient* aClientE=new MyClient("MyClientD",host,"MyServerB");
			main_sleep(200);			
		}
		else
		{			
	    	DISPLAY("Starting server threads...")
			STARTLOGGER("server.log")
			LOG("!!!!!!! example6.cpp - server !!!!!!!")
			MessageProxyFactory aFactory("MyFactory",9000);
			MyServer* aServerA=new MyServer("MyServerA");
			MyServer* aServerB=new MyServer("MyServerB");
			main_sleep(200);			
		}

	    DISPLAY("...stopping threads...")
	    Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(Exception& ex) 
	{
		DISPLAY(ex.getMessage().c_str())
	}
	catch(...)
	{
		DISPLAY("Unhandled exception")	
	}		

    DISPLAY("...done!")
	if(client)    
    	DISPLAY("See client.log for details")
    else	
    	DISPLAY("See server.log for details")
	return 0;
}
