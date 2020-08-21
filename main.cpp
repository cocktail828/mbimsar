#include <iostream>
#include <mutex>
#include <condition_variable>

#include "MbimToSar.h"
#include "logger.h"

int main(int argc, char **argv)
{
    Logger::Init("LOGGER", Severity::DEBUG);

    LOGI << __FILE__ << " " << __LINE__ << ENDL;
    MBIMTOSAR_Init(argv[1], 1);
    LOGI << __FILE__ << " " << __LINE__ << ENDL;

    // bool en;
    // MBIMTOSAR_SetSarEnable(true);
    // MBIMTOSAR_GetSarEnable(&en);
    // LOGI << "sar state " << en << ENDL;
    // MBIMTOSAR_SetSarEnable(false);
    // MBIMTOSAR_GetSarEnable(&en);
    // LOGI << "sar state " << en << ENDL;

    int mode;
    MBIMTOSAR_SetSarMode(true);
    MBIMTOSAR_GetSarMode(&mode);
    LOGI << "sar mode " << mode << ENDL;
    MBIMTOSAR_SetSarMode(false);
    MBIMTOSAR_GetSarMode(&mode);
    LOGI << "sar mode " << mode << ENDL;

    // MBIMTOSAR_SetSarProfile(int nMode, int nTable);
    // MBIMTOSAR_GetSarProfile(int nMode, int *nTable);

    // MBIMTOSAR_SetSarTableOn(int nMode, int nTable);
    // MBIMTOSAR_GetSarTableOn(int nMode, int *nTable);

    // MBIMTOSAR_SetSarState(int sartable);
    // MBIMTOSAR_GetSarState(int *sartable);

    // MBIMTOSAR_SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower);
    // MBIMTOSAR_GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbanpower);

    // MBIMTOSAR_SetSarClear();

    MBIMTOSAR_UnInit();

    return 0;
}