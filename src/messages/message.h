// header for p4c message support (internationalisation)
#pragma once

#include <setjmp.h>
#include <iostream>

namespace p4c {
	extern const wchar_t* getMessage(const wchar_t* original, int messageGroup, int stringID);
	extern const wchar_t* getMessage(const wchar_t* original);							// dummy
	extern void FAILURE(const wchar_t* str);
	extern void FAILURE(const std::wstring& str);

	enum MESSAGE_GROUPS {
		GENERAL_MESSAGES,
		GEOMETRY_ERROR_MESSAGES,
		PARAMSPMP
	};

	enum GENERAL_MESSAGES {
		MES_TITLE = 0,
		MES_UNFINISHEDCODING,
		MES_ERRORFILENAME,
		MES_LOGFILE,
		MES_LOGFILE1,
		MES_P4CMENU,
		MES_P4CMENUHINT
	};

	enum GEOMETRY_ERROR_MESSAGES{	// For geometry.lib
		MES_DIFFSCALE = 1000,
		MES_POINTONCENTRE,
		MES_INVALIDARC,
		MES_LOFTUNEQUALSPANCOUNT,
		MES_EQUALSPANCOUNTFAILED,
		MES_CANNOTTRIMSPAN,
		MES_INDEXOUTOFRANGE,
		MES_BAD_VERTEX_NUMBER,
		MES_BAD_REF_OFFSET,
		MES_BAD_SEC_OFFSET,
		MES_ROLLINGBALL4AXIS_ERROR,
		MES_INPUT_EQUALSPANCOUNT,
		MES_INVALIDPLANE
	};

}

using namespace p4c;		// for including file
