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
// MergeSort A[1..n]
// 1. If the input sequence has only one element –> Return
// 2. Partition the input sequence into two halves
// 3. Sort the two subsequences using the same algorithm
// 4. Merge the two sorted subsequences to form the output sequence
//
// -- Divide and Conquer --
// It is an algorithmic design paradigm that contains the following steps
// – Divide: Break the problem into smaller sub-problems
// – Recur: Solve each of the sub-problems recursively
// – Conquer: Combine the solutions of each of the sub-problems to form the solution of the problem
// 
// Merge Sort – O(N * Log N)
//
//This Temlate verion works only for "vectors" of any native 
//and user defined data types that supports the <=, =(assign) operators.

#include <vector>
using namespace std;

#ifndef WIN32
typedef unsigned int DWORD;
#endif 

//Template merge for vectors of pair<T1,T2> (T1 is used to compare using '.first')

template<class _Type, class _II> void Merge(_II _Left, _II _Mid, _II _Right)
{
	size_t nLeftArrayLen  = (_Mid - _Left) + 1;
	size_t nRightArrayLen = (_Right - _Mid);
 
	_Type LeftArray, RightArray;
	LeftArray.resize(nLeftArrayLen);
	RightArray.resize(nRightArrayLen);
 
	_II itrFirst, itrLast;
 
	//Create the left sub array...
	itrFirst = _Left;
	itrLast  = itrFirst + nLeftArrayLen;
	copy(itrFirst, itrLast, LeftArray.begin());
 
	//Create the right sub array...
	itrFirst = (_Mid + 1);
	itrLast  = itrFirst + nRightArrayLen;
	copy(itrFirst, itrLast, RightArray.begin());
 
	DWORD nLIndex = 0;
	DWORD nRIndex = 0;
	_II _CopyIndex = _Left;

	while (nLIndex < nLeftArrayLen && nRIndex < nRightArrayLen && _CopyIndex <= _Right)
	{
		if (LeftArray[nLIndex].first >= RightArray[nRIndex].first)
			*_CopyIndex++ = LeftArray[nLIndex++];
		else
			*_CopyIndex++ = RightArray[nRIndex++];
	}
	
	//Copy the remaining elements from the left subarray to main array...
	while (nLIndex < nLeftArrayLen && _CopyIndex <= _Right)
		*_CopyIndex++ = LeftArray[nLIndex++];

	//Copy the remaining elements from the right subarray to main array...
	while (nRIndex < nRightArrayLen && _CopyIndex <= _Right)
		*_CopyIndex++ = RightArray[nRIndex++];
}

//Template merge sort for vectors...

template<class _Type, class _II> void MergeSort(_II _Left, _II _Right)
{
	if (_Left < _Right)
	{	
		_II _Mid = _Left + ((_Right - _Left) / 2);
		MergeSort<_Type>(_Left, _Mid);
		MergeSort<_Type>(_Mid + 1, _Right);
		Merge<_Type>(_Left, _Mid, _Right);
	}
}


