#include <string.h>

#include "config_linux.h"
#include "CMbimManager.h"
#include "MbimMessage.h"
#include "MbimToSar.h"

static const char hex_chars[] = "0123456789ABCDEF";

#define ZeroMemory(p, l) memset(p, 0, l)

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

	LOGI << __FILE__ << " " << __LINE__ << ENDL;
	if (!mService.connect(use_sock ? "mbim-proxy" : tty))
		return -1;

	LOGI << __FILE__ << " " << __LINE__ << ENDL;
	if (!mService.startPolling())
		return -1;

	LOGI << __FILE__ << " " << __LINE__ << ENDL;
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
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
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
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
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
	int nLen = sizeof(MBIM_SAR_QUEC_REQ) + sizeof(MBIM_SAR_QUEC_DATA);
	MBIM_SAR_QUEC_REQ *quecinfo = reinterpret_cast<MBIM_SAR_QUEC_REQ *>(buf);
	quecinfo->QuecMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_SET);
	quecinfo->QuecGetSetState = htole32(QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET);
	quecinfo->QuecSetCount = htole32(1);
	quecinfo->data[0].BodySarMode = nMode;

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
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
	int nLen = sizeof(MBIM_SAR_QUEC_REQ);
	MBIM_SAR_QUEC_REQ *quecinfo = reinterpret_cast<MBIM_SAR_QUEC_REQ *>(buf);
	quecinfo->QuecMode = htole32(QUEC_MBIM_MS_SAR_BACKOFF_QUERY);
	quecinfo->QuecGetSetState = htole32(QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET);
	quecinfo->QuecSetCount = htole32(0);

	if (mService.setCommand(0x01, buf, nLen, &mRequestId))
	{
		LOGE << __func__ << " send error" << ENDL;
		return -1;
	}

	mService.attach(this);
	if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
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
	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_set_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET;
	// sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	// quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&sarinfo->data;
	// ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	// getsar_config->SetBodySarMode = nMode;
	// getsar_config->SetBodySarProfile = nTable;

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }

	return 0;
}

int CMbimManager::GetSarProfile(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_get_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET;
	// sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	// quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&sarinfo->data;
	// ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));
	// getsar_config->GetBodySarMode = nMode;

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }

	// *nTable = m_nSarTable;
	return 0;
}

int CMbimManager::SetSarTableOn(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_set_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET;
	// sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	// quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&sarinfo->data;
	// ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	// getsar_config->SetBodySarMode = m_nSarMode;
	// getsar_config->SetBodySarOnTable = nTable;

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }

	return 0;
}

int CMbimManager::GetSarTableOn(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_get_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET;
	// sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	// quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&sarinfo->data;
	// ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));
	// getsar_config->GetBodySarMode = nMode;

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }
	// *nTable = m_nSarTableOn;
	return 0;
}

int CMbimManager::GetSarState(int *sartable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	// if (mService.queryCommand(0x01, nullptr, 0, &mRequestId))
	// {
	// 	LOGE << "get sar state failed" << ENDL;
	// 	return -1;
	// }

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	*sartable = -1;
	// 	return -1;
	// }

	// *sartable = m_nSarIndex;
	return 0;
}

int CMbimManager::SetSarState(int sartable)
{
	std::unique_lock<std::mutex> _lk(mReqLock);

	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = 28;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = 1;
	// sarinfo->SARBackOffStatus = 1;
	// sarinfo->ElementCount = 1;

	// uint8_t *data = (uint8_t *)&sarinfo->data;

	// MBIM_SAR_CONFIG_STATUS *status = (MBIM_SAR_CONFIG_STATUS *)data;
	// status->SAROffset = 20;
	// status->SARStateSize = 8;

	// MBIM_SAR_STATE_BACKOFF_DATA *info = (MBIM_SAR_STATE_BACKOFF_DATA *)(data + 8);
	// info->SARAntennaIndex = 0xffffffff;
	// info->SARBAckOffIndex = sartable;

	// int ret = mService.setCommand(1, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }

	return 0;
}

int CMbimManager::SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_set_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET;
	// sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	// uint8_t *data = (uint8_t *)&sarinfo->data;
	// quec_mssar_set_sar_config_req *sar_config_req = (quec_mssar_set_sar_config_req *)data;
	// sar_config_req->SetBodySarMode = nMode;		  //目前sw模式
	// sar_config_req->SetBodySarProfile = nProfile; //选中哪张表
	// sar_config_req->SetBodySarTech = nTech;

	// for (int n = 0; n < 8; n++)
	// {
	// 	sar_config_req->SetBodySarPower[n] = sarbandpower.sarPower[n];
	// }
	// sar_config_req->SetBodySarBand = sarbandpower.sarBand;
	// sar_config_req->SetBodySarOnTable = 1;

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }
	return 0;
}

int CMbimManager::GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbandpower)
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_get_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET;
	// sarinfo->ElementCount = sizeof(quec_mssar_get_sar_config_resp);

	// uint8_t *data = (uint8_t *)&sarinfo->data;
	// quec_mssar_get_sar_config_req *sar_config_req = (quec_mssar_get_sar_config_req *)data;
	// ZeroMemory(sar_config_req, sizeof(quec_mssar_get_sar_config_req));
	// sar_config_req->GetBodySarMode = nMode;
	// sar_config_req->GetBodySarProfile = nProfile;
	// sar_config_req->GetBodySarTech = nTech;
	// sar_config_req->GetBodySarBand = nBand;

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;

	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }

	// *sarbandpower = m_sar_band_power;
	return 0;
}

int CMbimManager::SetSarClear()
{
	std::unique_lock<std::mutex> _lk(mReqLock);
	// MBIM_SAR_STATE_BACKOFF_REQ *sarinfo = nullptr;

	// uint8_t *msgBuf = nullptr;
	// int nLen = sizeof(MBIM_SAR_STATE_BACKOFF_REQ) + sizeof(quec_mssar_set_sar_config_req) - 1;
	// msgBuf = new (std::nothrow) uint8_t[nLen];
	// if (msgBuf == nullptr)
	// {
	// 	return -1;
	// }

	// sarinfo = (MBIM_SAR_STATE_BACKOFF_REQ *)msgBuf;
	// sarinfo->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	// sarinfo->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET;
	// sarinfo->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	// uint8_t *data = (uint8_t *)&sarinfo->data;
	// quec_mssar_set_sar_config_req *sar_config_req = (quec_mssar_set_sar_config_req *)data;
	// ZeroMemory(sar_config_req, sizeof(quec_mssar_set_sar_config_req));

	// int ret = mService.setCommand(0x01, msgBuf, nLen, &mRequestId);
	// delete[] msgBuf;
	// msgBuf = nullptr;
	// if (FAILED(ret))
	// 	return -1;

	// mService.attach(this);
	// if (mReqCond.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	LOGE << "request id: " << mRequestId << " timeout" << ENDL;
	// 	return -1;
	// }

	return 0;
}

void CMbimManager::update(uint8_t *data, int len)
{
	MBIM_MESSAGE_HEADER *ptr = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);

	if (le32toh(ptr->MessageType) == MBIM_COMMAND_DONE)
	{
		MBIM_COMMAND_DONE_T *p = reinterpret_cast<MBIM_COMMAND_DONE_T *>(data);
		if (p->Status)
			LOGD << "mbim command done status = " << le32toh(p->Status) << ENDL;
		mReqCond.notify_one();
	}
	else if (le32toh(ptr->MessageType) == MBIM_INDICATE_STATUS_MSG)
	{
		// indication or others
	}
}