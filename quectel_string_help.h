#ifndef _QUECTEL_STRING_HELPER_
#define _QUECTEL_STRING_HELPER_

#include <string>

#include <stdio.h>

// /*
// wchar * -> std::string
// */
// std::string wchar2string(wchar_t *s);

// bool string2wstring(const std::string &src, std::wstring &dest);
// bool wstring2string(const std::wstring &src, std::string &dest);

// //bool CString2string(const CString& src, std::string& dst);

// /*
// BSTR -> std::string
// */
// std::string bstr2string(BSTR bstr);
// std::wstring bstr2wstring(BSTR bstr);

// //int gsmDecodeSCA(const char *pSrc, CString& strSCA);

// void LogTrace(const TCHAR *pszFormat, const char *str);
// //#define LogTrace(fmt,var) {TCHAR sOut[256];_stprintf_s(sOut,(fmt),var);OutputDebugString(sOut);}

#define OutputDebugString(fmt, args...) fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#endif //_QUECTEL_STRING_HELPER_
