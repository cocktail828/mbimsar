#include <iostream>

#include "MbimToSar.h"
#include "CMbimManager.h"

int MBIMTOSAR_Init(const char *d)
{
	return MBIMMANAGER::instance().Init(d);
}

void MBIMTOSAR_UnInit(void)
{
	MBIMMANAGER::instance().UnInit();
}

int MBIMTOSAR_OpenDeviceServices(const char *interfaceid)
{
	return MBIMMANAGER::instance().OpenDeviceServices(interfaceid);
}

int MBIMTOSAR_CloseDeviceServices()
{
	return MBIMMANAGER::instance().CloseDeviceServices();
}

int MBIMTOSAR_GetIsMbimReady(bool *bEnable)
{
	return MBIMMANAGER::instance().GetIsMbimReady(bEnable);
}

int MBIMTOSAR_SetSarEnable(bool bEnable)
{
	return MBIMMANAGER::instance().SetSarEnable(bEnable);
}

int MBIMTOSAR_GetSarEnable(bool *bEnable)
{
	return MBIMMANAGER::instance().GetSarEnable(bEnable);
}

int MBIMTOSAR_SetSarMode(int nMode)
{
	return MBIMMANAGER::instance().SetSarMode(nMode);
}

int MBIMTOSAR_GetSarMode(int *nMode)
{
	return MBIMMANAGER::instance().GetSarMode(nMode);
}

int MBIMTOSAR_SetSarProfile(int nMode, int nTable)
{
	return MBIMMANAGER::instance().SetSarProfile(nMode, nTable);
}

int MBIMTOSAR_GetSarProfile(int nMode, int *nTable)
{
	return MBIMMANAGER::instance().GetSarProfile(nMode, nTable);
}

int MBIMTOSAR_SetSarTableOn(int nMode, int nTable)
{
	return MBIMMANAGER::instance().SetSarTableOn(nMode, nTable);
}

int MBIMTOSAR_GetSarTableOn(int nMode, int *nTable)
{
	return MBIMMANAGER::instance().GetSarTableOn(nMode, nTable);
}

int MBIMTOSAR_SetSarState(int sartable)
{
	return MBIMMANAGER::instance().SetSarState(sartable);
}

int MBIMTOSAR_GetSarState(int *sartable)
{
	return MBIMMANAGER::instance().GetSarState(sartable);
}

int MBIMTOSAR_SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower)
{
	return MBIMMANAGER::instance().SetSarValue(nMode, nProfile, nTech, sarbandpower);
}

int MBIMTOSAR_GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbanpower)
{
	return MBIMMANAGER::instance().GetSarValue(nMode, nProfile, nTech, nBand, sarbanpower);
}

int MBIMTOSAR_SetSarClear()
{
	return MBIMMANAGER::instance().SetSarClear();
}
