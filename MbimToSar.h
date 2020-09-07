#ifndef _MBIM_SAR_MODULE_HEAD_INCLUDE_
#define _MBIM_SAR_MODULE_HEAD_INCLUDE_
/*
* Copyright (c) 2020, 上海移远通信技术股份有限公司 
* All rights reserved.
* Create Date:	2020年4月29日
*****************************************************************
模块名       : sar over mbim
模块依赖     : 
文件实现功能 : mbim协议
作者         : Hilary
版本         : 1.0.0.0
---------------------------------------------------------------------
多线程安全性 : <是>
---------------------------------------------------------------------
修改记录:
日 期				版本				修改人				修改内容
*****************************************************************/

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(_WINDOWS) || defined(_WINDOWS_)
#ifndef
#define __declspec(dllimport)
#endif //

#ifndef
#define __stdcall
#endif

#ifndef STD_CALLBACK
#define STD_CALLBACK __stdcall
#endif
#else
#endif

#pragma pack(push, 1)

//错误码定义
#define MBIMTOSAR_SUCCESS 0			 // 成功
#define MBIMTOSAR_INVALID_POINTER -1 // 无效指针

#define QUEC_MBIM_MS_SAR_MAX_POWER_VALUE 300 /*30dbm*/
#define QUEC_MBIM_MS_SAR_MIN_POWER_VALUE 60	 /*6dbm*/

enum MBIM_SAR_MODE
{
	MBIM_SAR_MODE_UNKNOWN = -1,
	MBIM_SAR_MODE_DEFAULT = 0x00,
	MBIM_SAR_MODE_AT = 0x01,
	MBIM_SAR_MODE_GPIO = 0x02,
	MBIM_SAR_MODE_MBIM = 0x03
};

enum MBIM_SAR_TECH
{
	MBIM_SAR_TECH_UNKNOWN = -1,
	MBIM_SAR_TECH_WCDMA = 0x01,
	MBIM_SAR_TECH_LTE = 0x02
};

//mbim sar band
#ifndef MBIM_SAR_BAND_POWER_DEFINE
#define MBIM_SAR_BAND_POWER_DEFINE
typedef struct
{
	uint8_t sarTech;
	uint8_t sarBand;
	uint16_t sarPower[8];
} MBIM_SAR_BAND_POWER;
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	//************************************
	// 方  法:	MBIMTOSAR_Init
	// 参  数:	void
	// 返回值:	int										// 成功返回 MBIMTOSAR_SUCCESS 失败返回错误码
	// 描  述:	初始化接口库
	//************************************
	int MBIMTOSAR_Init(const char *tty, int use_sock);

	//************************************
	// 方  法:	MBIMTOSAR_UnInit
	// 参  数:	void
	// 返回值:	void
	// 描  述:	反初始化接口库 清理所有资源
	//************************************
	void MBIMTOSAR_UnInit(void);

	//************************************
	// 方  法:	MBIMTOSAR_GetIsMbimReady
	// 参  数:	int
	// 返回值:	int
	// 描  述:	mbim是否准备
	//************************************
	int MBIMTOSAR_GetIsMbimReady(bool *bEnable);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarEnable
	// 参  数:	bool
	// 返回值:	int
	// 描  述:	使能/禁用sar
	//************************************
	int MBIMTOSAR_SetSarEnable(bool bEnable);

	//************************************
	// 方  法:	MBIMTOSAR_GetSarEnable
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	获取sar
	//************************************
	int MBIMTOSAR_GetSarEnable(bool *bEnable);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarMode
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	设置sar模式
	//************************************
	int MBIMTOSAR_SetSarMode(int nMode);

	//************************************
	// 方  法:	MBIMTOSAR_GetSarMode
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	获取sar模式
	//************************************
	int MBIMTOSAR_GetSarMode(int *nMode);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarProfile
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	设置sar表号
	//************************************
	int MBIMTOSAR_SetSarProfile(int nMode, int nTable);

	//************************************
	// 方  法:	MBIMTOSAR_GetSarProfile
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	获取sar表号
	//************************************
	int MBIMTOSAR_GetSarProfile(int nMode, int *nTable);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarProfile
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	设置sar生效哪张表
	//************************************
	int MBIMTOSAR_SetSarTableOn(int nMode, int nTable);

	//************************************
	// 方  法:	MBIMTOSAR_GetSarProfile
	// 参  数:	bool*
	// 返回值:	int
	// 描  述:	获取sar生效表
	//************************************
	int MBIMTOSAR_GetSarTableOn(int nMode, int *nTable);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarState
	// 参  数:	int
	// 返回值:	int
	// 描  述:	设置SAR当前哪个生效索引power
	//************************************
	int MBIMTOSAR_SetSarState(int sartable);

	//************************************
	// 方  法:	MBIMTOSAR_GetSarState
	// 参  数:	int*
	// 返回值:	int
	// 描  述:	获取SAR当前哪个生效索引power
	//************************************
	int MBIMTOSAR_GetSarState(int *sartable);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarValue
	// 参  数:	char*
	// 返回值:	int
	// 描  述:	配置 SAR TX Power NV
	//************************************
	int MBIMTOSAR_SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower);

	//************************************
	// 方  法:	MBIMTOSAR_GetSarValue
	// 参  数:	char*
	// 返回值:	int
	// 描  述:	获取SAR TX Power NV
	//************************************
	int MBIMTOSAR_GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbanpower);

	//************************************
	// 方  法:	MBIMTOSAR_SetSarClear
	// 参  数:
	// 返回值:	int
	// 描  述:	恢复默认
	//************************************
	int MBIMTOSAR_SetSarClear();

#ifdef __cplusplus
}
#endif

#endif //_ONDT_MODULE_HEAD_INCLUDE_
