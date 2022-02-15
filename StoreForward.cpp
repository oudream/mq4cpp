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
#include "StoreForward.h"
#include "Logger.h"

#define LOGEXT ".tlog"
#define LOGSRC "*.tlog"
#define FWAIT 2*60*1000
#define SCAN_TIME 1000

MessageStorer::MessageStorer(const char* theName, const char* theWorkingPath, const char* theHost, short thePort,const char* theRemoteService)
			  :Observer(theName)
{
	TRACE("MessageStorer::MessageStorer - start")
	itsHost=theHost;
	itsPort=thePort;
	itsService=theRemoteService;

	itsCurDir=Directory::getCurrent();
	itsTlogDir=(Directory*)itsCurDir->get(theWorkingPath);
	if(itsTlogDir==NULL)
	{
		itsTlogDir=itsCurDir->mkdir(theWorkingPath);
		TRACE(itsTlogDir->getName() << " created")		
	}

	itsStartTime=Timer::time();
	itsMsgCnt=0;

	TRACE("MessageStorer::MessageStorer - end")
}

MessageStorer::~MessageStorer()
{
	TRACE("MessageStorer::~MessageStorer - start")
	delete itsCurDir;
	TRACE("MessageStorer::~MessageStorer - end")
}

void MessageStorer::send(string theBuffer)
{
	TRACE("MessageStorer::send - start")
	TRACE("Host=" << itsHost)
	TRACE("Port=" << itsPort)
	TRACE("Service=" << itsService)
	unsigned long aTime=Timer::time();
	TRACE("Time=" << aTime)

	ListProperty itsPropertiesList;
	StringProperty* aSource=new StringProperty("Source");
	aSource->set(getName());
	itsPropertiesList.add(aSource);
	LongIntProperty* aTimestamp=new LongIntProperty("Timestamp");
	aTimestamp->set(aTime);
	itsPropertiesList.add(aTimestamp);
	StringProperty* anHost=new StringProperty("Host");
	anHost->set(itsHost);
	itsPropertiesList.add(anHost);
	ShortIntProperty* aPort=new ShortIntProperty("Port");
	aPort->set(itsPort);
	itsPropertiesList.add(aPort);
	StringProperty* aService=new StringProperty("Service");
	aService->set(itsService);
	itsPropertiesList.add(aService);
	StringProperty* aMsg=new StringProperty("Message");
	aMsg->set(theBuffer);
	itsPropertiesList.add(aMsg);

	char aFileName[256];
	ostrstream aName(aFileName,sizeof(aFileName));
	unsigned long anID=((itsStartTime & 0xffff) << 16) + itsMsgCnt;
	
	aName << getName() << "." << anID << LOGEXT << ends;
	TRACE("Creating '" << aFileName << "' at '" << itsTlogDir->getName() << "'")
	File* aFile=itsTlogDir->create(aFileName);
	fstream& aStream=aFile->create();
	TRACE("Serializing properties....")
	itsPropertiesList.serialize(aStream);
	aFile->close();
	itsMsgCnt++;
		 		
	TRACE("MessageStorer::send - end")
}

TargetHost::TargetHost(const char* theName, const char* theHost,int thePort, const char* theTarget)
	            :Client(theName,theHost,thePort,theTarget)
{
	TRACE("TargetHost::TargetHost - start")	
	itsFile=NULL;
	itsState=STOP;
	itsTime=Timer::time();
	TRACE("TargetHost::TargetHost - end")	
}

TargetHost::~TargetHost()
{
	TRACE("TargetHost::~TargetHost - start")	
	if(itsFile!=NULL)
		delete itsFile;
	TRACE("TargetHost::~TargetHost - end")	
}

string TargetHost::getFileName()
{ 
	if(itsFile!=NULL)
		return itsFile->getName();
	return "";
}

bool TargetHost::send(string theBuffer, string theFileName)
{ 	
	TRACE("TargetHost::send - start")	
	bool ret=Client::send(theBuffer);
	if(ret)
	{
		itsFile=new File(theFileName);
		itsState=SENDING;
	}
	TRACE("TargetHost::send - end")	
	return ret; 	
}

void TargetHost::success(string theBuffer)
{
	TRACE("TargetHost::success - start")	
	itsFile->remove();
	delete itsFile;
	itsFile=NULL;
	itsState=SUCCESS;
	TRACE("TargetHost::success - end")	
}

void TargetHost::fail(string theError)
{
	TRACE("TargetHost::fail - start")	
	delete itsFile;
	itsFile=NULL;
	itsState=FAIL;

	char aMessage[1024];				
	ostrstream aStream(aMessage,sizeof(aMessage));
	aStream  << "Fail to send message " << "' to service '" << itsTarget
			 << "' hosted on '" << itsHost << ":" << itsPort << "'";				
	WARNING(aMessage);				

	TRACE("TargetHost::fail - end")	
}

MessageForwarder::MessageForwarder(const char* theName,const char* theWorkingPath)
				 :Observer(theName)
{
	TRACE("MessageForwarder::MessageForwarder - start")

	itsCurDir=Directory::getCurrent();
	itsTlogDir=(Directory*)itsCurDir->get(theWorkingPath);
	if(itsTlogDir==NULL)
	{
		itsTlogDir=itsCurDir->mkdir(theWorkingPath);
		TRACE(itsTlogDir->getName() << " created")		
	}	

	itsLastScanTime=Timer::time();
	SCHEDULE(this,SCAN_TIME);
	TRACE("MessageForwarder::MessageForwarder - end")
}

MessageForwarder::~MessageForwarder()
{
	TRACE("MessageForwarder::~MessageForwarder - start")
	if(!isShuttingDown())
	{
		for(vector<TargetHost*>::iterator i = itsHostList.begin(); i < itsHostList.end(); ++i)
			delete *i;	
	}
	itsHostList.clear();		
	delete itsCurDir;
	TRACE("MessageForwarder::~MessageForwarder - end")
}
	
void MessageForwarder::onWakeup(Wakeup* theMessage)
{
	TRACE("MessageForwarder::onWakeup - start")
	itsLastScanTime=Timer::time();
	scan();
	purge();
	TRACE("MessageForwarder::onWakeup - end")
}

void MessageForwarder::scan()
{
	TRACE("MessageForwarder::scan - start")

	try
	{	
		itsTlogDir->search(LOGSRC);
		for(vector<Persistent*>::iterator i = itsTlogDir->getIterator(); itsTlogDir->testIterator(i) ; ++i)
		{
			Persistent* anObj=*i;		
			if(anObj->is("File"))
			{
				File* aFile=(File*)anObj;
				string aFileName=aFile->getName();
				TRACE("Processing file name " << aFileName)
	
				bool found=false;
				TRACE("Cache size=" << itsHostList.size())
				for(vector<TargetHost*>::iterator t = itsHostList.begin(); t < itsHostList.end(); ++t)
				{
					TargetHost* aClient=*t;
					if(aClient->getFileName()==aFileName)
					{
						TRACE("File name already recorded")
						found=true;
						break;
					}
				}	
	
				if(!found)
				{
					TRACE("Adding in queue")
					fstream& aStream=aFile->open();
					ListProperty aPropertiesList;
					aPropertiesList.deserialize(aStream,true);
					aFile->close();
					
					TRACE("Find properties")
					string aSource=((StringProperty*)aPropertiesList.get("Source"))->get();			
					unsigned long aTimestamp=((LongIntProperty*)aPropertiesList.get("Timestamp"))->get();			
					string anHost=((StringProperty*)aPropertiesList.get("Host"))->get();			
					unsigned short aPort=((ShortIntProperty*)aPropertiesList.get("Port"))->get();			
					string aService=((StringProperty*)aPropertiesList.get("Service"))->get();			
					string aMessage=((StringProperty*)aPropertiesList.get("Message"))->get();			
		
					string aFullName=aFile->encodeFullName();				
					char aClientName[256];
					ostrstream aName(aClientName,sizeof(aClientName));
					aName << getName() << "(" << aFileName << ")" << ends;
					TargetHost* aClient=new TargetHost(aClientName,anHost.c_str(),aPort, aService.c_str());
					itsHostList.push_back(aClient);
					aClient->send(aMessage,aFullName);
					TRACE("Added file " << aFullName)
				}
			}
		}
	}
	catch(Exception& exc)
	{
		TRACE(exc.getMessage())	
	}

	TRACE("MessageForwarder::scan - end")
}

void MessageForwarder::purge()
{
	TRACE("MessageForwarder::purge - start")

	unsigned long aCurTime=Timer::time();

	if(!isShuttingDown())
	{
		for(vector<TargetHost*>::iterator i = itsHostList.begin(); i < itsHostList.end(); ++i)
		{
			TargetHost* aClient=*i;
			switch(aClient->getState())
			{
				case TargetHost::SUCCESS:
					itsHostList.erase(i);
					delete aClient;
					break;

				case TargetHost::FAIL:
					if(aClient->getCreationTime()-aCurTime > FWAIT)
					{
						itsHostList.erase(i);
						delete aClient;
					}
					break;
			}			
		}	
	}

	TRACE("MessageForwarder::purge - end")
}
