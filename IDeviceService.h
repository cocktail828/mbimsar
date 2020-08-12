#ifndef _IDEVICESERVICEMANAGER_
#define _IDEVICESERVICEMANAGER_
#include <string>

#include <stdint.h>

class IDeviceService
{
    uint32_t requestid;
    std::string device;
    int fd;

public:
    IDeviceService(const char *d);

    void Release();
    int Connect();
    int SetCommand(int cid, void *, int, uint32_t *);
    int QueryCommand(int cid, void *, int, uint32_t *);
    int OpenCommandSession(uint32_t *);
    int CloseCommandSession(uint32_t *);
    int SetDeviceServiceList();
};
#endif //_IDEVICESERVICEMANAGER
