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

#define SILENT
#include "Logger.h"
#include "Timer.h"
#include <time.h>
#include <string.h>

Logger* Logger::itsDefaultLogger=NULL;
const char* LogMessage::ClassName="LogMessage";

LogMessage::LogMessage(char* theLog)
		  :Message(LogMessage::ClassName), 
		   itsLog(theLog), itsFile(NULL), itsLine(0), itsLevel(INFO)
{
	TRACE("LogMessage constructor 1")
}

LogMessage::LogMessage(const char* theLog)
		  :Message(LogMessage::ClassName), 
		   itsLog(theLog), itsFile(""), itsLine(0), itsLevel(INFO)
{
	TRACE("LogMessage constructor 2")
}

LogMessage::LogMessage(char* theLog,const char* theFile,int theLine,enum LogLevel theLevel,const char* theInstance)
		  :Message(LogMessage::ClassName), 
		   itsLog(theLog),itsFile(theFile), itsLine(theLine), itsLevel(theLevel)
{
	TRACE("LogMessage constructor 3")
	if(theInstance!=NULL)
		itsInstance=theInstance+string("@");
}
	
LogMessage::LogMessage(const char* theLog,const char* theFile,int theLine,enum LogLevel theLevel,const char* theInstance)
		  :Message(LogMessage::ClassName), 
		   itsLog(theLog),itsFile(theFile), itsLine(theLine), itsLevel(theLevel)
{
	TRACE("LogMessage constructor 4")
	if(theInstance!=NULL)
		itsInstance=theInstance+string("@");
}

LogMessage::~LogMessage() 
{
	TRACE("LogMessage destructor")
}

void LogMessage::toStream(ostream& theStream)
{
	char aTimeString[40];
	time_t aTime=time(NULL);
	strftime(aTimeString,sizeof(aTimeString),"%Y-%m-%d %H:%M:%S", localtime(&aTime));		
	theStream << aTimeString;
	
	switch(getLevel())
	{
		case LogMessage::WARNING:
			theStream << " [WARN] ";
			break;
		
		case LogMessage::CRITICAL:
			theStream << " [CRIT] ";
			break;
		
		case LogMessage::DEBUG:
			theStream << " [DEBG] ";
			break;
					
		default:
			theStream << " [INFO] ";			
	}
	
	theStream << itsFile << "(" << itsInstance << itsLine << "): " << itsLog << endl;
}

Logger::Logger() 
      :MessageQueue("DefaultLogger")
{
	TRACE("Default logger constructor")
	itsStream.open("messages.log");
}

Logger::Logger(const char* theLoggerName, const char* theFileName)
	  :MessageQueue(theLoggerName)
{
	TRACE("Logger constructor")
	itsStream.open(theFileName);
}
	
Logger::~Logger() 
{	
	TRACE("Logger destructor")
	wait();
	free();
	itsStream.close();
	release();
}

void Logger::onMessage(Message* theMessage)
{
	TRACE("Logger::onMessage - start")
	TRACE("Class type=" << theMessage->getClass())

	if(theMessage->is("LogMessage"))
	{
		theMessage->toStream(itsStream);
	}
	
	TRACE("Logger::onMessage - end")    
}

void Logger::startDefaultLogger(const char* theFileName)
{
	TRACE("Logger::startDefaultLogger - start")
	TRACE("File name=" << theFileName)
	if(itsDefaultLogger==NULL)
		itsDefaultLogger=new Logger("DefaultLogger",theFileName);	
	TRACE("Logger::startDefaultLogger - end")
}

void Logger::postToDefaultLogger(LogMessage* theMessage)
{
	TRACE("Logger::postToDefaultLogger - start")
	
	if(isShuttingDown()) //++ v1.5
	{
		delete theMessage;
		return;
	}
	
	if(itsDefaultLogger==NULL)
		itsDefaultLogger=new Logger();
	
	if(theMessage)
		itsDefaultLogger->post(theMessage);

	TRACE("Logger::postToDefaultLogger - end")
}	

void Logger::waitForCompletion()
{ 
	TRACE("Logger::waitForCompletion - start")
	if(itsDefaultLogger!=NULL)
	{
		if(!isShuttingDown())		
			delete itsDefaultLogger;
		itsDefaultLogger=NULL;
	}
	TRACE("Logger::waitForCompletion - end")
}

void Logger::bufferDump(char* theBuffer,int theLen,const char* theFile,int theLine)
{
	TRACE("Logger::bufferDump - start")

	if(theLen>16)
	{
		ostrstream aStream;
		aStream << "Dump of " << theLen << " bytes" << ends;
		char* aString=aStream.str();
		postToDefaultLogger(new LogMessage(aString,theFile,theLine,LogMessage::DEBUG));
		delete [] aString;
	}

	int cnt=0;
	while(cnt < theLen)
	{
		ostrstream aStream;

		for(int cnt1=0; (cnt1 <  16) && (cnt < theLen); cnt++,cnt1++)
		{
			char high=0x30 + ((theBuffer[cnt] >> 4) & 0x0f);
			if (high > 0x39) high+=7;
			char low =0x30 + (theBuffer[cnt] & 0x0f);	
			if (low > 0x39) low+=7;

			aStream << high << low << " ";
		}

		aStream << ends;
		char* aString=aStream.str();
		postToDefaultLogger(new LogMessage(aString,theFile,theLine,LogMessage::DEBUG));
		delete [] aString;
	}

	TRACE("Logger::bufferDump - end")
}
