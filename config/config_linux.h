#ifndef CONFIG_LINUX_H_INCLUDED
#define CONFIG_LINUX_H_INCLUDED

#include <unistd.h>
#include <endian.h>
#include <sys/stat.h>

typedef int HRESULT;
#define E_FAIL -1
#define S_OK 0
#define FAILED(v) (!!v)
#ifndef ENABLE_SIMD
#define ENABLE_SIMD 1
#endif

#ifndef SAFE_COM_FREE
#define SAFE_COM_FREE(x) \
    if (x)               \
    {                    \
        (x)->Release();  \
        (x) = nullptr;   \
    }
#endif

#define string_format sprintf_s
#define string_copy strncpy
#define localtime_r(__time__, __tm__) localtime_s((__tm__), (__time__))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
#endif // CONFIG_LINUX_H_INCLUDED
