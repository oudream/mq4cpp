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

#ifndef __PROPERTIES__
#define __PROPERTIES__

#include "LinkedList.h"
#include "Exception.h"
#include <fstream>
#include <string>
#include <vector>
using namespace std;

class PropertyException : public Exception
{
private:
	string msg;
public:
	PropertyException();
	PropertyException(string m) { msg=m; };
	PropertyException(const char* m) { msg=m; };
	virtual ~PropertyException() {};
	virtual string getMessage() const { return msg; };
};	

typedef enum PropertyTypeEnum
{
	PROPERTY_NULL,
	PROPERTY_CHAR,
	PROPERTY_SHORTINT,
	PROPERTY_LONGINT,
	PROPERTY_STRING,
	PROPERTY_LIST // V1.3
} PropertyType;

class Property
{
protected:
	string itsName;
	PropertyType itsType;
public:
	Property(string theName, PropertyType theType) : itsName(theName), itsType(theType) {};
	virtual ~Property() {};
	virtual const char* getName() { return itsName.c_str(); };
	virtual bool is(PropertyType theType) { return (itsType==theType); };	
	virtual bool is(string theName) { return (itsName==theName); };	
	virtual void serialize(ostream& theStream)=0;
	virtual void deserialize(istream& theStream)=0;
};

class CharProperty : public Property
{	
protected:
	char itsValue;		

public:
	CharProperty(const char* theName) : Property(theName,PROPERTY_CHAR) {};
	char get() { return itsValue; };
	void set(char theValue) { itsValue=theValue; };
	virtual void serialize(ostream& theStream);
	virtual void deserialize(istream& theStream);
};

class ShortIntProperty : public Property
{	
protected:
	short itsValue;		

public:
	ShortIntProperty(const char* theName) : Property(theName,PROPERTY_SHORTINT) {};
	short get() { return itsValue; };
	void set(short theValue) { itsValue=theValue; };
	virtual void serialize(ostream& theStream);
	virtual void deserialize(istream& theStream);
};

class LongIntProperty : public Property
{	
protected:
	long long itsValue;		

public:
	LongIntProperty(const char* theName) : Property(theName,PROPERTY_LONGINT) {};
	long long get() { return itsValue; };
	void set(long long theValue) { itsValue=theValue; };
	virtual void serialize(ostream& theStream);
	virtual void deserialize(istream& theStream);
};

class StringProperty : public Property
{	
protected:
	string itsValue;		

public:
	StringProperty(const char* theName) : Property(theName,PROPERTY_STRING) {};
	string get() { return itsValue; };
	void set(const char* theValue) { itsValue=theValue; };
	void set(const char* theValue,unsigned long theSize) { itsValue.assign(theValue,theSize); };
	void set(string theValue) { itsValue=theValue; };
	virtual void serialize(ostream& theStream);
	virtual void deserialize(istream& theStream);
};

class ListProperty : public Property, public LinkedList
{
protected:
	enum {NONE, GET, REMOVE, SERIALIZE, DESERIALIZE, GETRECORDSET} itsAction;
	const char* itsPropertyName;
	Property* itsPropertyRet;
	ostream* itsOutStream;
	vector<ListProperty*>* itsRecordset;

public:
	ListProperty(); // V1.3
	ListProperty(const char* theName); // V1.3
	virtual ~ListProperty();
	virtual void add(Property* theProperty);
	virtual Property* get(const char* theName);		
	virtual vector<ListProperty*>* getRecordset();
	virtual void remove(const char* theName);		
	virtual void serialize(ostream& theStream);
	virtual void deserialize(istream& theStream);
	virtual void deserialize(istream& theStream,bool root); //++v1.4

protected:
	virtual bool onIteration(LinkedElement* theElement);
	virtual void deleteObject(void* theObject) { delete (Property*)theObject; }; //++ v1.5		
};

#endif
