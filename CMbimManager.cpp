#include <string.h>

#include "CMbimManager.h"
#include "MbimMessage.h"
#include "MbimToSar.h"

static const char hex_chars[] = "0123456789ABCDEF";

#define ZeroMemory(p, l) memset(p, 0, l)

std::mutex CMbimManager::mGlobalLock;
bool CMbimManager::mReady;
CMbimManager &CMbimManager::instance()
{
	static CMbimManager instance_;
	return instance_;
}

uint32_t CMbimManager::getRequestId()
{
	return mRequestId;
}

int CMbimManager::GetIsMbimReady(bool *en)
{
	*en = mService.ready();
	return 0;
}

CMbimManager::CMbimManager()
{
	mRequestId = 0;
	m_nSarTable = -1;
	m_vecSarConfigInfo.clear();
	mReady = false;

	m_bIsSarEnable = false;
	m_bIsMode = false;
	m_nSarIndex = -1;
	m_nSarMode = -1;
	m_nSarTech = -1;
	m_nSarTableOn = -1;
}

CMbimManager::~CMbimManager()
{
}

int CMbimManager::Init(const char *d)
{
	std::lock_guard<std::mutex> _lk(mGlobalLock);
	if (mReady)
		return true;

	if (mService.connect(d))
		return -1;

	mReady = true;
	return 0;
}

void CMbimManager::UnInit()
{
	std::lock_guard<std::mutex> _lk(mGlobalLock);
	mReady = false;

	if (!mReady)
		return;

	mService.release();
}

int CMbimManager::OpenDeviceServices(std::string strInterfaceId)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	if (!mService.ready())
		return -1;

	// int ret = E_FAIL;

	// mService.SetDeviceServiceList();
	// BSTR qbi_uuid = _com_util::ConvertStringToBSTR("{68223D04-9F6C-4E0F-822D-28441FB72340}");
	// hr = mbn_deviceservicescontext->GetDeviceService(qbi_uuid, &mService);
	// if (FAILED(ret))
	// 	return -1;
	// std::pair<IConnectionPoint *, DWORD> result_;
	// CDeviceServicesSink *sinkEvent = new(std::nothrow) CDeviceServicesSink();
	// result_ = quectel_resigter_sink(sinkEvent, MBIMMANAGER::instance()->getdeviceservicesmgr());
	// if (result_.first && result_.second != 0)
	// {
	// 	sinkEvent->Attach(this);
	// }
	// else
	// {
	// 	delete sinkEvent;
	// 	sinkEvent = nullptr;

	// 	return -1;
	// }
	// u32 mRequestId;
	// hr = mService.OpenCommandSession(&mRequestId);
	// if (FAILED(ret))
	// 	return -1;
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	delete sinkEvent;
	// 	sinkEvent = nullptr;

	// 	return -1;
	// }

	mReady = true;

	return 0;
}

int CMbimManager::CloseDeviceServices()
{
	std::lock_guard<std::mutex> _lk(mGlobalLock);
	if (!mReady)
		return 0;

	if (mService.ready())
	{
		mService.closeCommandSession(&mRequestId);
	}
	mReady = false;
	return 0;
}

int CMbimManager::SetSarEnable(bool bEnable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	MBIM_SAR_SET_INFO *sarinfo = nullptr;
	unsigned char *msgBuf = nullptr;
	msgBuf = new (std::nothrow) unsigned char[12];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = 1;
	sarinfo->SARBackOffStatus = bEnable;
	sarinfo->ElementCount = 0;

	int ret = mService.setCommand(0x01, msgBuf, 12, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;

	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::GetSarEnable(bool *bEnable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	//查询sar配置
	if (mService.queryCommand(0x01, nullptr, 0, &mRequestId))
	{
		LOGE << "get sar enable failed" << ENDL;
		return -1;
	}
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		*bEnable = 0;
		return -1;
	}

	*bEnable = m_bIsSarEnable;
	return 0;
}

int CMbimManager::SetSarMode(int nMode)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET;
	sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&sarinfo->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	getsar_config->SetBodySarMode = nMode;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

int CMbimManager::GetSarMode(int *nMode)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET;
	sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&sarinfo->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}
	*nMode = m_nSarMode;
	return 0;
}

int CMbimManager::SetSarProfile(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET;
	sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&sarinfo->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	getsar_config->SetBodySarMode = nMode;
	getsar_config->SetBodySarProfile = nTable;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::GetSarProfile(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET;
	sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&sarinfo->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));
	getsar_config->GetBodySarMode = nMode;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	*nTable = m_nSarTable;
	return 0;
}

int CMbimManager::SetSarTableOn(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET;
	sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&sarinfo->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	getsar_config->SetBodySarMode = m_nSarMode;
	getsar_config->SetBodySarOnTable = nTable;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::GetSarTableOn(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET;
	sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&sarinfo->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));
	getsar_config->GetBodySarMode = nMode;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}
	*nTable = m_nSarTableOn;
	return 0;
}

int CMbimManager::GetSarState(int *sartable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	if (mService.queryCommand(0x01, nullptr, 0, &mRequestId))
	{
		LOGE << "get sar state failed" << ENDL;
		return -1;
	}
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		*sartable = -1;
		return -1;
	}

	*sartable = m_nSarIndex;
	return 0;
}

int CMbimManager::SetSarState(int sartable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = 28;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = 1;
	sarinfo->SARBackOffStatus = 1;
	sarinfo->ElementCount = 1;

	unsigned char *data = (unsigned char *)&sarinfo->data;

	MBIM_SAR_CONFIG_STATUS *status = (MBIM_SAR_CONFIG_STATUS *)data;
	status->SAROffset = 20;
	status->SARStateSize = 8;

	MBIM_SAR_CONFIG_INFO *info = (MBIM_SAR_CONFIG_INFO *)(data + 8);
	info->SARAntennaIndex = 0xffffffff;
	info->SARBAckOffIndex = sartable;

	int ret = mService.setCommand(1, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET;
	sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	unsigned char *data = (unsigned char *)&sarinfo->data;
	quec_mssar_set_sar_config_req *sar_config_req = (quec_mssar_set_sar_config_req *)data;
	sar_config_req->SetBodySarMode = nMode;		  //目前sw模式
	sar_config_req->SetBodySarProfile = nProfile; //选中哪张表
	sar_config_req->SetBodySarTech = nTech;

	for (int n = 0; n < 8; n++)
	{
		sar_config_req->SetBodySarPower[n] = sarbandpower.sarPower[n];
	}
	sar_config_req->SetBodySarBand = sarbandpower.sarBand;
	sar_config_req->SetBodySarOnTable = 1;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}
	return 0;
}

int CMbimManager::GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbandpower)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET;
	sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_resp);

	unsigned char *data = (unsigned char *)&sarinfo->data;
	quec_mssar_get_sar_config_req *sar_config_req = (quec_mssar_get_sar_config_req *)data;
	ZeroMemory(sar_config_req, sizeof(quec_mssar_get_sar_config_req));
	sar_config_req->GetBodySarMode = nMode;
	sar_config_req->GetBodySarProfile = nProfile;
	sar_config_req->GetBodySarTech = nTech;
	sar_config_req->GetBodySarBand = nBand;

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;

	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	*sarbandpower = m_sar_band_power;
	return 0;
}

int CMbimManager::SetSarClear()
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	MBIM_SAR_SET_INFO *sarinfo = nullptr;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new (std::nothrow) unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	sarinfo = (MBIM_SAR_SET_INFO *)msgBuf;
	sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET;
	sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	unsigned char *data = (unsigned char *)&sarinfo->data;
	quec_mssar_set_sar_config_req *sar_config_req = (quec_mssar_set_sar_config_req *)data;
	ZeroMemory(sar_config_req, sizeof(quec_mssar_set_sar_config_req));

	int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(ret))
		return -1;
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

void CMbimManager::update(uint8_t *data, int len)
{
	MBIM_MESSAGE_HEADER *ptr = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);

	switch (le32toh(ptr->MessageType))
	{
	case MBIM_CLOSE_DONE:
	case MBIM_OPEN_DONE:
	{
		MBIM_CLOSE_DONE_T *p = reinterpret_cast<MBIM_CLOSE_DONE_T *>(data);
		LOGD << "mbim open/close status = " << le32toh(p->Status) << ENDL;
		mReqCond.notify_one();
		break;
	}
	case MBIM_COMMAND_DONE:
	{
		MBIM_COMMAND_DONE_T *p = reinterpret_cast<MBIM_COMMAND_DONE_T *>(data);
		if (p->Status)
		{
			LOGD << "mbim command done status = " << le32toh(p->Status) << ENDL;
			mReqCond.notify_one();
			break;
		}

		MBIM_SAR_INFO *sarInfo = reinterpret_cast<MBIM_SAR_INFO *>(p->InformationBuffer);

		unsigned long sarmode = le32toh(sarInfo->SARMode);
		unsigned long SARBackOffStatus = le32toh(sarInfo->SARBackOffStatus);
		unsigned long SARWifiIntegration = le32toh(sarInfo->SARWifiIntegration);
		unsigned long element = le32toh(sarInfo->ElementCount);
		unsigned long setmode = 0x92F456E8;
		unsigned long querymode = 0x8204F6F9;

		if (sarmode == 1 || sarmode == 0)
		{
			//m_bIsMode = sarmode;
			m_bIsSarEnable = SARBackOffStatus;
			for (int n = 0; n < element; n++)
			{
				unsigned char *configInfo = (unsigned char *)&sarInfo->data;
				MBIM_SAR_CONFIG_STATUS *status = new MBIM_SAR_CONFIG_STATUS();
				memcpy(status, configInfo, (n + 1) * 8);

				MBIM_SAR_CONFIG_INFO *info = new MBIM_SAR_CONFIG_INFO();
				memcpy(info, p->InformationBuffer + status->SAROffset, status->SARStateSize);

				MBIM_SAR_CONFIG_INFO config;
				config.SARAntennaIndex = info->SARAntennaIndex;
				config.SARBAckOffIndex = info->SARBAckOffIndex;
				m_vecSarConfigInfo.push_back(config);

				m_nSarIndex = config.SARBAckOffIndex;

				delete status;
				status = NULL;
				delete info;
				info = NULL;
			}
			mReqCond.notify_one();
			break;
		}
		else if (sarmode == setmode)
		{
			unsigned char *backoffset = (unsigned char *)&sarInfo->data;
			quec_mssar_set_sar_config_resp *sarconfig = (quec_mssar_set_sar_config_resp *)backoffset;

			unsigned char bodysarmode = sarconfig->SetBodySarMode;
			unsigned char bodyprofile = sarconfig->SetBodySarProfile;
			unsigned char bodyTech = sarconfig->SetBodySarTech;
			for (int n = 0; n < 8; n++)
			{
				unsigned short bodyPower = sarconfig->SetBodySarPower[n];
			}
			unsigned char bodyBand = sarconfig->SetBodySarBand;
			unsigned char bodyOnTable = sarconfig->SetBodySarOnTable;
			// if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET)
			// {
			// 	std::unique_lock<std::mutex> _lk(m_event_lock);
			// 	m_sarmode_setevent.notify_one();
			// }
			// else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET)
			// {
			// 	std::unique_lock<std::mutex> _lk(m_event_lock);
			// 	m_sarprofile_setevent.notify_one();
			// }
			// else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET)
			// {
			// 	std::unique_lock<std::mutex> _lk(m_event_lock);
			// 	m_sartablevalue_setevent.notify_one();
			// }
			// else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET)
			// {
			// 	std::unique_lock<std::mutex> _lk(m_event_lock);
			// 	m_sartableon_setevent.notify_one();
			// }
			// else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET)
			// {
			// 	std::unique_lock<std::mutex> _lk(m_event_lock);
			// 	m_sarclear_setevent.notify_one();
			// }
		}
		else if (sarmode == querymode)
		{
			unsigned char *backoffquery = (unsigned char *)&sarInfo->data;
			quec_mssar_get_sar_config_resp *getsar = (quec_mssar_get_sar_config_resp *)backoffquery;

			unsigned char bodysarmode = getsar->GetBodySarMode;
			unsigned char bodysarprofile = getsar->GetBodySarProfile;
			unsigned char bodysartech = getsar->GetBodySarTech;
			unsigned char bodysartableon = getsar->GetBodySarOnTable;

			unsigned char bodysarontable = getsar->GetBodySarOnTable;
			if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET)
			{
				m_nSarMode = bodysarmode;
			}
			else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET)
			{
				m_nSarTable = bodysarprofile;
			}
			else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET)
			{
				for (int n = 0; n < 8; n++)
				{
					unsigned short bodysarpower = getsar->GetBodySarPower[n];
					m_sar_band_power.sarPower[n] = bodysarpower;
				}

				unsigned char bodysarband = getsar->GetBodySarBand;
				m_sar_band_power.sarTech = bodysartech;
				m_sar_band_power.sarBand = bodysarband;
			}
			else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET)
			{
				m_nSarTableOn = bodysartableon;
			}
		}
		mReqCond.notify_one();
		break;
	}
	default:
		// indication or others
		break;
	}
}