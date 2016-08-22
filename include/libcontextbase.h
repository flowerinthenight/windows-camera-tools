/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>

class Context {
public:
	Context()
	{
		InitializeCriticalSection(&m_csLock);
	}

	~Context()
	{
		DeleteCriticalSection(&m_csLock);
	}

public:
	CRITICAL_SECTION m_csLock;
};
