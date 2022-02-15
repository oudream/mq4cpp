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

#ifndef __LOCKMANAGER__
#define __LOCKMANAGER__

#include "Session.h"
#include "FileSystem.h"
#include "Exception.h"
#include <list>
using namespace std;

#define LM_TIMEOUT 10
#define LM_SERVERTIMEOUT 100

typedef enum _LM_State 
{
	LM_STOP,
	LM_WAIT_ACK,
	LM_FAILED
} LockManagerState;

typedef enum _LM_Result
{
	LM_VOID,
	LM_NOT_ALLOWED,
	LM_LOCKED,
	LM_UNLOCKED
} LockManagerResult;

typedef struct _LM_Session
{
	unsigned long token;
	unsigned long time;
	string resource;
	LockManagerState state;
	unsigned long client;
} LockManagerSession;

class LockManagerException : public Exception
{
private:
	string msg;
public:
	LockManagerException();
	LockManagerException(string m) { msg=m; };
	LockManagerException(const char* m) { msg=m; };
	virtual ~LockManagerException() {};
	virtual string getMessage() const { return msg; };
};	

class LockManagerClient : protected Client
{
protected:
	ListProperty itsTxStructure;
	ListProperty itsResponse;	
	list<LockManagerSession> itsSessions;

public:
	LockManagerClient(const char* theName, const char* theHost,int thePort, const char* theTarget);
	LockManagerClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Encription* theEncr);
	virtual ~LockManagerClient();

	virtual bool lock(const char* theResourceName);
	virtual bool unlock(const char* theResourceName);
	
protected:
	virtual void success(string theBuffer);
	virtual void onWakeup(Wakeup* theMessage);

	virtual void onCompletion(LockManagerSession& theSession)=0; // To ovveride	
};

class LockManagerServer : protected Server
{
protected:
	list<LockManagerSession> itsSessions;
	ListProperty itsRxStructure;	
	
public:
	LockManagerServer(const char* theName);
	LockManagerServer(const char* theName,Encription* theEncr);
	virtual ~LockManagerServer();

	virtual unsigned long lock(string theResourceName);
	virtual void unlock(unsigned long theToken);
	
protected:
	virtual string service(string theBuffer);
	virtual void receiveLock(unsigned long theClientToken,ListProperty& theResponse);
	virtual void receiveUnlock(unsigned long theClientToken,ListProperty& theResponse);	
	virtual void setResponse(ListProperty& theResponse,LockManagerResult theCode,unsigned long theClientToken,unsigned long theToken=0L);
	virtual void onWakeup(Wakeup* theMessage);	
	virtual void onLock(LockManagerSession& theSession); // To ovveride	
	virtual void onUnlock(LockManagerSession& theSession); // To ovveride	
}; 


#endif


