#include <string.h>

#include "CQuectelMbimManager.h"
// #include "quectel_mbim_sink.h"
// #include "quectel_register_sink.h"
#include "MbimToSar.h"
#include "quectel_string_help.h"

static const char hex_chars[] = "0123456789ABCDEF";

#define ZeroMemory(p, l) memset(p, 0, l)
string convert_hex(unsigned char *md /*字符串*/, int nLen /*转义多少个字符*/)
{
	string strSha1((""));
	unsigned int c = 0;

	// 查看unsigned char占几个字节
	// 实际占1个字节，8位
	int nByte = sizeof(unsigned char);

	for (int i = 0; i < nLen; i++)
	{
		// 查看md一个字节里的信息
		unsigned int x = 0;
		x = md[i];
		x = md[i] >> 4; // 右移，干掉4位，左边高位补0000

		c = (md[i] >> 4) & 0x0f;
		strSha1 += hex_chars[c];
		strSha1 += hex_chars[md[i] & 0x0f];
	}
	return strSha1;
}

SINGLETON_IMPLEMENT(CQuectelMbimManager)

CQuectelMbimManager::CQuectelMbimManager()
{
	m_init_flag = false;
	m_deviceservice = nullptr;
	m_nDeviceId = -1;
	m_nSarTable = -1;
	m_vecSarConfigInfo.clear();
	m_bIsReady = false;

	m_bIsSarEnable = false;
	m_bIsMode = false;
	m_nSarIndex = -1;
	m_nSarMode = -1;
	m_nSarTech = -1;
	m_nSarTableOn = -1;
}

CQuectelMbimManager::~CQuectelMbimManager()
{
	if (m_deviceservice)
		delete m_deviceservice;
}

int CQuectelMbimManager::Init(const char *d)
{
	std::lock_guard<std::mutex> _lk(m_op_lock);
	if (m_init_flag)
		return true;

	HRESULT hr = E_FAIL;
	m_deviceservice = new IDeviceService(d);
	if (!m_deviceservice)
		return -1;

	if (m_deviceservice->Connect())
		return -1;

	m_init_flag = true;
	return 0;
}

void CQuectelMbimManager::UnInit()
{
	std::lock_guard<std::mutex> _lk(m_op_lock);
	if (!m_init_flag)
		return;

	//资源释放
	if (m_deviceservice)
		m_deviceservice->Release();

	m_bIsReady = false;
}

int CQuectelMbimManager::OpenDeviceServices(std::string strInterfaceId)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	if (!m_init_flag)
		return -1;

	HRESULT hr = E_FAIL;
	
	m_deviceservice->SetDeviceServiceList();
	// BSTR qbi_uuid = _com_util::ConvertStringToBSTR("{68223D04-9F6C-4E0F-822D-28441FB72340}");
	// hr = mbn_deviceservicescontext->GetDeviceService(qbi_uuid, &m_deviceservice);
	// if (FAILED(hr))
	// 	return -1;
	// std::pair<IConnectionPoint *, DWORD> result_;
	// CDeviceServicesSink *sinkEvent = new CDeviceServicesSink();
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
	// u32 requestId;
	// hr = m_deviceservice->OpenCommandSession(&requestId);
	// if (FAILED(hr))
	// 	return -1;
	// if (m_waitevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	// {
	// 	delete sinkEvent;
	// 	sinkEvent = nullptr;

	// 	return -1;
	// }

	m_bIsReady = true;

	return 0;
}

int CQuectelMbimManager::CloseDeviceServices()
{
	std::lock_guard<std::mutex> _lk(m_op_lock);
	if (!m_init_flag)
		return -1;

	if (m_deviceservice)
	{
		u32 requestId;
		m_deviceservice->CloseCommandSession(&requestId);
	}
	m_bIsReady = false;
	return 0;
}

int CQuectelMbimManager::GetIsMbimReady(bool *bEnable)
{
	*bEnable = m_bIsReady;

	return 0;
}

int CQuectelMbimManager::SetSarEnable(bool bEnable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);

	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	msgBuf = new unsigned char[12];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = 1;
	qmi->SARBackOffStatus = bEnable;
	qmi->ElementCount = 0;

	//打印数据
	string strHex = convert_hex(msgBuf, 12);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, 12, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_waitevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

int CQuectelMbimManager::GetSarEnable(bool *bEnable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);

	//查询sar配置
	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->QueryCommand(0x01, nullptr, 0, &ulRequestID);
	if (m_waitevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		*bEnable = 0;
		return -1;
	}

	*bEnable = m_bIsSarEnable;
	return 0;
}

int CQuectelMbimManager::SetSarMode(int nMode)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET;
	qmi->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&qmi->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	getsar_config->SetBodySarMode = nMode;

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sarmode_setevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

int CQuectelMbimManager::GetSarMode(int *nMode)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET;
	qmi->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&qmi->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sarmode_getevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}
	*nMode = m_nSarMode;
	return 0;
}

int CQuectelMbimManager::SetSarProfile(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET;
	qmi->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&qmi->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	getsar_config->SetBodySarMode = nMode;
	getsar_config->SetBodySarProfile = nTable;

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sarprofile_setevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

int CQuectelMbimManager::GetSarProfile(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET;
	qmi->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&qmi->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));
	getsar_config->GetBodySarMode = nMode;

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sarprofile_getevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	*nTable = m_nSarTable;
	return 0;
}

int CQuectelMbimManager::SetSarTableOn(int nMode, int nTable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET;
	qmi->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	quec_mssar_set_sar_config_req *getsar_config = (quec_mssar_set_sar_config_req *)&qmi->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_set_sar_config_req));
	getsar_config->SetBodySarMode = m_nSarMode;
	getsar_config->SetBodySarOnTable = nTable;

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sartableon_setevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

int CQuectelMbimManager::GetSarTableOn(int nMode, int *nTable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);

	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET;
	qmi->ElementCount = sizeof(quec_mssar_get_sar_config_req);

	quec_mssar_get_sar_config_req *getsar_config = (quec_mssar_get_sar_config_req *)&qmi->data;
	ZeroMemory(getsar_config, sizeof(quec_mssar_get_sar_config_req));
	getsar_config->GetBodySarMode = nMode;

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sartableon_getevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}
	*nTable = m_nSarTableOn;
	return 0;
}

int CQuectelMbimManager::GetSarState(int *sartable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);

	//查询sar配置
	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->QueryCommand(0x01, nullptr, 0, &ulRequestID);
	if (m_waitevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		*sartable = -1;
		return -1;
	}

	*sartable = m_nSarIndex;
	return 0;
}

int CQuectelMbimManager::SetSarState(int sartable)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);

	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = 28;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = 1;
	qmi->SARBackOffStatus = 1;
	qmi->ElementCount = 1;

	unsigned char *data = (unsigned char *)&qmi->data;

	MBIM_SAR_CONFIG_STATUS *status = (MBIM_SAR_CONFIG_STATUS *)data;
	status->SAROffset = 20;
	status->SARStateSize = 8;

	MBIM_SAR_CONFIG_INFO *info = (MBIM_SAR_CONFIG_INFO *)(data + 8);
	info->SARAntennaIndex = 0xffffffff;
	info->SARBAckOffIndex = sartable;

	string strHex = convert_hex(msgBuf, 28);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(1, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_waitevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

int CQuectelMbimManager::SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET;
	qmi->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	unsigned char *data = (unsigned char *)&qmi->data;
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

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sartablevalue_setevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}
	return 0;
}

int CQuectelMbimManager::GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbandpower)
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_get_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_QUERY;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET;
	qmi->ElementCount = sizeof(quec_mssar_get_sar_config_resp);

	unsigned char *data = (unsigned char *)&qmi->data;
	quec_mssar_get_sar_config_req *sar_config_req = (quec_mssar_get_sar_config_req *)data;
	ZeroMemory(sar_config_req, sizeof(quec_mssar_get_sar_config_req));
	sar_config_req->GetBodySarMode = nMode;
	sar_config_req->GetBodySarProfile = nProfile;
	sar_config_req->GetBodySarTech = nTech;
	sar_config_req->GetBodySarBand = nBand;

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;

	if (FAILED(hr))
		return -1;
	if (m_sartablevalue_getevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	*sarbandpower = m_sar_band_power;
	return 0;
}

int CQuectelMbimManager::SetSarClear()
{
	std::unique_lock<std::mutex> _lk(m_event_lock);
	MBIM_SAR_SET_INFO *qmi;

	unsigned char *msgBuf = nullptr;
	int nLen = sizeof(MBIM_SAR_SET_INFO) + sizeof(quec_mssar_set_sar_config_req) - 1;
	msgBuf = new unsigned char[nLen];
	if (msgBuf == nullptr)
	{
		return -1;
	}

	qmi = (MBIM_SAR_SET_INFO *)msgBuf;
	qmi->SARMode = QUEC_MBIM_MS_SAR_BACKOFF_SET;
	qmi->SARBackOffStatus = QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET;
	qmi->ElementCount = sizeof(quec_mssar_set_sar_config_req);

	unsigned char *data = (unsigned char *)&qmi->data;
	quec_mssar_set_sar_config_req *sar_config_req = (quec_mssar_set_sar_config_req *)data;
	ZeroMemory(sar_config_req, sizeof(quec_mssar_set_sar_config_req));

	string strHex = convert_hex(msgBuf, nLen);
	printf("\n");
	printf(strHex.c_str());
	printf("\n");

	u32 ulRequestID = 0;
	HRESULT hr = m_deviceservice->SetCommand(0x01, msgBuf, nLen, &ulRequestID);
	delete[] msgBuf;
	msgBuf = nullptr;
	if (FAILED(hr))
		return -1;
	if (m_sarclear_setevent.wait_for(_lk, std::chrono::milliseconds(500)) == std::cv_status::timeout)
	{
		return -1;
	}

	return 0;
}

IDeviceService *CQuectelMbimManager::getdeviceservice()
{
	return m_deviceservice;
}

void CQuectelMbimManager::OpenCmdSessionCb()
{
	m_waitevent.notify_one();
}

void CQuectelMbimManager::update(MbnSubject *subject, void *notify)
{
	// MbnNotify *notification_ = (MbnNotify *)notify;
	// enum notification_type type_ = notification_->_type;
	// enum ServiceStatusType status_ = notification_->_servicestatus;
	// switch (type_)
	// {
	// case Sink_MBIM:
	// {
	// 	m_vecSarConfigInfo.clear();
	// 	if (status_ == ServiceOpenCmd)
	// 	{
	// 		std::unique_lock<std::mutex> _lk(m_event_lock);
	// 		m_waitevent.notify_one();
	// 	}
	// 	else if (status_ == ServiceQuery || status_ == ServiceSet)
	// 	{
	// 		MbnNotify *mbn_notify = (MbnNotify *)notify;
	// 		if (mbn_notify->status == S_OK)
	// 		{
	// 			long Lower, Upper;
	// 			HRESULT hr;
	// 			do
	// 			{
	// 				hr = SafeArrayGetLBound(mbn_notify->safeArray, 1, &Lower);
	// 				if (FAILED(hr))
	// 					break;
	// 				hr = SafeArrayGetLBound(mbn_notify->safeArray, 1, &Upper);
	// 				if (FAILED(hr))
	// 					break;
	// 				for (long idx = Lower; idx <= Upper; ++idx)
	// 				{
	// 					unsigned char *pBuf;
	// 					SafeArrayAccessData(mbn_notify->safeArray, (void **)&pBuf);
	// 					int nLen = mbn_notify->safeArray->rgsabound->cElements;
	// 					//打印数据
	// 					string strHex = convert_hex(pBuf, nLen);
	// 					printf("\n RECV:");
	// 					printf(strHex.c_str());
	// 					printf("\n");
	// 					//数据接收处理
	// 					if (nLen > 16)
	// 					{
	// 						MBIM_SAR_INFO *sarInfo = (MBIM_SAR_INFO *)pBuf;
	// 						unsigned long sarmode = sarInfo->SARMode;
	// 						unsigned long SARBackOffStatus = sarInfo->SARBackOffStatus;
	// 						unsigned long SARWifiIntegration = sarInfo->SARWifiIntegration;
	// 						unsigned long element = sarInfo->ElementCount;

	// 						unsigned long setmode = 0x92F456E8;
	// 						unsigned long querymode = 0x8204F6F9;
	// 						if (sarmode == 1 || sarmode == 0)
	// 						{
	// 							//m_bIsMode = sarmode;
	// 							m_bIsSarEnable = SARBackOffStatus;
	// 							for (int n = 0; n < element; n++)
	// 							{
	// 								unsigned char *configInfo = (unsigned char *)&sarInfo->data;
	// 								MBIM_SAR_CONFIG_STATUS *status = new MBIM_SAR_CONFIG_STATUS();
	// 								memcpy(status, configInfo, (n + 1) * 8);

	// 								MBIM_SAR_CONFIG_INFO *info = new MBIM_SAR_CONFIG_INFO();
	// 								memcpy(info, pBuf + status->SAROffset, status->SARStateSize);

	// 								MBIM_SAR_CONFIG_INFO config;
	// 								config.SARAntennaIndex = info->SARAntennaIndex;
	// 								config.SARBAckOffIndex = info->SARBAckOffIndex;
	// 								m_vecSarConfigInfo.push_back(config);

	// 								m_nSarIndex = config.SARBAckOffIndex;

	// 								delete status;
	// 								status = nullptr;
	// 								delete info;
	// 								info = nullptr;
	// 							}
	// 							std::unique_lock<std::mutex> _lk(m_event_lock);
	// 							m_waitevent.notify_one();
	// 						}
	// 						else if (sarmode == setmode)
	// 						{
	// 							unsigned char *backoffset = (unsigned char *)&sarInfo->data;
	// 							quec_mssar_set_sar_config_resp *sarconfig = (quec_mssar_set_sar_config_resp *)backoffset;

	// 							unsigned char bodysarmode = sarconfig->SetBodySarMode;
	// 							unsigned char bodyprofile = sarconfig->SetBodySarProfile;
	// 							unsigned char bodyTech = sarconfig->SetBodySarTech;
	// 							for (int n = 0; n < 8; n++)
	// 							{
	// 								unsigned short bodyPower = sarconfig->SetBodySarPower[n];
	// 							}
	// 							unsigned char bodyBand = sarconfig->SetBodySarBand;
	// 							unsigned char bodyOnTable = sarconfig->SetBodySarOnTable;
	// 							if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET)
	// 							{
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sarmode_setevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET)
	// 							{
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sarprofile_setevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET)
	// 							{
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sartablevalue_setevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET)
	// 							{
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sartableon_setevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET)
	// 							{
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sarclear_setevent.notify_one();
	// 							}
	// 						}
	// 						else if (sarmode == querymode)
	// 						{
	// 							unsigned char *backoffquery = (unsigned char *)&sarInfo->data;
	// 							quec_mssar_get_sar_config_resp *getsar = (quec_mssar_get_sar_config_resp *)backoffquery;

	// 							unsigned char bodysarmode = getsar->GetBodySarMode;
	// 							unsigned char bodysarprofile = getsar->GetBodySarProfile;
	// 							unsigned char bodysartech = getsar->GetBodySarTech;
	// 							unsigned char bodysartableon = getsar->GetBodySarOnTable;

	// 							unsigned char bodysarontable = getsar->GetBodySarOnTable;
	// 							if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET)
	// 							{
	// 								m_nSarMode = bodysarmode;
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sarmode_getevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET)
	// 							{
	// 								m_nSarTable = bodysarprofile;
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sarprofile_getevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET)
	// 							{
	// 								for (int n = 0; n < 8; n++)
	// 								{
	// 									unsigned short bodysarpower = getsar->GetBodySarPower[n];
	// 									m_sar_band_power.sarPower[n] = bodysarpower;
	// 								}

	// 								unsigned char bodysarband = getsar->GetBodySarBand;
	// 								m_sar_band_power.sarTech = bodysartech;
	// 								m_sar_band_power.sarBand = bodysarband;

	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sartablevalue_getevent.notify_one();
	// 							}
	// 							else if (SARBackOffStatus == QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET)
	// 							{
	// 								m_nSarTableOn = bodysartableon;
	// 								std::unique_lock<std::mutex> _lk(m_event_lock);
	// 								m_sartableon_getevent.notify_one();
	// 							}
	// 						}
	// 					}
	// 					SafeArrayUnaccessData(mbn_notify->safeArray);
	// 				}
	// 			} while (0);
	// 		}
	// 		else
	// 		{
	// 			OutputDebugString("ERROR");
	// 			printf("\n");
	// 			printf("ERROR");
	// 			printf("\n");
	// 		}
	// 	}

	// 	break;
	// }
	// default:
	// 	break;
	// }
}
