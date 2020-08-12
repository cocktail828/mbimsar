#ifndef CONFIG_H
#define CONFIG_H

#include <atomic>
#include <thread>
#include <memory>
#include <stdint.h>

// 单体全局原子标志
static std::atomic_flag global_single_flag = ATOMIC_FLAG_INIT;

// 配置WINDOWS下执行时字符集为UTF8
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(_WINDOWS) || defined(_WINDOWS_)
/*
* Windows预编译系统宏定义
*/
#define BUILD_WITH_WINDOWS 1
#define BUILD_WITH_LINUX 0
#define BUILD_LITTLE_ENDIAN 1
#define DIR_CHARACTER '\\'

#if defined(APP_EXEC_WITH_UTF8)
#pragma execution_character_set("utf-8")
#define EXEC_UTF8_CHAR 1
#else
#define EXEC_UTF8_CHAR 0
#endif // APP_EXEC_WITH_UTF8

/*
* Windows函数以及宏定义
*/
#include "config_win.h"

#ifndef STD_CALL
#define STD_CALL __stdcall
#endif

#ifndef STD_CALLBACK
#define STD_CALLBACK __stdcall
#endif

#else // Linux
/*
* Linux预编译系统宏定义
*/
#include "config_linux.h"
#define BUILD_WITH_WINDOWS 0
#define BUILD_WITH_LINUX 1
#ifdef __ARM_ARCH
#define BUILD_WITH_TX2 1 // 当编译平台为ARM时则认为是TX2平台编译
#else
#define BUILD_WITH_TX2 0 // TX2平台编译
#endif					 //__ARM_ARCH
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BUILD_LITTLE_ENDIAN 1
#else
#define BUILD_LITTLE_ENDIAN 0
#endif

#define DIR_CHARACTER '/'

#define EXEC_UTF8_CHAR 0

/*
* Linux函数以及宏定义
*/
#include "config_linux.h"

#ifndef STD_CALL
#define STD_CALL
#endif

#ifndef STD_CALLBACK
#define STD_CALLBACK
#endif

#endif // _WIN32

/*
* 通用宏定义
*/
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define CONNIFY(x, y) x##y
#define CONN(x, y) CONNIFY(x, y)

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) (void)(x)
#endif // UNREFERENCED_PARAMETER

// 大小对齐 2^n
#ifndef SIZE_ALGIN
#define SIZE_ALGIN(sz, a) (((sz) + (a - 1)) & ~(a - 1))
#endif

// 禁止赋值拷贝
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(_className_)  \
	_className_(const _className_ &) = delete; \
	_className_ &operator=(const _className_ &) = delete
#endif

// 删除堆指针
#ifndef SAFE_PTR_FREE
#define SAFE_PTR_FREE(x) \
	if (x)               \
	{                    \
		delete (x);      \
		(x) = nullptr;   \
	}
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// 删除堆数组
#ifndef SAFE_ARRAY_FREE
#define SAFE_ARRAY_FREE(x) \
	if (x)                 \
	{                      \
		delete[](x);       \
		(x) = nullptr;     \
	}
#endif

// 单体实例对象声明
#ifndef SINGLETON_DECLARE
#define SINGLETON_DECLARE(cls)     \
public:                            \
	static cls *instance(void);    \
                                   \
private:                           \
	DISALLOW_COPY_AND_ASSIGN(cls); \
	static std::atomic<cls *> unique_instance_##cls;
#endif

// 单体实例对象变量初始化
#ifndef SINGLETON_IMPLEMENT
#define SINGLETON_IMPLEMENT(cls)                                               \
	std::atomic<cls *> cls::unique_instance_##cls(nullptr);                    \
	std::shared_ptr<cls> unique_shareptr_##cls(nullptr);                       \
	cls *cls::instance(void)                                                   \
	{                                                                          \
		cls *_inst = unique_instance_##cls;                                    \
		if (!_inst)                                                            \
		{                                                                      \
			while (global_single_flag.test_and_set(std::memory_order_acquire)) \
			{                                                                  \
				std::this_thread::yield();                                     \
			}                                                                  \
			_inst = unique_instance_##cls;                                     \
			if (!_inst)                                                        \
			{                                                                  \
				_inst = new cls();                                             \
				unique_instance_##cls = _inst;                                 \
				unique_shareptr_##cls.reset(_inst);                            \
			}                                                                  \
			global_single_flag.clear(std::memory_order_relaxed);               \
		}                                                                      \
		return _inst;                                                          \
	}
#endif

inline unsigned short ushort_ntoh(unsigned short v)
{
#if BUILD_LITTLE_ENDIAN
	return ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8);
#else
	return v;
#endif
}

inline unsigned int uint_ntoh(unsigned int v)
{
#if BUILD_LITTLE_ENDIAN
	return ((v & 0x000000FF) << 24) | ((v & 0x0000FF00) << 8) | ((v & 0x00FF0000) >> 8) | ((v & 0xFF000000) >> 24);
#else
	return v;
#endif
}

/*
* 编译条件宏定义
*/

#endif // CONFIG_H
