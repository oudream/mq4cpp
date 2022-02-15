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

#ifndef __FILESTRANSFER__
#define __FILESTRANSFER__

#include "Session.h"
#include "FileSystem.h"
#include "Compression.h"
#include <list>
using namespace std;

#define FT_BLOCKSIZE 65000
#define FT_TIMEOUT 10

typedef enum _FT_State 
{
	FT_STOP,
	FT_STARTING_TX,
	FT_SENDING_HEADER,
	FT_SENDING_BLOCKS,
	FT_WAIT_LAST_ACK,
	FT_FAILED
} FileTransferState;

typedef enum _FT_Result
{
	FT_VOID,
	FT_NOT_ALLOWED,
	FT_STREAM_ERROR,
	FT_SESSION_STARTED,
	FT_SESSION_FINISHED,
	FT_SESSION_CONTINUE
} FileTransferResult;

typedef enum _FT_MSG
{
	FT_MSG_FILE,
	FT_MSG_DIR,
	FT_MSG_ITERATE
} FileTransferSourceType;

class FileTransferMessage : public Message
{
protected:	
	FileTransferSourceType itsSourceType;
	string itsSource;
	string itsDestination;
			
public:
	FileTransferMessage();
	FileTransferMessage(File* theFile, const char* theDestination);
	FileTransferMessage(Directory* theDir, const char* theDestination);
	virtual ~FileTransferMessage() {};
	virtual bool iterate() { return itsSourceType==FT_MSG_ITERATE; };
	virtual string getSource() { return itsSource; };
	virtual bool directory() { return itsSourceType==FT_MSG_DIR; }; 
	virtual string getDestination() { return itsDestination; };
};

class FileTransferClient : public Client
{
protected:
	FileTransferState itsState;
	File* itsFile;
	Directory* itsSrcDirectory;
	string itsDstDirectory;
	vector<File*> itsFileList;
	string itsDestinationPath;
	ListProperty itsTxStructure;	
	ListProperty itsResponse;
	unsigned long itsTxCnt;
	unsigned long itsSessionToken;
	unsigned long itsLastTx;

public:
	FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget);
	FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Encription* theEncr);
	FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Compression* theCompr);
	FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Encription* theEncr,Compression* theCompr);
	virtual ~FileTransferClient();
	virtual bool send(File* theFile,const char* theDestinationPath="");
	virtual bool send(Directory* theDir,const char* theDestinationPath="");
	virtual bool fail(); 
	virtual bool done();
	virtual unsigned short percent(); 	
	
protected:
	virtual void success(string theBuffer);
	virtual void fail(string theError);	
	virtual bool sendHeader();
	virtual void sendBlock();
	virtual void cleanup(FileTransferState theState=FT_STOP);	
	virtual void onWakeup(Wakeup* theMessage);
	virtual void onCompletion(); // To ovveride	
	virtual void onLocal(Message* theMessage);
};

typedef struct _FT_Session
{
	unsigned long rxcnt;
	unsigned long size;
	File* file;
	unsigned long token;
	unsigned long time;
} FileTransferSession;

class FileTransferServer : public Server
{
protected:
	Directory itsDirectory;
	ListProperty itsRxStructure;	
	list<FileTransferSession> itsSessions;
	
public:
	FileTransferServer(const char* theName,Directory* thePath);
	FileTransferServer(const char* theName,Directory* thePath,Encription* theEncr);
	FileTransferServer(const char* theName,Directory* thePath,Compression* theCompr);
	FileTransferServer(const char* theName,Directory* thePath,Encription* theEncr,Compression* theCompr);
	virtual ~FileTransferServer();
	
protected:
	virtual string service(string theBuffer);
	virtual void receiveHeader(ListProperty& theResponse);
	virtual void receiveBlock(ListProperty& theResponse);	
	virtual void setResponse(ListProperty& theResponse,FileTransferResult theCode,unsigned long theToken=0L);
	virtual void onWakeup(Wakeup* theMessage);	
	virtual void onCompletion(File* theFile); // To ovveride	
}; 

#endif


