#ifndef CONFIG_WIN_H_INCLUDED
#define CONFIG_WIN_H_INCLUDED

//  从 Windows 头文件中排除极少使用的信息
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma warning(disable:4996 4819)
// Windows 头文件:
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>

#ifndef ENABLE_SIMD
#define ENABLE_SIMD 1
#endif

// 修复VS2015后 error LNK2019: 无法解析的外部符号 _sscanf等链接错误
#ifdef _MSC_VER
#if (_MSC_VER >=1900)
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif
#endif // _MSC_VER

#ifndef SAFE_COM_FREE
#define SAFE_COM_FREE(x) if(x){(x)->Release(); (x) = nullptr;}
#endif

#define string_format	sprintf_s
#define string_copy		strncpy
#define localtime_r(__time__, __tm__) localtime_s((__tm__), (__time__))

#ifdef _MSC_VER
char * strptime(const char *s, const char *format, struct tm *tm);
#endif // _MSC_VER
/*
* Windows显示加载动态库句柄
*/
typedef HMODULE			DLLHANDLE;
#define dll_open		LoadLibraryA
#define dll_getfunc		GetProcAddress
#define dll_free		FreeLibrary




#endif	// CONFIG_WIN_H_INCLUDED
