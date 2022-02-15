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

#include "FileTransfer.h"
#include "Logger.h"
#include <string>
#include <strstream>
using namespace std;

void main_sleep(int val)
{
	DISPLAY("...wait " << val << " secs...")	
	Thread::sleep(val*1000);
}

int main(int argv,char* argc[]) 
{
	DISPLAY("mqftp - File transfer based on MQ4CPP")
	DISPLAY("Copyright (C) 2004-2007  Riccardo Pompeo (Italy)")
	DISPLAY("Details of this project at http://www.sixtyfourbit.org")
	
	bool client=false;
	char* host=NULL;
	char* path=NULL;
	char* file=NULL;
	int hport=0;
	
	try
	{
		if(argv < 4)
		{
			DISPLAY("Client usage: mqftp -c hostip port [filename|dir] [dest_path]")
			DISPLAY("Server usage: mqftp -s port def_path")
			return 0;	
		}

		if(string(argc[1]).compare("-c")==0 && argv==5)
		{
			client=true;
			DISPLAY("Host name=" << argc[2])
			host=argc[2];
			DISPLAY("Host port=" << argc[3])
			hport=atoi(argc[3]);
			DISPLAY("File name=" << argc[4])
			file=argc[4];
		}	
		else if(string(argc[1]).compare("-c")==0 && argv==6)
		{
			client=true;
			DISPLAY("Host name=" << argc[2])
			host=argc[2];
			DISPLAY("Host port=" << argc[3])
			hport=atoi(argc[3]);
			DISPLAY("File name=" << argc[4])
			file=argc[4];
			DISPLAY("Path=" << argc[5])
			path=argc[5];
		}	
		else if(string(argc[1]).compare("-s")==0 && argv==4)
		{
			client=false;
			DISPLAY("Server port=" << argc[2])
			hport=atoi(argc[2]);
			DISPLAY("Path=" << argc[3])
			path=argc[3];
		}	
		else
		{
			DISPLAY("Client usage: mqftp -c hostip port [filename|dir] [dest_path]")
			DISPLAY("Server usage: mqftp -s port def_path")
			return 0;	
		}

		Encription* anEncr=new Rijndael128("M355493qu3u1n9f0"); 

		if(client==true)
		{
			STARTLOGGER("mqftp_client.log")
			LOG("Starting client threads...")

			FileTransferClient* aClient=new FileTransferClient("mqftp_client",host,hport,"mqftp_server",anEncr);
			
			Directory* aDir=Directory::getCurrent();
			Persistent* aPersistent=aDir->get(file);
			if(aPersistent==NULL)
			{
				DISPLAY("Not found in current directory. Try to search as full path...")
				delete aDir;
				Directory aTemp(file);
				aDir=aTemp.upper();
				aDir->search();	
				aPersistent=aDir->get(aTemp.getName().c_str());
			}

			if(aPersistent!=NULL && aPersistent->is("File"))
			{			
				aClient->send((File*)aPersistent,path);
	
				while(!aClient->done()) { Thread::sleep(100); }
				
				if(aClient->fail())
					DISPLAY("File transfer failed")	
				else
					DISPLAY("File transfer completed")
				
				delete aDir;
			}
			else if(aPersistent!=NULL && aPersistent->is("Directory"))
			{
				aClient->send((Directory*)aPersistent,path);
	
				while(!aClient->done()) { Thread::sleep(100); }
				
				if(aClient->fail())
					DISPLAY("Directory transfer failed")	
				else
					DISPLAY("Directory transfer completed")
				
				delete aDir;
			}			
			else
			{
				DISPLAY("File not found");	
			}
		}
		else
		{			
			STARTLOGGER("mqftp_server.log")
			LOG("Starting server threads...")

			Directory* aDir=Directory::getCurrent();
			Persistent* aPath=aDir->get(path);
			if(aPath!=NULL && aPath->is("Directory"))
			{			
				MessageProxyFactory aFactory("MyFactory",hport);
				FileTransferServer* aServer=new FileTransferServer("mqftp_server",(Directory*)aPath,anEncr);
				while(1)
				{
					Thread::sleep(100);
				}

				delete aDir;
			}
			else
			{
				DISPLAY("Path not found in current directory");	
			}
		}

		LOG("...stopping threads")
		Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(Exception& ex) 
	{
		LOG(ex.getMessage().c_str())
		Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}
	catch(...)
	{
		LOG("Unhandled exception")	
		Thread::shutdownInProgress();
		STOPLOGGER()
		STOPREGISTRY()
		STOPTIMER()
	}		

	return 0;
}
