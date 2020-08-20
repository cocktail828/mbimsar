#ifndef __MBIMSERVICE
#define __MBIMSERVICE

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include <stdint.h>

#include "ISubjectObserver.h"

class NonCopyable
{
private:
    NonCopyable(const NonCopyable &);
    NonCopyable &operator=(const NonCopyable &);

public:
    NonCopyable(){};
    ~NonCopyable(){};
};

/** 
 * MbimService should not be copyed
 */
class MbimService final : public NonCopyable, public ISubject
{
private:
    const char *proxy_name = "mbim-proxy";
    /* lock for send/recv */
    static std::mutex mRWLock;

private:
    /* lock for do commands */
    std::string mDevice;
    int mPollingHandle;
    bool mQuitFlag;
    bool mReady;

private:
    MbimService(const MbimService &) = delete;

    MbimService &operator=(const MbimService &) = delete;

public:
    MbimService()
        : mDevice(""),
          mPollingHandle(0),
          mQuitFlag(false),
          mReady(false) {}
    ~MbimService() {}

    static MbimService &Instance();
    bool ready();
    void release();
    bool connect(const char *d = nullptr);
    int sendAsync(uint8_t *, int);
    int recvAsync(uint8_t *, int *);
    int setCommand(int cid, uint8_t *data = nullptr, int len = 0, uint32_t *rid = nullptr);
    int queryCommand(int cid, uint8_t *data = nullptr, int len = 0, uint32_t *rid = nullptr);
    int openCommandSession(const char *dev);
    int closeCommandSession(uint32_t *rid);
    void processResponse(uint8_t *, int);

    friend void pollingRead(MbimService *);
};
typedef MbimService MBIMSERVICE;

#endif //__MBIMSERVICE
