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

#ifndef __STOREFORWARD__
#define __STOREFORWARD__

#include "Session.h"
#include "FileSystem.h"

class MessageStorer : protected Observer
{
protected:
	string itsHost;
	short itsPort;
	string itsService;
	Directory* itsCurDir;
	Directory* itsTlogDir;
	unsigned long itsStartTime;
	unsigned long itsMsgCnt;	

public:
	MessageStorer(const char* theName, const char* theWorkingPath, const char* theHost, short thePort,const char* theRemoteService);
	virtual ~MessageStorer();
	virtual void send(string theBuffer);
};


class TargetHost : public Client
{
public:
	typedef enum { STOP, SENDING, SUCCESS, FAIL } TargetHostState;

protected:
	File* itsFile;
	TargetHostState itsState;
	unsigned long itsTime;

public:
	TargetHost(const char* theName, const char* theHost,int thePort, const char* theTarget);
	virtual ~TargetHost();
	virtual bool send(string theBuffer,string theFileName); 
	virtual TargetHostState getState() { return itsState; };
	virtual string getFileName();
	virtual unsigned long getCreationTime() { return itsTime; };
	
protected:
	virtual void success(string theBuffer);
	virtual void fail(string theError);	
};

class MessageForwarder : protected Observer 
{
protected:
	Directory* itsCurDir;
	Directory* itsTlogDir;
	vector<TargetHost*> itsHostList;
	unsigned long itsLastScanTime;

public:
	MessageForwarder(const char* theName,const char* theWorkingPath);
	virtual ~MessageForwarder();
	
protected:
	virtual void onWakeup(Wakeup* theMessage);
	virtual void scan();
	virtual void purge();
};

#endif

