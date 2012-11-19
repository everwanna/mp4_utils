// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#include <string>
#include <iostream>
using namespace std;

#include <boost/cstdint.hpp>
#include <boost/smart_ptr.hpp>
using namespace boost;


typedef unsigned char byte;

#pragma warning(disable:4290)