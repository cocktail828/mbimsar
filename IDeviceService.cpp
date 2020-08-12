#include "IDeviceService.h"
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <fcntl.h>
#include <string.h>

IDeviceService::IDeviceService(const char *d)
{
    requestid = 0;
    device = d;
    fd = 0;
}

void IDeviceService::Release()
{
    close(fd);
}

int unix_sock(const char *n)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    struct sockaddr_un addr;
    socklen_t socklen;
    memset(&addr, 0, sizeof(sockaddr));
    addr.sun_family = AF_LOCAL;
    memcpy((char *)addr.sun_path + 1, n, strlen(n));
    socklen = offsetof(sockaddr_un, sun_family) + strlen(n) + 1;

    //    setsockopt(sockfd, SO_REUSEADDR,);
    connect(sockfd, (sockaddr *)&addr, socklen);
    return sockfd;
}

int IDeviceService::Connect()
{
    if (device.find("/dev") != std::string::npos)
        fd = open(device.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);
    else
        fd = unix_sock(device.c_str());
    return fd > 0;
}

int IDeviceService::SetCommand(int cid, void *, int, uint32_t *)
{
}

int IDeviceService::QueryCommand(int cid, void *, int, uint32_t *)
{
}

int IDeviceService::OpenCommandSession(uint32_t *)
{
}

int IDeviceService::CloseCommandSession(uint32_t *)
{
}
