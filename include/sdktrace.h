/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#pragma once

#include <Windows.h>
#include <evntrace.h>

#define FN              __FUNCTION__
#define FL              __FILE__
#define LN              __LINE__

#define TL_ERROR        TRACE_LEVEL_ERROR
#define TL_INFO         TRACE_LEVEL_INFORMATION

#define KW_ENTRY        1 << 0
#define KW_EXIT         1 << 1
#define KW_INFO         1 << 2

#if (NTDDI_VERSION > NTDDI_WINBLUE)
//
// New trace logging feature from Win10.
//
#include <TraceLoggingProvider.h>

//
// Short versions of the logging macros. See https://msdn.microsoft.com/en-us/library/windows/desktop/dn904631(v=vs.85).aspx.
//
#define TL_WRITE(p, ...) \
	TraceLoggingWrite(p, "Jenkins", TraceLoggingValue(__FILE__, "FILE:"), TraceLoggingValue(__FUNCTIONW__, "FUNC:"), __VA_ARGS__)

#define TL_VALUE        TraceLoggingValue
#define TL_WIDESTR      TraceLoggingWideString
#define TL_PTR          TraceLoggingPointer
#define TL_BOOL         TraceLoggingBool
#define TL_BOOLEAN      TraceLoggingBoolean
#define TL_HEXUINT32    TraceLoggingHexUInt32
#define TL_WINERR       TraceLoggingWinError
#define TL_NTSTATUS     TraceLoggingNTStatus
#define TL_HR           TraceLoggingHResult
#endif