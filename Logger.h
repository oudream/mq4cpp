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

#ifndef __LOGGER__
#define __LOGGER__

#include <fstream>
using namespace std;

#include "Trace.h"
#include "MessageQueue.h"

class LogMessage : public Message
{
public:
	enum LogLevel { INFO,WARNING,CRITICAL,DEBUG };

protected:
	string itsLog; //v1.4
	string itsFile; //v1.4
	int itsLine;
	enum LogLevel itsLevel;
	string itsInstance;

public:	
	static const char* ClassName;
	LogMessage(char* theLog);
	LogMessage(const char* theLog);
	LogMessage(char* theLog,const char* theFile,int theLine,enum LogLevel theLevel,const char* theInstance=NULL);
	LogMessage(const char* theLog,const char* theFile,int theLine,enum LogLevel theLevel,const char* theInstance=NULL);
	virtual ~LogMessage();
	virtual const char* get() { return itsLog.c_str(); };
	virtual const char* getFile() { return itsFile.c_str(); };
	virtual int getLine() { return itsLine; };
	virtual enum LogLevel getLevel() { return itsLevel; };
	virtual void toStream(ostream& theStream);
};

class Logger : public MessageQueue
{
protected:
	static Logger* itsDefaultLogger;
	ofstream itsStream; 
		
public:
	Logger(const char* theLoggerName, const char* theFileName);
	virtual ~Logger();
	static void startDefaultLogger(const char* theFileName); // ++  v1.5
	static void postToDefaultLogger(LogMessage* theMessage);
	static void waitForCompletion();
	static void bufferDump(char* theBuffer,int theLen,const char* theFile,int theLine);
	
protected:
	Logger();
	virtual void onMessage(Message* theMessage);	
};

#ifndef SILENT
#define DEBUG(a) \
	Logger::post(new LogMessage(a,__FILE__,__LINE__, LogMessage::DEBUG));
#else
#define DEBUG(a) ;
#endif

#define BUFFER(a,b) \
	Logger::bufferDump(a,b,__FILE__,__LINE__);
#define LOG(a) \
	Logger::postToDefaultLogger(new LogMessage(a,__FILE__,__LINE__, LogMessage::INFO));
#define WARNING(a) \
	Logger::postToDefaultLogger(new LogMessage(a,__FILE__,__LINE__, LogMessage::WARNING,getName()));
#define CRITICAL(a) \
	Logger::postToDefaultLogger(new LogMessage(a,__FILE__,__LINE__, LogMessage::CRITICAL,getName()));
#define STARTLOGGER(a) \
	Logger::startDefaultLogger(a);
#define STOPLOGGER() \
	Logger::waitForCompletion();
#endif
