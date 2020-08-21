#ifndef __MBIMSERVICE
#define __MBIMSERVICE

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include <stdint.h>

#include "ISubjectObserver.h"

/** 
 * MbimService should not be copyed
 */
class MbimService final : public ISubject
{
private:
    /* lock for send/recv */
    std::mutex mRWLock;
    std::mutex mOpenCloseLock;
    std::condition_variable mOpenCloseCond;

    /* lock for do commands */
    std::string mDevice;
    int mPollingHandle;
    bool mQuitFlag;
    bool mInitFlag; /* indicate whether MBIM is open */
    bool mReady;

private:
    MbimService(const MbimService &) = delete;
    MbimService &operator=(const MbimService &) = delete;

public:
    MbimService();
    ~MbimService();

    bool ready();
    void release();
    bool connect(const char *d = nullptr);
    int sendAsync(uint8_t *, int);
    int recvAsync(uint8_t *, int *);
    int setCommand(int cid, uint8_t *data = nullptr, int len = 0, uint32_t *rid = nullptr);
    int queryCommand(int cid, uint8_t *data = nullptr, int len = 0, uint32_t *rid = nullptr);
    int openCommandSession(const char *dev);
    int closeCommandSession();
    void processResponse(uint8_t *, int);
    bool startPolling();

    friend void pollingRead(MbimService *);
};
typedef MbimService MBIMSERVICE;

#endif //__MBIMSERVICE
