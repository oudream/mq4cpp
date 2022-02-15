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
#include "FileTransfer.h"
#include "Trace.h"
#include "GeneralHashFunctions.h"
#include "Logger.h"

FileTransferMessage::FileTransferMessage()
                    :Message("FileTransferMessage")
{
	TRACE("FileTransferMessage::FileTransferMessage - start")
	itsSourceType=FT_MSG_ITERATE;
	TRACE("FileTransferMessage::FileTransferMessage - end")
}

FileTransferMessage::FileTransferMessage(File* theFile, const char* theDestination)
                    :Message("FileTransferMessage")
{
	TRACE("FileTransferMessage::FileTransferMessage - start")
	itsSourceType=FT_MSG_FILE;
	itsSource=theFile->encodeFullName();
	if(theDestination!=NULL)
		itsDestination=theDestination;
	TRACE("FileTransferMessage::FileTransferMessage - end")
}

FileTransferMessage::FileTransferMessage(Directory* theDir, const char* theDestination)
                    :Message("FileTransferMessage")
{
	TRACE("FileTransferMessage::FileTransferMessage - start")
	itsSourceType=FT_MSG_DIR;
	itsSource=theDir->encodeFullName();
	if(theDestination!=NULL)
		itsDestination=theDestination;
	TRACE("FileTransferMessage::FileTransferMessage - end")
}

FileTransferClient::FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget) 
				   :Client(theName, theHost, thePort, theTarget)
{
	TRACE("FileTransferClient::FileTransferClient - start")
	itsState=FT_STOP;
	itsFile=NULL;
	itsSrcDirectory=NULL;
	itsTxCnt=0;
	itsLastTx=0;
	TRACE("FileTransferClient::FileTransferClient - end")
}

FileTransferClient::FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Encription* theEncr) 
				   :Client(theName, theHost, thePort, theTarget)
{
	TRACE("FileTransferClient::FileTransferClient - start")
	setEncription(theEncr);
	itsState=FT_STOP;
	itsFile=NULL;
	itsSrcDirectory=NULL;
	itsTxCnt=0;
	itsLastTx=0;
	TRACE("FileTransferClient::FileTransferClient - end")
}

FileTransferClient::FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Compression* theCompr) 
				   :Client(theName, theHost, thePort, theTarget)
{
	TRACE("FileTransferClient::FileTransferClient - start")
	setCompression(theCompr);
	itsState=FT_STOP;
	itsFile=NULL;
	itsSrcDirectory=NULL;
	itsTxCnt=0;
	itsLastTx=0;
	TRACE("FileTransferClient::FileTransferClient - end")
}

FileTransferClient::FileTransferClient(const char* theName, const char* theHost,int thePort, const char* theTarget,Encription* theEncr,Compression* theCompr) 
				   :Client(theName, theHost, thePort, theTarget)
{
	TRACE("FileTransferClient::FileTransferClient - start")
	setEncription(theEncr);
	setCompression(theCompr);
	itsState=FT_STOP;
	itsFile=NULL;
	itsSrcDirectory=NULL;
	itsTxCnt=0;
	itsLastTx=0;
	TRACE("FileTransferClient::FileTransferClient - end")
}

FileTransferClient::~FileTransferClient()
{
	TRACE("FileTransferClient::~FileTransferClient - start")
	if(itsSrcDirectory==NULL && itsFile!=NULL)
	{
		itsFile->close();
		delete itsFile;
		itsFile=NULL;
	}
	else if(itsSrcDirectory!=NULL)
	{
		delete itsSrcDirectory;
		itsSrcDirectory=NULL;
		itsFile=NULL;
	}
	TRACE("FileTransferClient::~FileTransferClient - end")
}

bool FileTransferClient::send(File* theFile,const char* theDestinationPath)
{
	TRACE("FileTransferClient::send - start")
	bool ret=false;
	if(itsState==FT_STOP || itsState==FT_FAILED)
	{
		if(theDestinationPath==NULL)
			MessageQueue::post(new FileTransferMessage(theFile,""));
		else
			MessageQueue::post(new FileTransferMessage(theFile,theDestinationPath));
		ret=true;
	}
	else
	{
		WARNING("File transfer not allowed. Already in transmition.")	
	}
	TRACE("FileTransferClient::send - end")
	return ret;
}

bool FileTransferClient::send(Directory* theDir,const char* theDestinationPath)
{
	TRACE("FileTransferClient::send - start")
	bool ret=false;
	if(itsState==FT_STOP || itsState==FT_FAILED)
	{
		if(theDestinationPath==NULL)
			MessageQueue::post(new FileTransferMessage(theDir,""));
		else
			MessageQueue::post(new FileTransferMessage(theDir,theDestinationPath));
		ret=true;
	}
	else
	{
		WARNING("Directory transfer not allowed. Already in transmition.")	
	}
	TRACE("FileTransferClient::send - end")
	return ret;
}

void FileTransferClient::onLocal(Message* theMessage)
{
	TRACE("FileTransferClient::onLocal - start")
	if(theMessage->is("FileTransferMessage"))
	{
		FileTransferMessage* aMessage=(FileTransferMessage*)theMessage;
		if(aMessage->iterate())
		{
			string aPath=itsSrcDirectory->encodeFullName();
			itsFile=itsFileList.front();
			itsFileList.erase(itsFileList.begin());
			itsDestinationPath=itsDstDirectory + itsFile->encodePath().substr(aPath.size());
#ifndef WIN32
	        itsDestinationPath=itsDestinationPath.substr(0,itsDestinationPath.size()-1);
	    	TRACE("Path=" << itsDestinationPath)
#endif
			itsState=FT_STARTING_TX;
			string msg="Transfering " + itsFile->getName() + " to remote directory " + itsDestinationPath;
			LOG(msg.c_str())
			sendHeader();
		}
		else if(aMessage->directory())
		{
			if(itsSrcDirectory!=NULL)
				delete itsSrcDirectory;
	
			itsDstDirectory=aMessage->getDestination();
			if(itsDstDirectory.size()==0)
				itsDstDirectory=".";
			itsSrcDirectory=new Directory(aMessage->getSource());
	
			itsFileList.clear();
			Directory::find(itsFileList,*itsSrcDirectory);
			if(itsFileList.size()>0)
			{
				string aPath=aMessage->getSource();
				itsFile=itsFileList.front();
				itsFileList.erase(itsFileList.begin());
				itsDestinationPath=itsDstDirectory + itsFile->encodePath().substr(aPath.size());
#ifndef WIN32
				itsDestinationPath=itsDestinationPath.substr(0,itsDestinationPath.size()-1);
				TRACE("Path=" << itsDestinationPath)
#endif
				itsState=FT_STARTING_TX;
				string msg="Transfering " + itsFile->getName() + " to remote directory " + itsDestinationPath;
				LOG(msg.c_str())
				sendHeader();
			}
			else
			{
				cleanup();
				onCompletion();
			}
		}
		else
		{
			if(itsSrcDirectory!=NULL)
				delete itsSrcDirectory;

			itsSrcDirectory=NULL;
			itsDstDirectory="";
	
			string aDst=aMessage->getDestination();
			itsFile=new File(aMessage->getSource());
			if(aDst.size()>0)
				itsDestinationPath=aDst;
			else
				itsDestinationPath="";
	
			itsState=FT_STARTING_TX;
			string msg="Transfering " + itsFile->getName() + " to remote directory " + itsDestinationPath;
			LOG(msg.c_str())
			sendHeader();
		}
	}
	TRACE("FileTransferClient::onLocal - end")
}

bool FileTransferClient::fail() 
{
	wait(); 
	bool ret=(itsState==FT_FAILED);
	release();
	return ret; 
} 	

bool FileTransferClient::done() 
{ 
	wait(); 
	bool ret=(itsState==FT_STOP || itsState==FT_FAILED);
	release();	
	return ret; 
}

unsigned short FileTransferClient::percent()
{
	TRACE("FileTransferClient::send - start")
	unsigned short ret=0;
	wait();
	unsigned long size=itsTxCnt*FT_BLOCKSIZE;
	ret=(size>itsFile->getSize()) ? 100 : (unsigned short)(size/itsFile->getSize());
	release();
	TRACE("FileTransferClient::send - end")
	return ret;
}	

bool FileTransferClient::sendHeader()
{
	TRACE("FileTransferClient::sendHeader - start")

	char buffer[FT_BLOCKSIZE];
	itsState=FT_SENDING_HEADER;
	itsTxCnt=0;
	itsTxStructure.free();
	fstream& itsStream=itsFile->open();	
	if(!itsStream) 
	{
		WARNING("Fail to open file")
		cleanup(FT_FAILED);
		return false;
	}

  	unsigned long fsize = itsFile->getSize();
  	itsStream.read(buffer,FT_BLOCKSIZE);
	unsigned long bsize=itsStream.gcount();
	
	if(bsize < FT_BLOCKSIZE)
		itsState=FT_WAIT_LAST_ACK;
	
	StringProperty* aBlockTypeProperty=new StringProperty("BT");
	aBlockTypeProperty->set("HDR");		
	itsTxStructure.add(aBlockTypeProperty);

	StringProperty* aFileNameProperty=new StringProperty("FN");
	aFileNameProperty->set(itsFile->getName());		
	itsTxStructure.add(aFileNameProperty);
	TRACE("File name=" << itsFile->getName())

	if(itsDestinationPath.size()>0)
	{
		StringProperty* aDestinationPathProperty=new StringProperty("DP");
		aDestinationPathProperty->set(itsDestinationPath);		
		itsTxStructure.add(aDestinationPathProperty);
		TRACE("Destination path=" << itsDestinationPath)
	}

	LongIntProperty* aFileSizeProperty=new LongIntProperty("FS");
	aFileSizeProperty->set(fsize);		
	itsTxStructure.add(aFileSizeProperty);
	TRACE("File size=" << fsize)

	LongIntProperty* aBlockCntProperty=new LongIntProperty("BC");
	aBlockCntProperty->set(++itsTxCnt);		
	itsTxStructure.add(aBlockCntProperty);
	TRACE("Block count=" << itsTxCnt)

	LongIntProperty* aBlockSizeProperty=new LongIntProperty("BS");
	aBlockSizeProperty->set(bsize);		
	itsTxStructure.add(aBlockSizeProperty);
	TRACE("Block size=" << bsize)

	StringProperty* aBufferProperty=new StringProperty("BF");
	aBufferProperty->set(buffer,bsize);		
	itsTxStructure.add(aBufferProperty);

	string aString;
	encodeProperties(itsTxStructure,aString);
	sendMessage(aString);
	itsLastTx=Timer::time();

	TRACE("FileTransferClient::sendHeader - end")
	return true;
}

void FileTransferClient::sendBlock()
{
	TRACE("FileTransferClient::sendBlock - start")

	char buffer[FT_BLOCKSIZE];
	itsState=FT_SENDING_BLOCKS;
	itsTxStructure.free();
	fstream& itsStream=itsFile->get();	
  	itsStream.read(buffer,FT_BLOCKSIZE);
	unsigned long bsize=itsStream.gcount();
	if(bsize < FT_BLOCKSIZE)
		itsState=FT_WAIT_LAST_ACK;
	
	LongIntProperty* aTokenProperty=new LongIntProperty("TK");
	aTokenProperty->set(itsSessionToken);		
	itsTxStructure.add(aTokenProperty);
	TRACE("Session token=" << itsSessionToken)

	StringProperty* aBlockTypeProperty=new StringProperty("BT");
	aBlockTypeProperty->set("BLK");		
	itsTxStructure.add(aBlockTypeProperty);

	LongIntProperty* aBlockSizeProperty=new LongIntProperty("BS");
	aBlockSizeProperty->set(bsize);		
	itsTxStructure.add(aBlockSizeProperty);
	TRACE("Block size=" << bsize)

	LongIntProperty* aBlockCntProperty=new LongIntProperty("BC");
	aBlockCntProperty->set(++itsTxCnt);		
	itsTxStructure.add(aBlockCntProperty);
	TRACE("Block count=" << itsTxCnt)

	StringProperty* aBufferProperty=new StringProperty("BF");
	aBufferProperty->set(buffer,bsize);		
	itsTxStructure.add(aBufferProperty);

	string aString;
	encodeProperties(itsTxStructure,aString);
	sendMessage(aString);
	itsLastTx=Timer::time();

	TRACE("FileTransferClient::sendBlock - end")
}

void FileTransferClient::success(string theBuffer)
{
	TRACE("FileTransferClient::success - start")

	decodeProperties(theBuffer,itsResponse);

	FileTransferResult result=FT_VOID;

	Property* aProperty=itsResponse.get("RS");
	if(aProperty!=NULL && aProperty->is(PROPERTY_SHORTINT))
	{
		result=(FileTransferResult)((ShortIntProperty*)aProperty)->get();
		TRACE("Result=" << result)
	}

	aProperty=itsResponse.get("TK");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		itsSessionToken=((LongIntProperty*)aProperty)->get();
		TRACE("Session token=" << itsSessionToken)
	}

	switch(result)
	{
			case FT_NOT_ALLOWED:
			case FT_STREAM_ERROR:
				TRACE("File transfer exception")
				cleanup(FT_FAILED);
				onCompletion();
				break;
			
			case FT_SESSION_STARTED:
			case FT_SESSION_FINISHED:
			case FT_SESSION_CONTINUE:		
				switch(itsState)
				{
					case FT_WAIT_LAST_ACK:
						TRACE("Last acknoledge")
						if(itsSrcDirectory!=NULL && itsFileList.size() > 0)
						{
							MessageQueue::post(new FileTransferMessage());
						}
						else
						{
							cleanup();
							onCompletion();
						}
						break;

					case FT_SENDING_HEADER:
					case FT_SENDING_BLOCKS:
						TRACE("Send new block")
						sendBlock();
						break;
				}
				break;
			
			default:
				TRACE("Return value not allowed")
				cleanup(FT_FAILED);
				onCompletion();			
	}
	
	TRACE("FileTransferClient::success - end")
}

void FileTransferClient::fail(string theError)
{
	TRACE("FileTransferClient::fail - start")
	cleanup(FT_FAILED);
	onCompletion();
	WARNING(theError.c_str())
	TRACE("FileTransferClient::fail - end")
}

void FileTransferClient::onWakeup(Wakeup* theMessage)
{
//	TRACE("FileTransferClient::onWakeup - start")

	if((itsState==FT_SENDING_HEADER || itsState==FT_SENDING_BLOCKS || itsState==FT_WAIT_LAST_ACK) && 
		Timer::time()-itsLastTx > FT_TIMEOUT)
	{
		WARNING("Timeout during transmition")
		cleanup(FT_FAILED);
		onCompletion();
	}

	Client::onWakeup(theMessage);
	
//	TRACE("FileTransferClient::onWakeup - end")
}

void FileTransferClient::cleanup(FileTransferState theState)
{
	TRACE("FileTransferClient::cleanup - start")
	TRACE("State=" << theState)
	itsState=theState;
	if(itsSrcDirectory==NULL && itsFile!=NULL)
	{
		itsFile->close();
		delete itsFile;
		itsFile=NULL;
	}
	else if(itsSrcDirectory!=NULL)
	{
		delete itsSrcDirectory;
		itsSrcDirectory=NULL;
		itsFile=NULL;
	}
	TRACE("FileTransferClient::cleanup - end")
}

void FileTransferClient::onCompletion()
{
	TRACE("FileTransferClient::onCompletion - start")
	if(itsState==FT_STOP)
	{
		string aMsg=string("File transfer completed");
		LOG(aMsg.c_str())
	}
	else if(itsState==FT_FAILED)
	{
		string aMsg=string("File transfer aborted");
		WARNING(aMsg.c_str())
	}
	TRACE("FileTransferClient::onCompletion - end")
}

FileTransferServer::FileTransferServer(const char* theName,Directory* thePath)
				   :Server(theName), itsDirectory(thePath)
{
	TRACE("FileTransferServer::FileTransferServer - start")
	SCHEDULE(this,500);
	TRACE("FileTransferServer::FileTransferServer - end")
}

FileTransferServer::FileTransferServer(const char* theName,Directory* thePath,Encription* theEncr)
				   :Server(theName), itsDirectory(thePath)
{
	TRACE("FileTransferServer::FileTransferServer - start")
	setEncription(theEncr);	
	SCHEDULE(this,500);
	TRACE("FileTransferServer::FileTransferServer - end")
}

FileTransferServer::FileTransferServer(const char* theName,Directory* thePath,Compression* theCompr)
				   :Server(theName), itsDirectory(thePath)
{
	TRACE("FileTransferServer::FileTransferServer - start")
	setCompression(theCompr);
	SCHEDULE(this,500);
	TRACE("FileTransferServer::FileTransferServer - end")
}

FileTransferServer::FileTransferServer(const char* theName,Directory* thePath,Encription* theEncr,Compression* theCompr)
				   :Server(theName), itsDirectory(thePath)
{
	TRACE("FileTransferServer::FileTransferServer - start")
	setEncription(theEncr);	
	setCompression(theCompr);
	SCHEDULE(this,500);
	TRACE("FileTransferServer::FileTransferServer - end")
}

FileTransferServer::~FileTransferServer()
{
	TRACE("FileTransferServer::~FileTransferServer - start")
	
	TRACE("FileTransferServer::~FileTransferServer - end")
}
	
string FileTransferServer::service(string theBuffer)
{
	TRACE("FileTransferServer::service - start")
	string ret;
	ListProperty aResponse;

	decodeProperties(theBuffer,itsRxStructure);

	Property* aProperty=itsRxStructure.get("BT");
	if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
	{
		string btype=((StringProperty*)aProperty)->get();
		TRACE("Block type=" << btype)

		if(btype.compare("HDR")==0)
		{			
			receiveHeader(aResponse);
		}
		else if(btype.compare("BLK")==0)
		{
			receiveBlock(aResponse);
		}
		else
		{
			setResponse(aResponse,FT_NOT_ALLOWED);
		}	
	}

	encodeProperties(aResponse,ret);

	TRACE("FileTransferServer::service - end")
	return ret;
}

void FileTransferServer::setResponse(ListProperty& theResponse,FileTransferResult theCode,unsigned long theToken)
{
	TRACE("FileTransferServer::setResponse - start")
	TRACE("Result=" << theCode)
	TRACE("Token=" << theToken)
	
	ShortIntProperty* aResultProperty=new ShortIntProperty("RS");
	aResultProperty->set(theCode);		
	theResponse.add(aResultProperty);
	
	if(theToken!=0L)
	{
		LongIntProperty* aTokenProperty=new LongIntProperty("TK");
		aTokenProperty->set(theToken);		
		theResponse.add(aTokenProperty);
	}
	TRACE("FileTransferServer::setResponse - end")
}

void FileTransferServer::receiveHeader(ListProperty& theResponse)
{
	TRACE("FileTransferServer::receiveHeader - start")

	string aDestinationPath;
	Property* aProperty=itsRxStructure.get("DP");
	if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
	{
		aDestinationPath=((StringProperty*)aProperty)->get();
		TRACE("Destination path=" << aDestinationPath)
	}
	
	aProperty=itsRxStructure.get("FN");
	if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
	{
		FileTransferSession aNewSession;
		aNewSession.rxcnt=1;
		aNewSession.time=Timer::time();
		
		string fname=((StringProperty*)aProperty)->get();
		TRACE("File mame= " << fname)
		aNewSession.token=RSHash(fname)+Timer::time();

		File* aFile;
		if(aDestinationPath.size()>0)
		{
			Directory* aDir;
			if(aDestinationPath[0]=='.')
			{
				string aPath=itsDirectory.encodeFullName();
				aPath+=aDestinationPath.substr(1);
				aDir=Directory::mkfulldir(aPath.c_str());
			}
			else
			{
				aDir=Directory::mkfulldir(aDestinationPath.c_str());
			}
			
			string msg="Transfering " + fname + " to directory " + aDir->encodeFullName();
			LOG(msg.c_str())
			aFile=aDir->create(fname.c_str());
			aNewSession.file=new File(aFile);
			delete aDir; // delete also aFile
		}
		else
		{
			string msg="Transfering " + fname + " to default directory";
			LOG(msg.c_str())
			aFile=itsDirectory.create(fname.c_str());
			aNewSession.file=new File(aFile);
		}

		fstream& aStream=aNewSession.file->create();

		aNewSession.size=0L;
		aProperty=itsRxStructure.get("FS");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
		{
			aNewSession.size=((LongIntProperty*)aProperty)->get();
			TRACE("File size=" << aNewSession.size)
		}
		
		unsigned long bsize=0;
		aProperty=itsRxStructure.get("BS");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
		{
			bsize=((LongIntProperty*)aProperty)->get();
			TRACE("Block size=" << bsize)
		}

		unsigned long bcount=0;
		aProperty=itsRxStructure.get("BC");
		if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
		{
			bcount=((LongIntProperty*)aProperty)->get();
			TRACE("Block count=" << bcount)
		}

		aProperty=itsRxStructure.get("BF");
		if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
		{
			string buffer=((StringProperty*)aProperty)->get();
			TRACE("Buffer size=" << buffer.size())

			if(bsize==buffer.size() && bcount==1)
			{
				aStream.write(buffer.data(),buffer.size());		
				if(!aStream)
				{
					aNewSession.file->close();
					aNewSession.file->remove();
					delete aNewSession.file;
					setResponse(theResponse,FT_STREAM_ERROR);
					WARNING("Stream error")
				}
				else
				{			
					if(bsize<FT_BLOCKSIZE)
					{
						aNewSession.file->close();
						setResponse(theResponse,FT_SESSION_FINISHED);
						onCompletion(aNewSession.file);
						delete aNewSession.file;
						TRACE("Session finished")
					}
					else
					{
						itsSessions.push_back(aNewSession);
						setResponse(theResponse,FT_SESSION_STARTED,aNewSession.token);
						TRACE("Session started. Token=" << aNewSession.token)
					}
				}
			}
			else
			{
				setResponse(theResponse,FT_NOT_ALLOWED);
				TRACE("Wrong parameters from peer")
			}
		}
		else
		{
			setResponse(theResponse,FT_NOT_ALLOWED);
			TRACE("Wrong parameters from peer")
		}
	}
	else
	{
		setResponse(theResponse,FT_NOT_ALLOWED);
		TRACE("Wrong parameters from peer")
	}

	TRACE("FileTransferServer::receiveHeader - end")
}

void FileTransferServer::receiveBlock(ListProperty& theResponse)
{
	TRACE("FileTransferServer::receiveBlock - start")
	
	Property* aProperty=itsRxStructure.get("TK");
	if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
	{
		unsigned long token=((LongIntProperty*)aProperty)->get();
		TRACE("Session token=" << token)
		
		bool found=false;
		list<FileTransferSession>::iterator i;
		for (i=itsSessions.begin(); i != itsSessions.end(); i++)
		{
			if((*i).token==token)
			{
				found=true;
				break;	
			}			
		}
		
		if(found)
		{
			TRACE("Session found")
			FileTransferSession& aSession=*i;
			
			unsigned long bsize=0;
			aProperty=itsRxStructure.get("BS");
			if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			{
				bsize=((LongIntProperty*)aProperty)->get();
				TRACE("Block size=" << bsize)
			}
	
			unsigned long bcount=0;
			aProperty=itsRxStructure.get("BC");
			if(aProperty!=NULL && aProperty->is(PROPERTY_LONGINT))
			{
				bcount=((LongIntProperty*)aProperty)->get();
				TRACE("Block count=" << bcount)
			}

			aProperty=itsRxStructure.get("BF");
			if(aProperty!=NULL && aProperty->is(PROPERTY_STRING))
			{
				string buffer=((StringProperty*)aProperty)->get();
				TRACE("Buffer size=" << buffer.size())
				TRACE("Session block count=" << aSession.rxcnt+1)
				
				if(bcount<=aSession.rxcnt)
				{
					TRACE("Client retrasmition filtered")
				}
				else if(bsize==buffer.size() && bcount==aSession.rxcnt+1 && (aSession.rxcnt*FT_BLOCKSIZE+bsize)<=aSession.size)
				{
					aSession.time=Timer::time();
					aSession.rxcnt++;

					fstream& aStream=aSession.file->get();
					aStream.write(buffer.data(),buffer.size());			
					if(!aStream)
					{
						WARNING("Stream exception")
						aSession.file->close();
						aSession.file->remove();
						delete aSession.file;
						itsSessions.erase(i);
						setResponse(theResponse,FT_STREAM_ERROR);
					}
					else
					{			
						if(bsize<FT_BLOCKSIZE)
						{
							aSession.file->close(); //close stream
							setResponse(theResponse,FT_SESSION_FINISHED);
							onCompletion(aSession.file);
							delete aSession.file;
							itsSessions.erase(i);
							TRACE("Session finished")
						}
						else
						{
							setResponse(theResponse,FT_SESSION_CONTINUE,aSession.token);
							TRACE("Session continued. Token=" << aSession.token)
						}
					}
				}
				else
				{
					TRACE("Wrong parameters from peer")
					aSession.file->close();
					aSession.file->remove();
					delete aSession.file;
					itsSessions.erase(i);
					setResponse(theResponse,FT_NOT_ALLOWED);
				}
			}
			else
			{
				TRACE("Wrong parameters from peer")
				aSession.file->close();
				aSession.file->remove();
				delete aSession.file;
				itsSessions.erase(i);
				setResponse(theResponse,FT_NOT_ALLOWED);
			}
		}
		else
		{
			TRACE("Session not found")			
			setResponse(theResponse,FT_NOT_ALLOWED);
		}
	}
	else
	{
		TRACE("Wrong parameters from peer")
		setResponse(theResponse,FT_NOT_ALLOWED);
	}

	TRACE("FileTransferServer::receiveBlock - end")
}

void FileTransferServer::onWakeup(Wakeup* theMessage)
{
//	TRACE("FileTransferServer::onWakeup - start")
	
	for(list<FileTransferSession>::iterator i=itsSessions.begin(); i != itsSessions.end(); i++)
	{
		if(Timer::time()-(*i).time>FT_TIMEOUT)
		{
			TRACE("Session timeout. Token=" << (*i).token << " dTime=" << (theMessage->getTime()-(*i).time))
			(*i).file->close();
			(*i).file->remove();
			delete (*i).file;
			itsSessions.erase(i);
			break;
		}			
	}
	
//	TRACE("FileTransferServer::onWakeup - end")
}	

void FileTransferServer::onCompletion(File* theFile)
{
	TRACE("FileTransferServer::onCompletion - start")
	string aMsg=string("File transfer of ")+theFile->encodeFullName()+string(" completed");
	LOG(aMsg.c_str())
	TRACE("FileTransferServer::onCompletion - end")
}	


