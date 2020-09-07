#include <string.h>

#include "config_linux.h"
#include "CMbimManager.h"
#include "MbimMessage.h"
#include "MbimToSar.h"

std::mutex CMbimManager::mGlobalLock;
MbimService CMbimManager::mService;

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
	: mRequestId(0),
	  mSarEnable(false)
{
}

CMbimManager::~CMbimManager()
{
}

int CMbimManager::Init(const char *tty, bool use_sock)
{
	std::lock_guard<std::mutex> _lk(mGlobalLock);
	if (mService.ready())
		return true;

	if (!mService.connect(use_sock ? "mbim-proxy" : tty))
		return -1;

	if (!mService.startPolling())
		return -1;

	mService.openCommandSession(tty);
	return mService.ready();
}

void CMbimManager::UnInit()
{
	std::lock_guard<std::mutex> _lk(mGlobalLock);
	if (!mService.ready())
		return;

	mService.closeCommandSession();
	mService.release();
}

int CMbimManager::SetSarEnable(bool bEnable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	MBIM_SAR_STATE_BACKOFF_REQ sarinfo = {
		.SARMode = htole32(1),
		.SARBackOffState = htole32(bEnable),
		.ElementCount = 0,
	};

	if (mService.setCommand(0x01, reinterpret_cast<uint8_t *>(&sarinfo),
							sizeof(MBIM_SAR_STATE_BACKOFF_REQ), &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << __func__ << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}
	LOGE << __func__ << "request id: " << mRequestId << " succussfully" << ENDL;

	return 0;
}

int CMbimManager::GetSarEnable(bool *bEnable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	LOGI << "query mbim sar enable state" << ENDL;
	if (mService.queryCommand(0x01, nullptr, 0, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		*bEnable = 0;
		return -1;
	}

	*bEnable = mSarEnable;
	LOGE << __func__ << "request id: " << mRequestId << " succussfully" << ENDL;
	LOGD << "Sar enable state: " << mSarEnable << ENDL;
	return 0;
}

int CMbimManager::SetSarMode(int nMode)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	int nLen = sizeof(MBIM_SAR_QUEC_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);
	MBIM_SAR_QUEC_REQ *quecinfo = reinterpret_cast<MBIM_SAR_QUEC_REQ *>(buf);
	quecinfo->QuecMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_SET);
	quecinfo->QuecGetSetState = htole32(QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET);
	quecinfo->QuecSetCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *quecdata = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(quecinfo->data);
	quecdata->BodySarMode = htole32(nMode);

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		return -1;
	}
	LOGE << __func__ << "request id: " << mRequestId << " succussfully" << ENDL;

	return 0;
}

int CMbimManager::GetSarMode(int *nMode)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_QUEC_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);

	MBIM_SAR_QUEC_REQ *quecinfo = reinterpret_cast<MBIM_SAR_QUEC_REQ *>(buf);
	quecinfo->QuecMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_QUERY);
	quecinfo->QuecGetSetState = htole32(QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET);
	quecinfo->QuecSetCount = htole32(1);

	if (mService.queryCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	*nMode = mSarMode;
	LOGE << __func__ << "request id: " << mRequestId << " succussfully" << ENDL;
	LOGD << "Sar enable state: " << mSarMode << ENDL;
	return 0;
}

int CMbimManager::SetSarProfile(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_STATE_BACKOFF_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_SET);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *getsar_config = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarinfo->data);
	getsar_config->BodySarMode = htole32(nMode);
	getsar_config->BodySarProfile = htole32(nTable);

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::GetSarProfile(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_QUERY);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *getsar_config = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarinfo->data);
	getsar_config->BodySarMode = htole32(nMode);

	if (mService.queryCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	*nTable = mSarTable;
	return 0;
}

int CMbimManager::SetSarTableOn(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_SET);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *getsar_config = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarinfo->data);
	getsar_config->BodySarMode = htole32(nMode);
	getsar_config->BodySarOnTable = htole32(nTable);

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::GetSarTableOn(int nMode, int *nTableOn)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_QUERY);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *getsar_config = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarinfo->data);
	getsar_config->BodySarMode = htole32(nMode);

	if (mService.queryCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}
	*nTableOn = mSarTableOn;
	return 0;
}

int CMbimManager::GetSarState(int *sarindex)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	if (mService.queryCommand(0x01, nullptr, 0, &mRequestId))
	{
		LOGE << "get sar state failed" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		*sarindex = -1;
		return -1;
	}

	*sarindex = mSarIndex;
	return 0;
}

int CMbimManager::SetSarState(int sartable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_CONFIG_STATUS) +
			   sizeof(MBIM_SAR_STATE_BACKOFF_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);

	sarinfo->SARMode = htole32(1);
	sarinfo->SARBackOffState = htole32(1);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_CONFIG_STATUS *status = reinterpret_cast<MBIM_SAR_CONFIG_STATUS *>(sarinfo->data);
	status->SAROffset = htole32(20);
	status->SARStateSize = htole32(8);

	MBIM_SAR_STATE_BACKOFF_DATA *info = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_DATA *>(sarinfo->data + sizeof(MBIM_SAR_CONFIG_STATUS));
	info->SARAntennaIndex = htole32(0xffffffff);
	info->SARBAckOffIndex = htole32(sartable);

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

int CMbimManager::SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_SET);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *sar_config_req = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarinfo->data);
	sar_config_req->BodySarMode = nMode;	   //目前sw模式
	sar_config_req->BodySarProfile = nProfile; //选中哪张表
	sar_config_req->BodySarTech1 = nTech;

	for (int n = 0; n < 8; n++)
		sar_config_req->BodySarPower[n] = sarbandpower.sarPower[n];
	sar_config_req->BodySarBand = sarbandpower.sarBand;
	sar_config_req->BodySarOnTable = 1;

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}
	return 0;
}

int CMbimManager::GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbandpower)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_QUERY);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET);
	sarinfo->ElementCount = htole32(1);

	MBIM_SAR_QUEC_CONFIG_DATA *sar_config_req = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarinfo->data);
	sar_config_req->BodySarMode = nMode;
	sar_config_req->BodySarProfile = nProfile;
	sar_config_req->BodySarTech1 = nTech;
	sar_config_req->BodySarBand = nBand;

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	if (sarbandpower)
		memcpy(sarbandpower, &mSarBandPower, sizeof(MBIM_SAR_BAND_POWER));
	return 0;
}

int CMbimManager::SetSarClear()
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	uint8_t buf[1024];
	memset(buf, 0, sizeof(buf));
	int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(MBIM_SAR_QUEC_CONFIG_DATA);

	MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_REQ *>(buf);
	sarinfo->SARMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_SET);
	sarinfo->SARBackOffState = htole32(QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET);
	sarinfo->ElementCount = htole32(1);

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::seconds(10)) == std::cv_status::timeout)
	{
		LOGE << "request id: " << mRequestId << " timeout" << ENDL;
		return -1;
	}

	return 0;
}

void CMbimManager::update(uint8_t *data, int len)
{
	MBIM_MESSAGE_HEADER *ptr = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);

	if (le32toh(ptr->MessageType) == MBIM_INDICATE_STATUS_MSG)
	{
		// indication or others
	}
	else if (le32toh(ptr->MessageType) == MBIM_COMMAND_DONE)
	{
		MBIM_COMMAND_DONE_T *p = reinterpret_cast<MBIM_COMMAND_DONE_T *>(data);
		if (p->Status)
			LOGD << "mbim command done status = " << le32toh(p->Status) << ENDL;

		MBIM_SAR_STATE_BACKOFF_RESP *sarInfo = reinterpret_cast<MBIM_SAR_STATE_BACKOFF_RESP *>(p->InformationBuffer);
		uint32_t sarmode = le32toh(sarInfo->SARMode);
		uint32_t SARBackOffStatus = le32toh(sarInfo->SARBackOffStatus);
		// uint32_t SARWifiIntegration = le32toh(sarInfo->SARWifiIntegration);
		uint32_t element = le32toh(sarInfo->ElementCount);
		if (sarmode == 1 || sarmode == 0)
		{
			mSarEnable = SARBackOffStatus;
			for (uint32_t n = 0; n < element; n++)
			{
				MBIM_SAR_CONFIG_STATUS *status =
					reinterpret_cast<MBIM_SAR_CONFIG_STATUS *>(sarInfo->data + n * sizeof(MBIM_SAR_CONFIG_STATUS));

				MBIM_SAR_STATE_BACKOFF_DATA *info =
					reinterpret_cast<MBIM_SAR_STATE_BACKOFF_DATA *>(sarInfo->data + le32toh(status->SAROffset));

				MBIM_SAR_STATE_BACKOFF_DATA config;
				config.SARAntennaIndex = info->SARAntennaIndex;
				config.SARBAckOffIndex = info->SARBAckOffIndex;
				mVecSarConfigInfo.emplace_back(config);
				mSarIndex = config.SARBAckOffIndex;
			}
			mReqCond.notify_one();
		}
		else if (le32toh(sarmode) == QUEC_MBIM_MS_SAR_BACKOFF_SET)
		{
			// SET command, skip
			mReqCond.notify_one();
		}
		else if (le32toh(sarmode) == QUEC_MBIM_MS_SAR_BACKOFF_QUERY)
		{
			MBIM_SAR_QUEC_CONFIG_DATA *getsar = reinterpret_cast<MBIM_SAR_QUEC_CONFIG_DATA *>(sarInfo->data);
			uint8_t bodysarmode = getsar->BodySarMode;
			uint8_t bodysarprofile = getsar->BodySarProfile;
			uint8_t bodysartech = getsar->BodySarTech1;
			uint8_t bodysartableon = getsar->BodySarOnTable;
			if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET)
				mSarMode = bodysarmode;
			else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET)
				mSarTable = bodysarprofile;
			else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET)
			{
				mSarBandPower.sarTech = bodysartech;
				mSarBandPower.sarBand = getsar->BodySarBand;
				for (int n = 0; n < 8; n++)
					mSarBandPower.sarPower[n] = getsar->BodySarPower[n];
			}
			else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET)
				mSarTableOn = bodysartableon;
			mReqCond.notify_one();
		}
	}
}
