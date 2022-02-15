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
// Directory management documentation link: 
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/base/listing_the_files_in_a_directory.asp
//

#define SILENT
#include "Logger.h"
#include "FileSystem.h"

#ifndef WIN32
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <string.h>
#define IOBUFSIZE 16384
#endif

FileSystemException::FileSystemException() 
{ 
#ifdef WIN32
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	msg=(char*)lpMsgBuf;
    LocalFree(lpMsgBuf);
#else
	char aMsg[256];
	aMsg[0]=0;
	int err=errno;
	msg=strerror(err);
#endif
}

string Persistent::getName()
{
	TRACE("Persistent::getName - start")
	if(itsPath.size()==0)
		throw FileSystemException("Persistent::getName: path descriptor empty");

	string& aString=itsPath[itsPath.size()-1];
	TRACE("Persistent::getName - end")
	return aString;
}

void Persistent::setCreationTime(FSTimeType& theTime)
{
	TRACE("Persistent::setCreationTime - start")
#ifdef WIN32
	itsCreationTime.dwLowDateTime=theTime.dwLowDateTime;
	itsCreationTime.dwHighDateTime=theTime.dwHighDateTime;	
#else
	itsCreationTime=theTime;
#endif
	TRACE("Persistent::setCreationTime - end")
}

void Persistent::setSize(FSSizeType& theSize)
{
	TRACE("Persistent::setSize - start")
	itsSize.size_low=theSize.size_low;
	itsSize.size_high=theSize.size_high;
	TRACE("Persistent::setSize - end")
}

void Persistent::decodePath(const char* thePath)
{
	TRACE("Persistent::decodePath - start")
	TRACE("Path=" << thePath)
	itsPath.clear();
	char aBuffer[256];
	istrstream aStream(thePath);
	while(!aStream==false)
	{
		aStream.getline(aBuffer,sizeof(aBuffer),PATH_DELIM);
		if(!aStream==false)
		{
			if(aBuffer[0]!=0)
			{
				TRACE("Value=" << aBuffer)
				itsPath.push_back(aBuffer);
			}
		}
	}		
	TRACE("Persistent::decodePath - end")
}

string Persistent::encodeFullName()
{
	TRACE("Persistent::encodeFullName - start")
#ifdef WIN32 
	string res;
#else
	string res;
	res=PATH_DELIM;
#endif

	if(itsPath.size()>0)
	{
		unsigned cnt=1;
		for(vector<string>::iterator i = itsPath.begin(); i < itsPath.end(); ++i, ++cnt)
		{
			res+=*i;
			if(cnt<itsPath.size())
				res+=PATH_DELIM;
		}	
	}

	TRACE("Result=" << res.c_str())		
	TRACE("Persistent::encodeFullName - end")
	return res;	
}

string Persistent::encodePath()
{
	TRACE("Persistent::encodePath - start")
#ifdef WIN32 
	string res;
#else
	string res;
	res=PATH_DELIM;
#endif

	unsigned size=itsPath.size();
	if(size>0)
	{
		for(unsigned cnt=1; cnt<size; cnt++)
		{
			res+=itsPath[cnt-1];
			res+=PATH_DELIM;
		}	
	}

	TRACE("Result=" << res.c_str())		
	TRACE("Persistent::encodePath - end")
	return res;	
}

void Persistent::move(const char* theDestPath)
{
	TRACE("Persistent::move - start")
	string aSrcName=encodeFullName();
	string aDestName=theDestPath;
	aDestName+=PATH_DELIM;
	aDestName+=getName();
	TRACE("Source=" << aSrcName.c_str())
	TRACE("Destination=" << aDestName.c_str())

#ifdef WIN32
	BOOL ret=MoveFile(aSrcName.c_str(),aDestName.c_str());
	if(ret==FALSE)
		throw FileSystemException();
#else
	if(rename(aSrcName.c_str(),aDestName.c_str())<0)
		throw FileSystemException();
#endif	

	decodePath(aDestName.c_str());
	TRACE("Persistent::move - end")	
}

File::File(File* theObj) : Persistent("File") //++1.7
{
	TRACE("File::File - start")
	itsPath=theObj->itsPath;
	itsReadOnly=theObj->itsReadOnly;
	itsHidden=theObj->itsHidden;
	memcpy(&itsCreationTime,&theObj->itsCreationTime,sizeof(FSTimeType));
	memcpy(&itsSize,&theObj->itsSize,sizeof(FSSizeType));
	TRACE("File::File - end")
}

fstream& File::open()
{
	TRACE("File::open - start")
	string aFullName=encodeFullName();
	itsStream.open(aFullName.c_str(),fstream::in|fstream::binary);
	if(!itsStream)
		throw FileSystemException("Failed to open file");
		
	itsStream.seekg(0, ios::end);
	if(!itsStream)
		throw FileSystemException("Failed to seek file");

  	itsSize.size_low = itsStream.tellg();
	itsSize.size_high = 0L;

  	itsStream.seekg(0, ios::beg);	
	if(!itsStream)
		throw FileSystemException("Failed to seek file");
			
	TRACE("File::open - end")
	return itsStream;
}

fstream& File::create()
{
	TRACE("File::create - start")
	string aFullName=encodeFullName();
	itsStream.open(aFullName.c_str(),fstream::out|fstream::trunc|fstream::binary);
	if(!itsStream)
		throw FileSystemException("Failed to open file");	
	return itsStream;
	TRACE("File::create - end")
}

void File::close()
{
	TRACE("File::close - start")
	itsStream.close();
	TRACE("File::close - end")
}

File* File::copy(const char* theDestFullName)
{
	TRACE("File::copy - start")
	TRACE("Destination=" << theDestFullName)
	string aSrcName=encodeFullName();

#ifdef WIN32
	BOOL success=CopyFile(aSrcName.c_str(),theDestFullName,TRUE);
	if(success==FALSE)
		throw FileSystemException();
#else
	struct stat statbuf;
	int in_fd = ::open(aSrcName.c_str(), O_RDONLY);
	if(in_fd<0)
		throw FileSystemException();

	fstat(in_fd, &statbuf);

	int out_fd = ::open(theDestFullName, O_WRONLY|O_CREAT|O_TRUNC, statbuf.st_mode);
	if(out_fd<0)
	{
		::close(in_fd);
		throw FileSystemException();
	}

	char* aBuffer=new char[IOBUFSIZE];

	unsigned long bytesleft=statbuf.st_size;
	while(bytesleft>0)
	{
		ssize_t bytesread=read(in_fd, aBuffer, ((bytesleft>IOBUFSIZE)? IOBUFSIZE : bytesleft));
		if(bytesread==-1)
		{
			::close(out_fd);
			::close(in_fd);
			throw FileSystemException();
		}
	
		ssize_t bytewritten=write(out_fd, aBuffer, bytesread);
		if(bytewritten==-1 || bytewritten!=bytesread)
		{
			::close(out_fd);
			::close(in_fd);
			throw FileSystemException();
		}

		bytesleft-=bytewritten;
	}

	delete [] aBuffer;

	fsync(out_fd);
	::close(out_fd);
	::close(in_fd);
#endif	

	File* ret=new File(theDestFullName);
	ret->itsHidden=itsHidden;
	ret->itsReadOnly=itsReadOnly;
	ret->itsSize=itsSize;
	memcpy(&ret->itsCreationTime,&itsCreationTime,sizeof(FSTimeType));
	memcpy(&ret->itsSize,&itsSize,sizeof(FSSizeType));

	TRACE("File::copy - end")	
	return ret;
}

void File::remove()
{
	TRACE("File::remove - start")
	string aFullName=encodeFullName();
	
#ifdef WIN32	
	BOOL ret=DeleteFile(aFullName.c_str());
	if(ret==FALSE)
		throw FileSystemException();
#else
	if(::remove(aFullName.c_str())<0)
		throw FileSystemException();
#endif	

	TRACE("File::remove - end")
}	

Directory::Directory(Directory* theObj) : Persistent("Directory") //++1.7
{
	TRACE("Directory::Directory - start")
	itsPath=theObj->itsPath;
	itsReadOnly=theObj->itsReadOnly;
	itsHidden=theObj->itsHidden;
	memcpy(&itsCreationTime,&theObj->itsCreationTime,sizeof(FSTimeType));
	memcpy(&itsSize,&theObj->itsSize,sizeof(FSSizeType));
	TRACE("Directory::Directory - end")
}

Directory* Directory::upper() //++1.7
{
	TRACE("Directory::upper")
	return new Directory(encodePath());
}

Directory* Directory::getCurrent()
{
	TRACE("Directory::getCurrent - start")
#ifdef WIN32
	char buf[256];
	BOOL success=GetCurrentDirectory(sizeof(buf),buf);
	if(success==FALSE)
		throw FileSystemException();	
	TRACE("Current directory=" << buf)
	string path = replaceAll(string(buf), "\\", "/");
	Directory* ret=new Directory(path);
	ret->search();
#else
#ifdef __FreeBSD__
	char* ptr=getcwd (NULL, PATH_MAX);
#else
	char* ptr=get_current_dir_name();
#endif
	TRACE("Current directory=" << ptr)
	Directory* ret=new Directory(ptr);
	ret->search();
	::free(ptr);
#endif
	TRACE("Directory::getCurrent - end")
	return ret;
}

void Directory::free()
{
	TRACE("Directory::free - start")
	if(itsContent.size()>0)
	{
		for(vector<Persistent*>::iterator i = itsContent.begin(); i < itsContent.end(); ++i)
		{
			delete *i;
		}	
	}
	itsContent.clear();	
	TRACE("Directory::free - end")
}

string Directory::replaceAll(string strBase, string strOld, string strNew)
{
	TRACE("Directory::replaceAll - start")
	unsigned long iIndex1 = strBase.find(strOld, 0);
	unsigned long iIndex2 = 0;
	unsigned long iLengthOld = strOld.length();
	unsigned long iLengthNew = strNew.length();
	while (iIndex1 != string::npos)
	{
		iIndex2 = iIndex1 + iLengthNew + 1;
		strBase = strBase.erase(iIndex1, iLengthOld);
		strBase = strBase.insert(iIndex1, strNew);
		iIndex1 = strBase.find(strOld, iIndex2);
	}
	TRACE("Directory::replaceAll - end")
	return strBase;
}

void Directory::search(const char* thePattern) 
{
	TRACE("Directory::search - start")
	TRACE("Pattern=" << thePattern)

	free();
	string aCurPath=encodeFullName();

#ifdef WIN32	
	aCurPath+=PATH_DELIM;
	string aDirSpec=aCurPath;
	aDirSpec+=thePattern;
	TRACE("Search path=" << aDirSpec.c_str())

	if(aCurPath=="/") //++v1.7
	{
	   BOOL bFlag;
	   TCHAR Buf[MAX_PATH];          // temporary buffer for volume name
	   TCHAR Drive[] = TEXT("c:\\"); // template drive specifier
	   TCHAR I;                      // generic loop counter
	
	   for (I = TEXT('a'); I < TEXT('z');  I++ ) 
	   {
	      // Stamp the drive for the appropriate letter.
	      Drive[0] = I;
	
	      bFlag = GetVolumeNameForVolumeMountPoint(
	                 Drive,   // input volume mount point or directory
	                 Buf,     // output volume name buffer
	                 MAX_PATH // size of volume name buffer
	              );
	
	      if (bFlag) 
	      {
				TRACE("Found drive=" << Drive)
				string drv;
				drv+=(char)Drive[0];
				drv+=":";
				itsContent.push_back(new Directory(drv));
	      }
	   }
	}
	else
	{
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFile(aDirSpec.c_str(), &FindFileData);
	
		if (hFind == INVALID_HANDLE_VALUE) 
			throw FileSystemException();
	
		do 
		{
			TRACE("Current file name is " << FindFileData.cFileName)
			string aFileName=FindFileData.cFileName;
			string aFullName=aCurPath+aFileName;
	
			if(aFileName!="." && aFileName!="..") //++v1.7
			{ 
				Persistent* aPersistent=NULL;			 			
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					aPersistent=new Directory(aFullName);
				else
					aPersistent=new File(aFullName);
				
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
					aPersistent->setReadOnly();
				
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
					aPersistent->setHidden();
		
				if(FindFileData.ftCreationTime.dwLowDateTime!=0 && FindFileData.ftCreationTime.dwHighDateTime!=0)
					aPersistent->setCreationTime(FindFileData.ftCreationTime);
				else
					aPersistent->setCreationTime(FindFileData.ftLastAccessTime);
		
				FSSizeType aSize;
				aSize.size_low=FindFileData.nFileSizeLow;
				aSize.size_high=FindFileData.nFileSizeHigh;
				aPersistent->setSize(aSize);
		
				itsContent.push_back(aPersistent);
			}
		}
		while (FindNextFile(hFind, &FindFileData) != 0); 
		FindClose(hFind);
	}
#else
	string pattern = replaceAll(thePattern, ".", "\\.");
	pattern = replaceAll(pattern, "*", ".*");
	pattern = pattern + "$";
	TRACE("Pattern=" << pattern.c_str())

	regex_t regex;
	if (regcomp(&regex, pattern.c_str(), REG_EXTENDED|REG_NOSUB) == -1)
		throw FileSystemException();

	dirent* dp=NULL;
	DIR* dirp = opendir(aCurPath.c_str()); 
	if(dirp==NULL)
		throw FileSystemException();

	while ((dp = readdir(dirp))!= NULL)
	{
	    if (regexec(&regex, dp->d_name, 0, NULL, 0) == 0)
	    {
			TRACE("Current file name is " << dp->d_name)
			string aFileName=dp->d_name;
			string aFullName=aCurPath;
			aFullName+=PATH_DELIM;
			aFullName+=aFileName;
			TRACE("Full path name=" << aFullName.c_str())

			if(aFileName!="." && aFileName!="..") //++v1.7
			{ 
				Persistent* aPersistent=NULL;
				struct stat aStat;
				if(stat(aFullName.c_str(),&aStat)<0)
					throw FileSystemException();

				if(dp->d_type==DT_UNKNOWN)
				{
					// Fix reiserfs problem on kernel 2.6
					if((aStat.st_mode & S_IFDIR))
						aPersistent=new Directory(aFullName);
					else
						aPersistent=new File(aFullName);
				}
				else if(dp->d_type==DT_DIR)
					aPersistent=new Directory(aFullName);
				else
					aPersistent=new File(aFullName);
				
				if(!(aStat.st_mode & S_IWUSR))
					aPersistent->setReadOnly();
				
				if(dp->d_name[0]=='.')
					aPersistent->setHidden();
		
				aPersistent->setCreationTime(aStat.st_mtime);
		
				FSSizeType aSize;
				aSize.size_low=aStat.st_size;
				aSize.size_high=0;
				aPersistent->setSize(aSize);
		
				itsContent.push_back(aPersistent);
			}
	    }
	}
	closedir(dirp);
  	regfree(&regex);	
#endif

	TRACE("Directory::search - end")
}

Persistent* Directory::get(const char* theName)
{
	TRACE("Directory::get - start")
	TRACE("Searching for " << theName)
	Persistent* res=NULL;
	if(itsContent.size()>0)
	{
		for(vector<Persistent*>::iterator i = itsContent.begin(); i < itsContent.end(); ++i)
		{
			Persistent* anObj=*i;
			if(anObj->getName()==theName)
			{
				res=anObj;
				break;
			}
		}	
	}
	TRACE("Directory::get - end")
	return res;
}

Persistent* Directory::get(unsigned thePosition) // ++v1.7
{
	TRACE("Directory::get - start")
	TRACE("Position=" << thePosition)
	Persistent* res=NULL;
	if(thePosition < itsContent.size())
	{
		res=itsContent[thePosition];
	}
	TRACE("Directory::get - end")
	return res;
}

vector<Persistent*>::iterator Directory::getIterator()
{
	return itsContent.begin();
}

bool Directory::testIterator(vector<Persistent*>::iterator& i)
{
	return (i < itsContent.end());
}

void Directory::find(vector<File*>& theResult,Directory& theDir,const char* thePattern)
{
	TRACE("Directory::find - start")
	theDir.search(thePattern);
	for(vector<Persistent*>::iterator i = theDir.getIterator(); theDir.testIterator(i) ; ++i)
	{
		Persistent* anObj=*i;		
		if(anObj->is("File"))
		{
			theResult.push_back((File*)anObj);
		}
		else if(anObj->is("Directory"))
		{
			find(theResult,(Directory&)*anObj,thePattern);
		}
	}
	TRACE("Directory::find - end")	
}

void Directory::copy(Directory& theDestDir,Directory& theSrcDir)
{
	TRACE("Directory::copy - start")
	theSrcDir.search();
	for(vector<Persistent*>::iterator i = theSrcDir.getIterator(); theSrcDir.testIterator(i) ; ++i)
	{
		Persistent* anObj=*i;		
		if(anObj->is("File"))
		{
			theDestDir.copy((File*)anObj);
		}
		else if(anObj->is("Directory"))
		{
			string aName=theSrcDir.getName();
			Directory* aDir=theDestDir.mkdir(aName.c_str());
			copy(*aDir,(Directory&)*anObj);
		}
	}
	TRACE("Directory::copy - end")		
}

void Directory::rmdir()
{
	TRACE("Directory::rmdir - start")
	string aFullName=encodeFullName();

#ifdef WIN32		
	BOOL ret=RemoveDirectory(aFullName.c_str());
	if(ret==FALSE)
		throw FileSystemException();	
#else
	if(::rmdir(aFullName.c_str())<0)
		throw FileSystemException();
#endif

	TRACE("Directory::rmdir - end")
}

void Directory::rmdir(Directory& theDir)
{
	TRACE("Directory::rmdir - start")
	theDir.search();
	for(vector<Persistent*>::iterator i = theDir.getIterator(); theDir.testIterator(i) ; ++i)
	{
		Persistent* anObj=*i;		
		if(anObj->is("File"))
		{
			((File*)anObj)->remove();
		}
		else if(anObj->is("Directory"))
		{
			rmdir((Directory&)*anObj);
		}
	}
	theDir.rmdir();
	TRACE("Directory::rmdir - end")	
}

void Directory::move(Directory& theDestDir,Directory& theSrcDir)
{
	TRACE("Directory::move - start")
	theSrcDir.search();
	for(vector<Persistent*>::iterator i = theSrcDir.getIterator(); theSrcDir.testIterator(i) ; ++i)
	{
		Persistent* anObj=*i;		
		if(anObj->is("File"))
		{
			theDestDir.copy((File*)anObj);
			((File*)anObj)->remove();
		}
		else if(anObj->is("Directory"))
		{
			string aName=theSrcDir.getName();
			Directory* aDir=theDestDir.mkdir(aName.c_str());
			move(*aDir,(Directory&)*anObj);
		}
	}
	theSrcDir.rmdir();
	TRACE("Directory::move - end")		
}

Directory* Directory::mkdir(const char* theName)
{
	TRACE("Directory::mkdir - start")
	string aFullName=encodeFullName();
	aFullName+=PATH_DELIM;
	aFullName+=theName;

#ifdef WIN32		
	BOOL ret=CreateDirectory(aFullName.c_str(),NULL);
	if(ret==FALSE)
		throw FileSystemException();
#else
	if(::mkdir(aFullName.c_str(),S_IWUSR|S_IRUSR|S_IXUSR|S_IWGRP|S_IRGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH)<0)
		throw FileSystemException();
#endif		

	Directory* aDir=new Directory(aFullName);
	itsContent.push_back(aDir);
	TRACE("Directory::mkdir - end")
	return aDir;		
}

Directory* Directory::mkfulldir(const char* theFullPath)
{
	TRACE("Directory::mkfulldir - start")
	TRACE("Path=" << theFullPath)
	char aBuffer[256];
	Directory* aDir=NULL;
	Directory* aRoot=NULL;
	Directory* ret=NULL;
	istrstream aStream(theFullPath);

	while(!aStream==false)
	{
		aStream.getline(aBuffer,sizeof(aBuffer),PATH_DELIM);
		if(!aStream==false && aBuffer[0]!=0)
		{
			if(aDir==NULL)
			{
				aRoot=new Directory(aBuffer);
				aDir=aRoot;
			}
			else
			{
				aDir->search();
				Persistent* aPersistent=aDir->get(aBuffer);
				if(aPersistent!=NULL && aPersistent->is("Directory"))
				{
					TRACE("Found dir=" << aBuffer)
					aDir=(Directory*)aPersistent;
				}
				else
				{
					TRACE("Create new dir=" << aBuffer)
					aDir=aDir->mkdir(aBuffer);
				}
			}
		}
	}
	
	if(aDir!=NULL)
		ret=new Directory(aDir);
	else
		throw FileSystemException();

	if(aRoot!=NULL)
		delete aRoot;
			
	TRACE("Directory::mkfulldir - end")
	return ret;
}

File* Directory::create(const char* theFileName)
{
	TRACE("Directory::create - start")
	string aFullName=encodeFullName();
	aFullName+=PATH_DELIM;
	aFullName+=theFileName;
	File* aFile=new File(aFullName.c_str());
	itsContent.push_back(aFile);
	TRACE("Directory::create - end")
	return aFile;
}

void Directory::copy(File* theFile)
{
	TRACE("Directory::copy - start")
	string aFullName=encodeFullName();
	aFullName+=PATH_DELIM;
	aFullName+=theFile->getName();
	File* aFile=theFile->copy(aFullName.c_str());
	itsContent.push_back(aFile);
	TRACE("Directory::copy - end")		
}

void testFS()
{
	char* TMPDIR="/tmp";
	char* TLOGDIR="/tmp/tlog/test_copy.log";
	char* TLOG1DIR="/tmp/tlog1";

	DISPLAY("Start test")
	Directory* aTlogDir=NULL;
	Directory* aTlogDir1=NULL;
	
	Directory* aCurDir=Directory::getCurrent();
	Directory* aTmpDir=new Directory(TMPDIR);
	aTmpDir->search();

	aTlogDir=(Directory*)aTmpDir->get("tlog");
	if(aTlogDir==NULL)
	{
		aTlogDir=aTmpDir->mkdir("tlog");
		DISPLAY(aTlogDir->getName() << " created")		
	}	

	aTlogDir1=(Directory*)aTmpDir->get("tlog1");
	if(aTlogDir1==NULL)
	{
		aTlogDir1=aTmpDir->mkdir("tlog1");		
		DISPLAY(aTlogDir1->getName() << " created")		
	}	

	File* aFile=((Directory*)aTlogDir)->create("test.log");
	fstream& aStream=aFile->create();
	aStream << "Hello World!";
	aFile->close();
	DISPLAY(aFile->getName() << " created")		

	aFile->move(TLOG1DIR);
	File* aFile1=aFile->copy(TLOGDIR);
	DISPLAY(aFile1->getName() << " copied")		

	aFile->remove();	
	aFile1->remove();
	aTlogDir->rmdir();
	aTlogDir1->rmdir();
	
	delete aCurDir;
	delete aTmpDir;	
	DISPLAY("Test done")		
}



