#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <linux/un.h>
#include <fcntl.h>
#include <string.h>
#include <termio.h>
#include <endian.h>

#include "MbimService.h"
#include "MbimMessage.h"
#include "logger.h"

static int open_unix_sock(const char *n)
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    struct sockaddr_un addr;
    socklen_t socklen;
    memset(&addr, 0, sizeof(sockaddr));
    addr.sun_family = AF_UNIX;
    memcpy((char *)addr.sun_path + 1, n, strlen(n));
    socklen = offsetof(sockaddr_un, sun_path) + strlen(n) + 1;

    if (connect(sockfd, (sockaddr *)&addr, socklen) < 0)
    {
        close(sockfd);
        return -1;
    }
    int soflag = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &soflag, sizeof(soflag));
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

    return sockfd;
}

static int open_tty(const char *d)
{
    int fd = open(d, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        LOGE << "fail to open " << d << " for " << strerror(errno) << ENDL;
        return fd;
    }

    return fd;
}

void pollingRead(MbimService *args)
{
    int epfd = epoll_create(10);

    epoll_event event;
    event.data.fd = args->mPollingHandle;
    event.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_ADD, args->mPollingHandle, &event);

    std::thread::id mPollID = std::this_thread::get_id();

    LOGI << "polling thread start polling, thread id " << mPollID << ENDL;
    epoll_event events[10];
    do
    {
        args->mReady = true;
        int num = epoll_wait(epfd, events, 10, 2000);
        if (num < 0)
        {
            LOGE << "epoll_wait get error: " << strerror(errno) << ENDL;
            goto quit_polling;
        }
        else if (num == 0)
        {
            if (args->mQuitFlag)
            {
                goto quit_polling;
            }
        }
        else
        {
            for (int i = 0; i < num; i++)
            {
                // serial closed?
                if (events[i].events & EPOLLRDHUP)
                {
                    LOGE << "peer close device" << ENDL;
                    goto quit_polling;
                }

                if (events[i].events & (EPOLLERR | EPOLLHUP))
                {
                    LOGE << "epoll error, event = " << events[i].events << ENDL;
                    goto quit_polling;
                }

                if (events[i].events & EPOLLIN)
                {
                    int olen = 0;
                    static uint8_t recvBuff[MAX_DATA_SIZE]; // RIL message has max size of 8K

                    // read message
                    if (!args->recvAsync(recvBuff, &olen) && olen > 0)
                    {
                        args->processResponse(recvBuff, olen);
                    }
                    else
                    {
                        LOGE << "read response data failed" << ENDL;
                    }
                }
            }
        }
    } while (1);

quit_polling:
    epoll_ctl(epfd, EPOLL_CTL_DEL, args->mPollingHandle, &event);
    close(epfd);
    LOGW << "polling thread quit polling" << ENDL;
    args->mReady = false;
    args->processResponse(nullptr, 0);
    close(args->mPollingHandle);
    args->mPollingHandle = 0;
}

MbimService::MbimService()
    : mDevice(""),
      mPollingHandle(0),
      mQuitFlag(false),
      mInitFlag(false),
      mReady(false)
{
}

MbimService::~MbimService()
{
}

bool MbimService::ready()
{
    return (mPollingHandle > 0) && mReady && mInitFlag;
}

void MbimService::release()
{
    mQuitFlag = true;
    if (mPollingHandle)
        close(mPollingHandle);
    mDevice = "";
}

/**
 * uses can pass null when call the function
 * we provide an candicante varibale 'mbim-proxy'
 * which means, use proxy mode
 */
bool MbimService::connect(const char *d)
{
    assert(d != nullptr);
    mDevice = d;
    if (!strncasecmp(d, "mbim", 4))
    {
        mPollingHandle = open_unix_sock(mDevice.c_str());
    }
    else
    {
        mPollingHandle = open_tty(mDevice.c_str());
    }

    if (mPollingHandle < 0)
        LOGD << "fail to open " << mDevice << ENDL;
    else
        LOGD << "successfuly open " << mDevice << " fd = " << mPollingHandle << ENDL;

    return (mPollingHandle > 0);
}

bool MbimService::startPolling()
{
    /** start polling */
    std::thread pollingthread(pollingRead, this);
    pollingthread.detach();

    LOGI << __FILE__ << " " << __LINE__ << ENDL;
    int max_try = 5;
    int try_time = 0;
    do
    {
        if (mReady)
            break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOGD << "startPolling try time = " << try_time << ENDL;
    } while (try_time < max_try);
    return (mPollingHandle > 0) && mReady;
}

int MbimService::sendAsync(uint8_t *d, int len)
{
    std::lock_guard<std::mutex> _lk(mRWLock);

    dump_msg(d, len);
    if (write(mPollingHandle, d, len) < 0)
    {
        LOGE << "send command fail for " << strerror(errno) << ENDL;
        return -1;
    }
    return 0;
}

int MbimService::recvAsync(uint8_t *d, int *len)
{
    std::lock_guard<std::mutex> _lk(mRWLock);
    uint8_t buffer[MAX_DATA_SIZE];
    int ret;

    assert(d != nullptr);
    if (len)
        *len = 0;

    if ((ret = read(mPollingHandle, buffer, sizeof(buffer))) < 0)
    {
        LOGE << "write command fail for " << strerror(errno) << ENDL;
        return -1;
    }

    std::copy(buffer, buffer + ret, d);
    if (len)
        *len = ret;

    dump_msg(buffer, ret);

    return 0;
}

int MbimService::setCommand(int cid, uint8_t *d, int len, uint32_t *rid)
{
    MbimMessage msg(UUID_MS_SARCONTROL);
    msg.newMbimSetCommand(cid, d, len);

    if (rid)
        *rid = msg.requestId();
    if (sendAsync(msg.toBytes(), msg.length()))
    {
        LOGE << "send setCommand fail" << ENDL;
        return -1;
    }
    return 0;
}

int MbimService::queryCommand(int cid, uint8_t *d, int len, uint32_t *rid)
{
    MbimMessage msg(UUID_MS_SARCONTROL);
    msg.newMbimQueryCommand(cid, d, len);

    if (rid)
        *rid = msg.requestId();
    if (sendAsync(msg.toBytes(), msg.length()))
    {
        LOGE << "send setCommand fail" << ENDL;
        return -1;
    }
    return 0;
}

int MbimService::openCommandSession(const char *dev)
{
    std::unique_lock<std::mutex> _lk(mOpenCloseLock);

    std::string proxy_name = "mbim-proxy";
    MbimMessage msg(UUID_PROXY_CONTROL);

    if (mDevice == proxy_name)
    {
        std::vector<uint8_t> vec = utf8_to_utf16(dev);
        int datalen = vec.size() + sizeof(MBIM_PROXY_T);
        auto msgBuf = new (std::nothrow) uint8_t[datalen];
        MBIM_PROXY_T *proxy = reinterpret_cast<MBIM_PROXY_T *>(msgBuf);
        proxy->Offset = htole32(sizeof(MBIM_PROXY_T));
        proxy->Length = htole32(strlen(dev) * 2);
        proxy->Timeout = htole32(30);
        std::copy(vec.begin(), vec.end(), proxy->Data);

        msg.newMbimSetCommand(1, msgBuf, datalen);
        delete[] msgBuf;
    }
    else
    {
        msg.newMbimOpen(4096);
    }

    LOGI << "open mbim session: " << mDevice << ENDL;
    if (sendAsync(msg.toBytes(), msg.length()))
    {
        LOGE << "send openCommandSession fail" << ENDL;
        return -1;
    }

    if (mOpenCloseCond.wait_for(_lk, std::chrono::seconds(5)) == std::cv_status::timeout)
    {
        LOGE << "openCommandSession fail" << ENDL;
        return -1;
    }

    LOGI << "finish open mbim session: " << mDevice << ENDL;
    return 0;
}

int MbimService::closeCommandSession()
{
    std::unique_lock<std::mutex> _lk(mOpenCloseLock);
    MbimMessage msg(UUID_BASIC_CONNECT);
    msg.newMbimClose();

    LOGI << "close mbim session" << ENDL;
    if (sendAsync(msg.toBytes(), msg.length()))
    {
        LOGE << "send closeCommandSession fail" << ENDL;
        return -1;
    }

    if (mOpenCloseCond.wait_for(_lk, std::chrono::seconds(5)) == std::cv_status::timeout)
    {
        LOGE << "closeCommandSession fail" << ENDL;
        return -1;
    }

    LOGI << "finish close mbim" << ENDL;
    return 0;
}

void MbimService::processResponse(uint8_t *data, int len)
{
    MBIM_MESSAGE_HEADER *msg = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);

    switch (le32toh(msg->MessageType))
    {
    case MBIM_OPEN_DONE:
    case MBIM_CLOSE_DONE:
    {
        MBIM_CLOSE_DONE_T *p = reinterpret_cast<MBIM_CLOSE_DONE_T *>(data);
        LOGD << "mbim open/close status = " << le32toh(p->Status) << ENDL;
        mInitFlag = !!p->Status;
        mOpenCloseCond.notify_one();
        break;
    }
    case MBIM_COMMAND_DONE:
    {
        /** process proxy message response here
         * we will send 'proxy-control' to mbim-proxy if works in proxy mode
        */
        MBIM_COMMAND_DONE_T *p = reinterpret_cast<MBIM_COMMAND_DONE_T *>(data);
        if (uuid2str(&p->DeviceServiceId) == std::string(UUID_PROXY_CONTROL))
        {
            LOGD << "mbim open/close status = " << le32toh(p->Status) << ENDL;
            mInitFlag = !!p->Status;
            mOpenCloseCond.notify_one();
            break;
        }
    }
    case MBIM_INDICATE_STATUS_MSG:
    {
        MBIM_MESSAGE_HEADER *ptr = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);
        notify(le32toh(ptr->TransactionId), data, len);
        break;
    }
    case MBIM_FUNCTION_ERROR_MSG:
    {
        MBIM_FUNCTION_ERROR_MSG_T *ptr = reinterpret_cast<MBIM_FUNCTION_ERROR_MSG_T *>(data);
        LOGE << "OOPS: function error received: code = " << le32toh(ptr->ErrorStatusCode) << ENDL;
    }
    default:
        break;
    }
}