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

#ifndef __FILESYSTEM__
#define __FILESYSTEM__

#include "Exception.h"
#include <vector>
#include <string>
#include <fstream>
using namespace std;

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#define PATH_DELIM '/'

class FileSystemException : public Exception
{
private:
	string msg;
public:
	FileSystemException();
	FileSystemException(string m) { msg=m; };
	FileSystemException(const char* m) { msg=m; };
	virtual ~FileSystemException() {};
	virtual string getMessage() const { return msg; };
};	

#ifdef WIN32
typedef FILETIME FSTimeType;
#else
typedef __time_t FSTimeType;
#endif

typedef struct 
{
	unsigned long size_low;
	unsigned long size_high;
} FSSizeType;	 

class Persistent
{
protected:
	string itsClass;
	vector<string> itsPath;
	bool itsReadOnly;
	bool itsHidden;
	FSTimeType itsCreationTime;
	FSSizeType itsSize;
	
public:
	virtual ~Persistent() {};

	string getClass() { return itsClass; }; 	
	bool is(const char* theClass) { return itsClass==theClass; };

	virtual string getName();
	virtual bool isReadOnly() { return itsReadOnly; };
	virtual void setReadOnly() { itsReadOnly=true; };
	virtual bool isHidden() { return itsHidden; };
	virtual void setHidden() { itsHidden=true; };
	virtual void setCreationTime(FSTimeType& theTime);
	virtual void setSize(FSSizeType& theTime);
	virtual void move(const char* theDestPath);	
	virtual string encodePath();
	virtual string encodeFullName();
	virtual unsigned long getSize() {  return itsSize.size_low; };
	virtual FSSizeType getSizeExt() {  return itsSize; };

protected:
	Persistent(const char* theClass) : itsClass(theClass), itsReadOnly(false), itsHidden(false) {};
	virtual  void decodePath(const char* thePath);
};

class File : public Persistent
{
protected:
	fstream itsStream;

public:
	File(File* theObj); //++v1.7
	File(string theName) : Persistent("File") { decodePath(theName.c_str()); }; 
	File(const char* theName) : Persistent("File") { decodePath(theName); }; 
	virtual ~File() {};

	virtual fstream& open();
	virtual fstream& create();
	virtual fstream& get() { return itsStream; };
	virtual void close();
	
	virtual void remove();
	virtual File* copy(const char* theDestFullName);		
};

class Directory : public Persistent
{
protected:
	vector<Persistent*> itsContent;
	
public:
	Directory(Directory* theObj); //++v1.7
	Directory(string theName) : Persistent("Directory") { decodePath(theName.c_str()); }; 
	Directory(const char* theName) : Persistent("Directory") {  decodePath(theName); }; 
	virtual ~Directory() { free(); };

	virtual void free();
	virtual Directory* upper();

	virtual void search(const char* thePattern="*");
	virtual Persistent* get(const char* theName);

	virtual vector<Persistent*>::iterator getIterator();
	virtual bool testIterator(vector<Persistent*>::iterator& i);
	virtual int size() { return itsContent.size(); }; //++ v1.7
	Persistent* get(unsigned thePosition); // ++v1.7
	
	virtual void rmdir();
	virtual Directory* mkdir(const char* theName);

	virtual File* create(const char* theFileName);
	virtual void copy(File* theFile);

	static Directory* getCurrent();
	static void find(vector<File*>& theResult,Directory& theDir,const char* thePattern="*");
	static void copy(Directory& theDestDir,Directory& theSrcDir);
	static void move(Directory& theDestDir,Directory& theSrcDir);
	static void rmdir(Directory& theDir);
	static Directory* mkfulldir(const char* theFullPath);

 protected:
	static string replaceAll(string strBase, string strOld, string strNew);
};

#endif

