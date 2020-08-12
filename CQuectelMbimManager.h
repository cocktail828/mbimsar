#pragma once
#include <atomic>
#include <algorithm>
#include <memory>
#include <map>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include "config/config.h"
#include "IDeviceService.h"
#include "quectel_observer.h"
#include "MbimToSar.h"
#pragma pack(push, 1)
using namespace std;

#define QUEC_MBIM_MS_SAR_BACKOFF_SET 0x92F456E8
#define QUEC_MBIM_MS_SAR_BACKOFF_QUERY 0x8204F6F9

typedef enum
{
	QUEC_SVC_MSSAR_NULL_STATE = 0,
	QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET,
	QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET,
	QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET,
	QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET,
	QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET,
	QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET,
	QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET,
	QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET,
	QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET,

	QUEC_SVC_MSSAR_MAX_STATE
} quec_mssar_set_get_state;

typedef struct
{
	unsigned char SetBodySarMode; /*0 HW control by DPR pin----1 SW control*/
	unsigned char SetBodySarProfile /*0 Sar table 0----1 Sar table 1..........*/;
	unsigned char SetBodySarTech /*1==WCDMA  2==LTE*/;
	unsigned short SetBodySarPower[8] /* sar power */;
	unsigned char SetBodySarBand;
	unsigned char SetBodySarOnTable;
} quec_mssar_set_sar_config_req;

typedef struct
{
	unsigned char SetBodySarMode;	   /*0 HW control by DPR pin----1 SW control*/
	unsigned char SetBodySarProfile;   /*0 Sar table 0----1 Sar table 1..........*/
	unsigned char SetBodySarTech;	   /*1==WCDMA 2==LTE*/
	unsigned short SetBodySarPower[8]; /*sar power*/
	unsigned char SetBodySarBand;
	unsigned char SetBodySarOnTable;
} quec_mssar_set_sar_config_resp;

typedef struct
{
	unsigned char GetBodySarMode;	   /*0 HW control by DPR pin----1 SW control*/
	unsigned char GetBodySarProfile;   /*0 Sar table 0----1 Sar table 1..........*/
	unsigned char GetBodySarTech;	   /*1==WCDMA 2==LTE*/
	unsigned short GetBodySarPower[8]; /*sar power*/
	unsigned char GetBodySarBand;
	unsigned char GetBodySarOnTable;
} quec_mssar_get_sar_config_req;

typedef struct
{
	unsigned char GetBodySarMode;	   /*0 HW control by DPR pin----1 SW control*/
	unsigned char GetBodySarProfile;   /*0 Sar table 0----1 Sar table 1..........*/
	unsigned char GetBodySarTech;	   /*1==WCDMA 2==LTE*/
	unsigned short GetBodySarPower[8]; /*sar power*/
	unsigned char GetBodySarBand;
	unsigned char GetBodySarOnTable;
} quec_mssar_get_sar_config_resp;

typedef struct
{
	unsigned long SARMode;
	unsigned long SARBackOffStatus;
	unsigned long SARWifiIntegration;
	unsigned long ElementCount;
	unsigned char data;
} MBIM_SAR_INFO;

typedef struct
{
	unsigned long SARMode;
	unsigned long SARBackOffStatus;
	unsigned long ElementCount;
	unsigned char data;
} MBIM_SAR_SET_INFO;

typedef struct
{
	unsigned long SAROffset;
	unsigned long SARStateSize;
} MBIM_SAR_CONFIG_STATUS;

typedef struct
{
	unsigned long SARAntennaIndex;
	unsigned long SARBAckOffIndex;
} MBIM_SAR_CONFIG_INFO;

class CQuectelMbimManager : public MbnObserver
{
	SINGLETON_DECLARE(CQuectelMbimManager)
public:
	CQuectelMbimManager();
	~CQuectelMbimManager();

	//初始化
	int Init(const char *d);

	//反初始化
	void UnInit();

	//打开设备服务
	int OpenDeviceServices(std::string strInterfaceId);
	//关闭设备服务
	int CloseDeviceServices(/*CQuectelMbimServices* handle*/);

	int GetIsMbimReady(bool *bEnable);

	int SetSarEnable(bool bEnable);

	int GetSarEnable(bool *bEnable);

	int SetSarMode(int nMode);

	int GetSarMode(int *nMode);

	int SetSarProfile(int nMode, int nTable);

	int GetSarProfile(int nMode, int *nTable);

	int SetSarTableOn(int nMode, int nTable);

	int GetSarTableOn(int nMode, int *nTable);

	int GetSarState(int *sartable);

	int SetSarState(int sartable);

	int SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower);

	int GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbandpower);

	int SetSarClear();

	IDeviceService *getdeviceservice();

	void OpenCmdSessionCb();

	void update(MbnSubject *subject, void *notify);

private:
	bool m_init_flag;
	std::mutex m_op_lock;

	bool m_bIsReady;

	bool m_bIsSarEnable;

	bool m_bIsMode;

	int m_nSarTable;

	int m_nSarTableOn;

	int m_nSarIndex;

	int m_nSarMode;

	int m_nSarTech;

	std::string m_strSarValue;

	std::vector<MBIM_SAR_CONFIG_INFO> m_vecSarConfigInfo;

	MBIM_SAR_BAND_POWER m_sar_band_power;

public:
	//设备服务
	IDeviceService *m_deviceservice;

	long m_nDeviceId;

	std::mutex m_event_lock;
	std::condition_variable m_waitevent;

	std::condition_variable m_sarmode_setevent;
	std::condition_variable m_sarmode_getevent;

	std::condition_variable m_sarprofile_setevent;
	std::condition_variable m_sarprofile_getevent;

	std::condition_variable m_sartableon_setevent;
	std::condition_variable m_sartableon_getevent;

	std::condition_variable m_sartablevalue_setevent;
	std::condition_variable m_sartablevalue_getevent;

	std::condition_variable m_sarclear_setevent;
};

typedef CQuectelMbimManager MBIMMANAGER;
