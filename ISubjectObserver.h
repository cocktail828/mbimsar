#ifndef __ISUBJECTOBSERVER
#define __ISUBJECTOBSERVER

#include <mutex>
#include <condition_variable>
#include <vector>

#include "logger.h"

class IObserver
{
protected:
    std::mutex mLock;
    std::condition_variable mCond;

public:
    IObserver() {}
    virtual ~IObserver() {}
    virtual void update(uint8_t *, int) = 0;
    virtual uint32_t getRequestId() = 0;
};

// TODO, distinguish sync/async requests
class ISubject
{
private:
    std::mutex mListLock;
    std::vector<IObserver *> mObservers;
    const size_t mMaxSize = 10;

public:
    void attach(IObserver *o)
    {
        std::lock_guard<std::mutex> _lk(mListLock);
        mObservers.emplace_back(o);
        LOGD << "attach new observer(" + std::to_string(mObservers.size()) + ") tid "
             << o->getRequestId() << ", list size " << mObservers.size() << ENDL;
    }

    void detach(IObserver *o)
    {
        std::lock_guard<std::mutex> _lk(mListLock);
        for (auto iter = mObservers.begin(); iter != mObservers.end();)
        {
            if ((*iter) == o)
            {
                mObservers.erase(iter);
                LOGD << "detach observer(" + std::to_string(mObservers.size()) + ") tid "
                     << o->getRequestId()
                     << ", list size " << mObservers.size() << ENDL;
            }
            else
            {
                iter++;
            }
        }
    }

    void detachAll()
    {
        std::lock_guard<std::mutex> _lk(mListLock);
        /* notice all the ovservers the quit event */

        LOGD << "removing all observers" << ENDL;
        for (auto o : mObservers)
        {
            LOGD << "removing observer: tid " << o->getRequestId() << ENDL;
            o->update(nullptr, 0);
        }

        mObservers.clear();
        LOGD << "removing all observers finished" << ENDL;
    }

    void notify(uint32_t tid, uint8_t *data, int len)
    {
        std::lock_guard<std::mutex> _lk(mListLock);

        LOGD << "try to find and notify observer, tid " << tid << ENDL;
        for (auto iter = mObservers.begin(); iter != mObservers.end();)
        {
            IObserver *o = *iter;
            if (o->getRequestId() == tid)
            {
                mObservers.erase(iter);
                LOGD << "detach observer(" + std::to_string(mObservers.size()) + ") tid "
                     << o->getRequestId() << ENDL;
                o->update(data, len);
                break;
            }
        }
    }

    bool isListFull()
    {
        std::lock_guard<std::mutex> _lk(mListLock);
        return (mObservers.size() >= mMaxSize);
    }
};

#endif //__ISUBJECTOBSERVER
