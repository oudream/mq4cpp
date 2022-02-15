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
#include <string>
#include <strstream>
using namespace std;

class MyMessage : public Message
{
protected:
	string itsBuffer;
	
public:
	MyMessage(char* theMessage) : Message("MyMessage") 
	{
		itsBuffer=theMessage;
	};
	
	virtual ~MyMessage() {};
	virtual void toStream(ostream& theStream) { theStream << itsBuffer; };
	virtual string toString() { return itsBuffer; };
};

class MyClient : protected MessageQueue
{
protected:
	unsigned itsCounter;
	MessageQueue* itsTarget;

public:
	MyClient(MessageQueue* theQueue) : MessageQueue("MyClient"), itsTarget(theQueue)
	{
		SCHEDULE(this,500);
		itsCounter=0;
	};
	
	virtual ~MyClient() {};

protected:
	virtual void onMessage(Message* theMessage)
	{
		if(theMessage->is("Wakeup"))
		{
			Wakeup* aWakeup=(Wakeup*)theMessage;
			
			ostrstream aStream;
			aStream << "MyClient wakeup at ";
			aStream << aWakeup->getTime() << ends;
			char* aString=aStream.str();
			TRACE(aString)
			LOG(aString)
			delete [] aString;
			
			ostrstream aStream1;
			aStream1 << "Message n.";
			aStream1 << ++itsCounter << ends;
			char* aString1=aStream1.str();
			itsTarget->post(new MyMessage(aString1));
			delete [] aString1;
		}
	};
};

class MyServer : public MessageQueue
{
public:
	MyServer(): MessageQueue("MyServer") {};
	virtual ~MyServer() {};

protected:
	virtual void onMessage(Message* theMessage)
	{
		if(theMessage->is("MyMessage"))
		{
			MyMessage* aMessage=(MyMessage*)theMessage;
			ostrstream aStream;
			aStream << "MyServer receive <";
			aStream << aMessage->toString() << ">" << ends;
			char* aString=aStream.str();
			TRACE(aString)
			LOG(aString)
			delete [] aString;
		}
	};
};

int main() 
{
	DISPLAY("MQ4CPP example1.cpp")
	DISPLAY("This example shows how to send direct messages")
	LOG("!!!!!!! example1.cpp !!!!!!!")

	try
	{
	    DISPLAY("Starting client and server threads...")
		MyServer* aServer=new MyServer();
		MyClient* aClient=new MyClient(aServer);
		DISPLAY("...wait 20 secs...")	
		Thread::sleep(20*1000);
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
    DISPLAY("See messages.log for details")
	return 0;
}
