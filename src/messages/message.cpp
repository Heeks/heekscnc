#include "message.h"

namespace p4c {

	void FAILURE(wchar_t* str) {
		// called for FATAL error, no easy way back
		throw str;
	}

	void FAILURE(std::wstring& str) {
		// called for FATAL error, no easy way back
		throw (wchar_t*)str.data();
	}


	wchar_t* getMessage(wchar_t* original, int messageGroup, int StringID) {
		// returns international string from system yet to be written
		//
		// input
		//		original - string written to code and returned if no string found
		//		messageGroup - the id of the message group (related to file)
		//		StringID - the ID of the string in our string table (to be coded)
		// returns
		//		the International String or original if not defined

		// find a pointer to the international string here!!!!!!!

		return original;
	}

	wchar_t* getMessage(wchar_t* original) {  // temp like above until proper messaging is added
		return original;
	}
}