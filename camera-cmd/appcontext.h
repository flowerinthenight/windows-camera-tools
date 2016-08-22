/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include "..\include\libcontextbase.h"

class CContext : public Context {
public:
	CContext() :
		m_subparamstart(0),
		m_pCmdSupported(NULL),
		m_argc(0),
		m_ppargv(NULL),
		m_cchlen(0)
	{}

	~CContext() {}

public:
	size_t m_subparamstart;
	BOOL *m_pCmdSupported;
	int m_argc;
	wchar_t **m_ppargv;
	size_t m_cchlen;
};
