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

#ifndef __ENCRIPTION__
#define __ENCRIPTION__

#include "rijndael.h"
#include <string>

class Encription
{
public:
	virtual string code(string& theBuffer)=0;
	virtual string decode(string& theBuffer)=0;
	
	static string generateKey128(string theString);
	static string generateKey256(string theString);
	
protected:
	static string toString(unsigned int theValue);	
};

class Rijndael128 : public Encription
{
protected:
	RI itsRI;

public:
	Rijndael128();
	Rijndael128(string theKey);
	virtual ~Rijndael128() {};
	virtual string code(string& theBuffer);
	virtual string decode(string& theBuffer);
};

class Rijndael256 : public Encription
{
protected:
	RI itsRI;

public:
	Rijndael256();
	Rijndael256(string theKey);
	virtual ~Rijndael256() {};
	virtual string code(string& theBuffer);
	virtual string decode(string& theBuffer);
};

#ifdef WIN32
extern "C" 
{
        int rijndael_128_LTX__mcrypt_set_key(RI * rinst, byte * key, int nk);
        void rijndael_128_LTX__mcrypt_encrypt(RI * rinst, byte * buff);
        void rijndael_128_LTX__mcrypt_decrypt(RI * rinst, byte * buff);
        int rijndael_256_LTX__mcrypt_set_key(RI * rinst, byte * key, int nk);
        void rijndael_256_LTX__mcrypt_encrypt(RI * rinst, byte * buff);
        void rijndael_256_LTX__mcrypt_decrypt(RI * rinst, byte * buff);
}
#endif

#endif
