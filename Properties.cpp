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
#include "Properties.h"
#include "Trace.h"

void CharProperty::serialize(ostream& theStream)
{
	TRACE("CharProperty::serialize - start")
	char type=(char)itsType;
	unsigned short namelen=itsName.length();
	theStream.write((const char*)&type,sizeof(type));
	theStream.write((const char*)&itsValue,sizeof(itsValue));
	theStream.write((const char*)&namelen,sizeof(namelen));
	theStream.write((const char*)itsName.data(),namelen);	
	TRACE("CharProperty::serialize - end")
}

void CharProperty::deserialize(istream& theStream)
{
	TRACE("CharProperty::deserialize - start")
	if(!theStream) throw PropertyException("Fail during deserialization");	
	unsigned short namelen=0;
	theStream.read((char*)&itsValue,sizeof(itsValue));
	TRACE("value=" << (int)itsValue)
	theStream.read((char*)&namelen,sizeof(namelen));
	TRACE("namelen=" << namelen)
	char* aName=new char[namelen];
	theStream.read(aName,namelen);
	DUMP("Property name",aName,namelen);
	itsName.assign(aName,namelen);
	delete [] aName; //++v1.9
	TRACE("CharProperty::deserialize - end")
}

void ShortIntProperty::serialize(ostream& theStream)
{
	TRACE("ShortIntProperty::serialize - start")
	char type=(char)itsType;
	unsigned short namelen=itsName.length();
	theStream.write((const char*)&type,sizeof(type));
	theStream.write((const char*)&itsValue,sizeof(itsValue));
	theStream.write((const char*)&namelen,sizeof(namelen));
	theStream.write((const char*)itsName.data(),namelen);	
	TRACE("ShortIntProperty::serialize - end")
}

void ShortIntProperty::deserialize(istream& theStream)
{
	TRACE("ShortIntProperty::deserialize - start")
	if(!theStream) throw PropertyException("Fail during deserialization");	
	unsigned short namelen=0;
	theStream.read((char*)&itsValue,sizeof(itsValue));
	TRACE("value=" << itsValue)
	theStream.read((char*)&namelen,sizeof(namelen));
	TRACE("namelen=" << namelen)
	char* aName=new char[namelen];
	theStream.read(aName,namelen);
	DUMP("Property name",aName,namelen);
	itsName.assign(aName,namelen);
	delete [] aName; //++v1.9
	TRACE("ShortIntProperty::deserialize - end")
}
		
void LongIntProperty::serialize(ostream& theStream)
{
	TRACE("LongIntProperty::serialize - start")
	char type=(char)itsType;
	unsigned short namelen=itsName.length();
	theStream.write((const char*)&type,sizeof(type));
	theStream.write((const char*)&itsValue,sizeof(itsValue));
	theStream.write((const char*)&namelen,sizeof(namelen));
	theStream.write((const char*)itsName.data(),namelen);	
	TRACE("LongIntProperty::serialize - end")
}

void LongIntProperty::deserialize(istream& theStream)
{
	TRACE("LongIntProperty::deserialize - start")
	if(!theStream) throw PropertyException("Fail during deserialization");	
	unsigned short namelen=0;
	theStream.read((char*)&itsValue,sizeof(itsValue));
	TRACE("value=" << itsValue)
	theStream.read((char*)&namelen,sizeof(namelen));
	TRACE("namelen=" << namelen)
	char* aName=new char[namelen];
	theStream.read(aName,namelen);
	DUMP("Property name",aName,namelen);
	itsName.assign(aName,namelen);
	delete [] aName; //++v1.9
	TRACE("LongIntProperty::deserialize - end")
}

void StringProperty::serialize(ostream& theStream)
{
	TRACE("StringProperty::serialize - start")
	char type=(char)itsType;
	unsigned short namelen=itsName.length();
	unsigned short valuelen=itsValue.length();
	theStream.write((const char*)&type,sizeof(type));
	theStream.write((const char*)&namelen,sizeof(namelen));
	theStream.write((const char*)&valuelen,sizeof(valuelen));
	theStream.write((const char*)itsName.data(),namelen);	
	theStream.write((const char*)itsValue.data(),valuelen);	
	TRACE("StringProperty::serialize - end")
}

void StringProperty::deserialize(istream& theStream)
{
	TRACE("StringProperty::deserialize - start")
	if(!theStream) throw PropertyException("Fail during deserialization");	
	unsigned short namelen=0;
	unsigned short valuelen=0;
	theStream.read((char*)&namelen,sizeof(namelen));
	TRACE("namelen=" << namelen)
	theStream.read((char*)&valuelen,sizeof(valuelen));
	TRACE("valuelen=" << valuelen)
	char* aName=new char[namelen];
	theStream.read(aName,namelen);
	DUMP("Property name",aName,namelen);
	itsName.assign(aName,namelen);
	char* aValue=new char[valuelen];
	theStream.read(aValue,valuelen);
	DUMP("Property value",aValue,valuelen);
	itsValue.assign(aValue,valuelen);
	delete [] aName; //++v1.9
	delete [] aValue; //++v1.9
	TRACE("StringProperty::deserialize - end")
}

ListProperty::ListProperty()
			 :Property("Root",PROPERTY_LIST) // v1.3
{
	TRACE("ListProperty::ListProperty - start")
	itsAction=ListProperty::NONE;
	itsPropertyName=NULL;
	itsOutStream=NULL;
	itsPropertyRet=NULL;
	TRACE("ListProperty::ListProperty - end")	
}
	
ListProperty::ListProperty(const char* theName)
			 :Property(theName,PROPERTY_LIST) // v1.3
{
	TRACE("ListProperty::ListProperty - start")
	TRACE("ListProperty=" << theName) // v1.3	
	itsAction=ListProperty::NONE;
	itsPropertyName=NULL;
	itsOutStream=NULL;
	itsPropertyRet=NULL;
	itsRecordset=NULL;
	TRACE("ListProperty::ListProperty - end")	
}

ListProperty::~ListProperty()
{
	TRACE("ListProperty::~ListProperty - start")	
	free();
	TRACE("ListProperty::~ListProperty - end")	
}

void ListProperty::add(Property* theProperty)
{
	TRACE("ListProperty::add - start")
	push(theProperty);
	TRACE("ListProperty::add - end")	
}

Property* ListProperty::get(const char* theName)	
{
	TRACE("ListProperty::get - start")	
	TRACE("Property name=" << theName)
	itsAction=ListProperty::GET;
	itsPropertyName=theName;
	itsPropertyRet=NULL;
	forEach();
	TRACE("ListProperty::get - end")
	return itsPropertyRet;
}

vector<ListProperty*>* ListProperty::getRecordset()
{
	TRACE("ListProperty::getRecordset - start")	
	itsAction=ListProperty::GETRECORDSET;
	itsRecordset=new vector<ListProperty*>;
	forEach();
	TRACE("ListProperty::getRecordset - end")
	return itsRecordset;
}

void ListProperty::remove(const char* theName)		
{
	TRACE("ListProperty::remove - start")	
	TRACE("Property name=" << theName)
	itsAction=ListProperty::REMOVE;
	itsPropertyName=theName;
	forEach();
	TRACE("ListProperty::remove - end")
}

void ListProperty::serialize(ostream& theStream)
{
	TRACE("ListProperty::serialize - start")
	char type=(char)itsType;
	unsigned short namelen=itsName.length();
	unsigned short propscnt=elements();
	theStream.write((const char*)&type,sizeof(type)); //++v1.3
	theStream.write((const char*)&namelen,sizeof(namelen)); //++v1.3
	theStream.write((const char*)itsName.data(),namelen); //++v1.3	
	theStream.write((const char*)&propscnt,sizeof(propscnt));	
	itsAction=ListProperty::SERIALIZE;
	itsOutStream=&theStream;
	forEach();
	TRACE("ListProperty::serialize - end")	
}

bool ListProperty::onIteration(LinkedElement* theElement)	
{
	TRACE("ListProperty::onIteration - start")
	Property* aProperty=(Property*)theElement->getObject();
	TRACE("Property=" << aProperty->getName())	
	bool ret=true;
	
	switch(itsAction)
	{
		case ListProperty::GET:
			if(aProperty->is(itsPropertyName))
			{
				itsPropertyRet=aProperty;
				ret=false;
				TRACE("Property found")
			}
			break;

		case ListProperty::GETRECORDSET:
			if(aProperty->is(PROPERTY_LIST) && itsRecordset!=NULL)
			{
				TRACE("Property found")
				itsRecordset->push_back((ListProperty*)aProperty);
			}
			break;

		case ListProperty::REMOVE:
			if(aProperty->is(itsPropertyName))
			{
				theElement->remove();
				delete theElement;
				itsElementCount--;
				delete aProperty;
				TRACE("Property removed")
				ret=false;
			}
			break;

		case ListProperty::SERIALIZE:
			aProperty->serialize(*itsOutStream);
			break;

		default:
			PropertyException("ListProperty::onIteration: Unhandled action");
	}
	
	TRACE("ListProperty::onIteration - end")	
	return ret;
}

void ListProperty::deserialize(istream& theStream,bool root) //++v1.4
{
	TRACE("ListProperty::deserialize(2par) - start")	
	if(!theStream) throw PropertyException("Fail during deserialization");	
	char type;
	if(root==true)
	{
		theStream.read((char*)&type,sizeof(type));
		TRACE("Property type=" << (int)type)
		if(type!=PROPERTY_LIST)
			PropertyException("Root properties list ID not found");
	}

	deserialize(theStream);	
	TRACE("ListProperty::deserialize(2par) - end")	
}

void ListProperty::deserialize(istream& theStream)
{
	TRACE("ListProperty::deserialize - start")	
	if(!theStream) throw PropertyException("Fail during deserialization");	
	unsigned short propscnt;
	char type;
	unsigned short namelen=0;

	theStream.read((char*)&namelen,sizeof(namelen)); //++v1.3
	TRACE("namelen=" << namelen) //++v1.3
	char* aName=new char[namelen]; //++v1.3
	theStream.read(aName,namelen); //++v1.3
	DUMP("Property name",aName,namelen); //++v1.3
	itsName.assign(aName,namelen); //++v1.3
	theStream.read((char*)&propscnt,sizeof(propscnt));
	TRACE("Number of properties=" << propscnt)
	delete [] aName; //++v1.9
	
	for(unsigned short cnt=0; cnt < propscnt; cnt++)
	{
		if(!theStream) throw PropertyException("Fail during deserialization");	
		theStream.read((char*)&type,sizeof(type));
		if(!theStream) throw PropertyException("Fail during deserialization");	
		TRACE("Property type=" << (int)type)

		switch(type)
		{
			case PROPERTY_CHAR:
				{
					TRACE("type=PROPERTY_CHAR")
					CharProperty* aProperty=new CharProperty("Unnamed");
					aProperty->deserialize(theStream);
					remove(aProperty->getName());
					add(aProperty);		
				}
				break;
			
			case PROPERTY_SHORTINT:
				{
					TRACE("type=PROPERTY_SHORTINT")
					ShortIntProperty* aProperty=new ShortIntProperty("Unnamed");
					aProperty->deserialize(theStream);
					remove(aProperty->getName());
					add(aProperty);		
				}
				break;

			case PROPERTY_LONGINT:
				{
					TRACE("type=PROPERTY_LONGINT")
					LongIntProperty* aProperty=new LongIntProperty("Unnamed");
					aProperty->deserialize(theStream);
					remove(aProperty->getName());
					add(aProperty);		
				}
				break;

			case PROPERTY_STRING:
				{
					TRACE("type=PROPERTY_STRING")
					StringProperty* aProperty=new StringProperty("Unnamed");
					aProperty->deserialize(theStream);
					remove(aProperty->getName());
					add(aProperty);		
				}
				break;
				
			case PROPERTY_LIST: // ++v1.3
				{
					TRACE("type=PROPERTY_LIST")
					ListProperty* aProperty=new ListProperty("Unnamed");
					aProperty->deserialize(theStream);
					remove(aProperty->getName());
					add(aProperty);		
				}
				break;

			default:
				throw PropertyException("Property type unknown");	
		}
	}

	TRACE("ListProperty::deserialize - end")	
}

