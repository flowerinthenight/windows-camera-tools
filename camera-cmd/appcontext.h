/*
 *		Win-camera-tools: Generic camera tools for Windows.
 *		Copyright (C) 2015 Chew Esmero
 *
 *		This file is part of win-camera-tools.
 *
 *		Win-camera-tools is free software: you can redistribute it and/or modify
 *		it under the terms of the GNU General Public License as published by
 *		the Free Software Foundation, either version 3 of the License, or
 *		(at your option) any later version.
 *
 *		Win-camera-tools is distributed in the hope that it will be useful,
 *		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *		GNU General Public License for more details.
 *
 *		You should have received a copy of the GNU General Public License
 *		along with win-camera-tools. If not, see <http://www.gnu.org/licenses/>.
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
