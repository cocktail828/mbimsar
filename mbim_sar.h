#ifndef __MBIM_SAR
#define __MBIM_SAR

#include <iostream>
#include <vector>
#include <mutex>

#include "config/config_linux.h"
using namespace std;
/*
all sink send notification to observer.
observer invoke update
*/
enum notification_type
{
	Sink_ConnectionContext = 0,
	Sink_Connection,
	Sink_ConnectionManager,
	Sink_ConnectionProfile,
	Sink_Interface,
	Sink_InterfaceManager,
	Sink_Pin,
	Sink_PinManager,
	Sink_Radio,
	Sink_Registration,
	Sink_ServiceActivation,
	Sink_Signal,
	Sink_Sms,
	Sink_MBIM,
	Sink_Max
};

/*	CConnectionEventsSink */
enum ConnectionEventType
{
	ConnectionComplete = 0,
	DisconnectionComplete,
	ConnectStateChange,
	VoiceCallsStateChange,
};

typedef struct tagConnectionEventPara
{
	enum ConnectionEventType type_;
} ConnectionEventPara;
/*	CConnectionEventsSink */

/*	CConnectionManagerEventsSink */
enum ConnectionManagerEventType
{
	ConnectionArrival = 0,
	ConnectionRemoval
};
typedef struct tagConnectionManagerEventPara
{
	enum ConnectionManagerEventType type_;
} ConnectionManagerEventPara;
/*	CConnectionManagerEventsSink */

/*	CInterfaceEventsSink */
enum InterfaceEventType
{
	InterfaceCapabilityAvailable = 0,
	SubscriberInformationChange,
	ReadyStateChange,
	EmergencyModeChange,
	HomeProviderAvailable,
	PreferredProvidersChange,
	SetPreferredProvidersComplete,
	ScanNetworkComplete
};
typedef struct tagInterfaceEventPara
{
	enum InterfaceEventType type_;
} InterfaceEventPara;
/*	CInterfaceEventsSink */

/*	CInterfaceManagerEventsSink */
enum InterfaceManagerEventType
{
	InterfaceArrival = 0,
	InterfaceRemoval
};
typedef struct tagInterfaceManagerEventPara
{
	enum InterfaceManagerEventType type_;
} InterfaceManagerEventPara;
/*	CInterfaceManagerEventsSink */

/*	CPinEventsSink */
enum PinEventEventType
{
	EnableComplete = 0,
	DisableComplete,
	EnterComplete,
	ChangeComplete,
	UnblockComplete
};
typedef struct tagPinEventEventPara
{
	enum PinEventEventType type_;
} PinEventEventPara;
/*	CPinEventsSink */

/*	CRadioEventsSink */
enum RadioEventType
{
	RadioStateChange = 0,
	SetSoftwareRadioStateComplete
};
typedef struct tagRadioEventPara
{
	enum RadioEventType type_;
} RadioEventPara;
/*	CRadioEventsSink */

/*	CRegistrationEventsSink */
enum RegistrationEventType
{
	RegisterModeAvailable = 0,
	RegisterStateChange,
	PacketServiceStateChange,
	SetRegisterModeComplete
};
typedef struct tagRegistrationEventPara
{
	enum RegistrationEventType type_;
} RegistrationEventPara;
/*	CRegistrationEventsSink */

/*	CSignalEventsSink */
enum SignalEventType
{
	SignalStateChange = 0,
};
typedef struct tagSignalEventPara
{
	enum SignalEventType type_;
} SignalEventPara;
/*	CSignalEventsSink */

/*	CSmsEventsSink */
enum SmsEventType
{
	SmsConfigurationChange = 0,
	SetSmsConfigurationComplete,
	SmsSendComplete,
	SmsReadComplete,
	SmsNewClass0Message,
	SmsDeleteComplete,
	SmsStatusChange
};
typedef struct tagSmsEventPara
{
	enum SmsEventType type_;
} SmsEventPara;
/*	CSmsEventsSink */
/*CPinManagerEventsSink*/
enum PinManagerEventType
{
	PinListAvailable = 0,
	PinStateComplete
};

typedef struct tagPinManagerEventPara
{
	enum PinManagerEventType type_;
} PinManagerEventPara;
/*CPinManagerEventsSink*/

enum ServiceStatusType
{
	ServiceOpenCmd = 0,
	ServiceCloseCmd,
	ServiceSet,
	ServiceQuery,
	ServiceEvent,
	ServiceOpenData,
	ServiceCloseData,
	ServiceWriteData,
	ServiceReadData,
	ServiceState,
	ServiceMax
};

typedef struct
{
	enum notification_type _type;
	enum ServiceStatusType _servicestatus;
	void *mbn_interface_ptr;
	std::string strinterfaceid;
	int _servicestate;
	union {
		ConnectionEventPara connectioneventpara_;
		ConnectionManagerEventPara connectionmanagereventpara_;
		InterfaceEventPara interfaceeventpara_;
		InterfaceManagerEventPara interfacemanagereventpara_;
		PinEventEventPara pineventpara_;
		PinManagerEventPara pinmanagerpara_;
		RadioEventPara radioeventpara_;
		RegistrationEventPara registrationeventpara_;
		SignalEventPara signaleventpara_;
		SmsEventPara smseventpara_;
	} Para;

	// MBN_PIN_INFO *mbn_pin_info;
	// MBN_SMS_FORMAT mbn_sms_format;
	u8 safeArray[0];
	// VARIANT_bool moreMsgs;
	u32 requestID;
	u32 eventID;
	HRESULT status;
} MbnNotify;

#endif //__MBIM_SAR
