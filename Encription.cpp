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
#include "Trace.h"
#include "Logger.h"
#include "Encription.h"
#include "Thread.h"
#include "GeneralHashFunctions.h"

#define R128SIZE 16
#define R256SIZE 32

Rijndael128::Rijndael128()
{
	TRACE("Rijndael128::Rijndael128 - start")
	byte aKey[]="sixtyfourbit.org";
	rijndael_128_LTX__mcrypt_set_key(&itsRI, &aKey[0], R128SIZE);
	TRACE("Rijndael128::Rijndael128 - end")
}

Rijndael128::Rijndael128(string theKey)
{
	TRACE("Rijndael128::Rijndael128 - start")
	if(theKey.length()!=R128SIZE)
		throw ThreadException("Rijndael128:Key size not allowed");
	
	rijndael_128_LTX__mcrypt_set_key(&itsRI, (byte*)theKey.data(), R128SIZE);
	TRACE("Rijndael128::Rijndael128 - end")
}

string Rijndael128::code(string& theBuffer)
{
	TRACE("Rijndael128::code - start")
	string aReturnString;
	int blkcnt=theBuffer.length()/R128SIZE;
	int rem=theBuffer.length()%R128SIZE;
	byte* ptr=(byte*)theBuffer.data();
	byte aBuffer[R128SIZE];

	for(int cnt=0; cnt < blkcnt; cnt++)	
	{
		memcpy(aBuffer,ptr+cnt*R128SIZE,R128SIZE);	
		rijndael_128_LTX__mcrypt_encrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R128SIZE);
	}
	
	if(rem>0)
	{
		memset(aBuffer,0,R128SIZE);
		memcpy(aBuffer,ptr+blkcnt*R128SIZE,rem);	
		rijndael_128_LTX__mcrypt_encrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R128SIZE);
	}
	
	TRACE("Rijndael128::code - end")
	return aReturnString;
}

string Rijndael128::decode(string& theBuffer)
{
	TRACE("Rijndael128::decode - start")
	string aReturnString;
	int blkcnt=theBuffer.length()/R128SIZE;
	int rem=theBuffer.length()%R128SIZE;
	byte* ptr=(byte*)theBuffer.data();
	byte aBuffer[R128SIZE];

	for(int cnt=0; cnt < blkcnt; cnt++)	
	{
		memcpy(aBuffer,ptr+cnt*R128SIZE,R128SIZE);	
		rijndael_128_LTX__mcrypt_decrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R128SIZE);
	}
	
	if(rem>0)
	{
		memset(aBuffer,0,R128SIZE);
		memcpy(aBuffer,ptr+blkcnt*R128SIZE,rem);	
		rijndael_128_LTX__mcrypt_decrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R128SIZE);
	}
	
	TRACE("Rijndael128::decode - end")
	return aReturnString;
}

Rijndael256::Rijndael256()
{
	TRACE("Rijndael256::Rijndael256 - start")
	byte aKey[]="SuperKal1Frag1lySp1keSp1ral1d0s0";
	rijndael_256_LTX__mcrypt_set_key(&itsRI, &aKey[0], R256SIZE);
	TRACE("Rijndael256::Rijndael256 - end")
}

Rijndael256::Rijndael256(string theKey)
{
	TRACE("Rijndael256::Rijndael256 - start")
	if(theKey.length()!=R256SIZE)
		throw ThreadException("Rijndael256:Key size not allowed");
	
	//BUFFER((char*)theKey.c_str(),theKey.size())
	
	rijndael_256_LTX__mcrypt_set_key(&itsRI, (byte*)theKey.data(), R256SIZE);
	TRACE("Rijndael256::Rijndael256 - end")
}

string Rijndael256::code(string& theBuffer)
{
	TRACE("Rijndael256::code - start")
	string aReturnString;
	int blkcnt=theBuffer.length()/R256SIZE;
	int rem=theBuffer.length()%R256SIZE;
	byte* ptr=(byte*)theBuffer.data();
	byte aBuffer[R256SIZE];

	for(int cnt=0; cnt < blkcnt; cnt++)	
	{
		memcpy(aBuffer,ptr+cnt*R256SIZE,R256SIZE);	
		rijndael_256_LTX__mcrypt_encrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R256SIZE);
	}
	
	if(rem>0)
	{
		memset(aBuffer,0,R256SIZE);
		memcpy(aBuffer,ptr+blkcnt*R256SIZE,rem);	
		rijndael_256_LTX__mcrypt_encrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R256SIZE);
	}
	
	TRACE("Rijndael256::code - end")
	return aReturnString;
}

string Rijndael256::decode(string& theBuffer)
{
	TRACE("Rijndael256::decode - start")
	string aReturnString;
	int blkcnt=theBuffer.length()/R256SIZE;
	int rem=theBuffer.length()%R256SIZE;
	byte* ptr=(byte*)theBuffer.data();
	byte aBuffer[R256SIZE];

	for(int cnt=0; cnt < blkcnt; cnt++)	
	{
		memcpy(aBuffer,ptr+cnt*R256SIZE,R256SIZE);	
		rijndael_256_LTX__mcrypt_decrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R256SIZE);
	}
	
	if(rem>0)
	{
		memset(aBuffer,0,R256SIZE);
		memcpy(aBuffer,ptr+blkcnt*R256SIZE,rem);	
		rijndael_256_LTX__mcrypt_decrypt(&itsRI,&aBuffer[0]);
		aReturnString.append((const char*)&aBuffer[0],R256SIZE);
	}
	
	TRACE("Rijndael256::decode - end")
	return aReturnString;
}

string Encription::generateKey128(string theString)
{
	TRACE("Encription::generateKey128 - start")
	TRACE("Seed=" << theString)
	string ret;
	
	ret+=toString(RSHash(theString));
	ret+=toString(JSHash(theString));
	ret+=toString(PJWHash(theString));
	ret+=toString(ELFHash(theString));
		
	TRACE("Encription::generateKey128 - end")
	return ret;
}

string Encription::generateKey256(string theString)
{
	TRACE("Encription::generateKey256 - start")
	TRACE("Seed=" << theString)
	string ret;

	ret+=toString(RSHash(theString));
	ret+=toString(JSHash(theString));
	ret+=toString(PJWHash(theString));
	ret+=toString(ELFHash(theString));
	ret+=toString(BKDRHash(theString));
	ret+=toString(SDBMHash(theString));
	ret+=toString(DJBHash(theString));
	ret+=toString(APHash(theString));
	
	//BUFFER((char*)ret.c_str(),ret.size())
	
	TRACE("Encription::generateKey256 - end")
	return ret;
}

string Encription::toString(unsigned int theValue)
{
	TRACE("Encription::toString - start")
	string ret;

	ret+=(char)((theValue >>  0) & 0xff); 
	ret+=(char)((theValue >>  8) & 0xff); 
	ret+=(char)((theValue >> 16) & 0xff); 
	ret+=(char)((theValue >> 24) & 0xff); 
	
	TRACE("Encription::toString - end")
	return ret;
}


