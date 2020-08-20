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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    struct sockaddr_un addr;
    socklen_t socklen;
    memset(&addr, 0, sizeof(sockaddr));
    addr.sun_family = AF_LOCAL;
    memcpy((char *)addr.sun_path + 1, n, strlen(n));
    socklen = offsetof(sockaddr_un, sun_family) + strlen(n) + 1;

    int soflag = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &soflag, sizeof(soflag));
    if (connect(sockfd, (sockaddr *)&addr, socklen) < 0)
    {
        close(sockfd);
        return -1;
    }
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

    struct termios tio;
    struct termios settings;
    int retval;
    memset(&tio, 0, sizeof(tio));
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 5;
    cfsetospeed(&tio, B115200); // 115200 baud
    cfsetispeed(&tio, B115200); // 115200 baud
    tcsetattr(fd, TCSANOW, &tio);
    retval = tcgetattr(fd, &settings);
    if (-1 == retval)
    {
        LOGE << "setUart error" << ENDL;
    }
    cfmakeraw(&settings);
    settings.c_cflag |= CREAD | CLOCAL;
    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd, TCSANOW, &settings);

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
                    int length = 0;

                    // read message length
                    // read message body
                    if (args->recvAsync(recvBuff, &olen) && olen > 0)
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
}

std::mutex MbimService::mRWLock;
MbimService &MbimService::Instance()
{
    static MbimService instance;
    return instance;
}

bool MbimService::ready()
{
    return mPollingHandle > 0;
}

void MbimService::release()
{
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
    if (!d || 0 == strlen(d) || !strncasecmp(d, "mbim", 4))
    {
        mDevice = proxy_name;
        mPollingHandle = open_unix_sock(mDevice.c_str());
    }
    else
    {
        mDevice = d;
        mPollingHandle = open_tty(mDevice.c_str());
    }

    if (mPollingHandle < 0)
        LOGD << "fail to open " << mDevice << ENDL;
    else
        LOGD << "successfuly open " << mDevice << " fd = " << mPollingHandle << ENDL;
    return (mPollingHandle > 0);
}

int MbimService::sendAsync(uint8_t *d, int len)
{
    std::lock_guard<std::mutex> _lk(mRWLock);

    dump_msg(d, len);
    if (write(mPollingHandle, d, len) < 0)
    {
        LOGE << "write command fail for " << strerror(errno) << ENDL;
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
    MbimMessage msg(UUID_PROXY_CONTROL);

    if (mDevice == proxy_name)
    {
        std::vector<uint8_t> vec = utf8_to_utf16(proxy_name);
        int datalen = vec.size() + sizeof(MBIM_PROXY_T);
        auto msgBuf = new (std::nothrow) uint8_t[datalen];
        MBIM_PROXY_T *proxy = reinterpret_cast<MBIM_PROXY_T *>(msgBuf);
        proxy->Offset = htole32(sizeof(MBIM_PROXY_T));
        proxy->Length = htole32(vec.size());
        proxy->Timeout = htole32(30);
        std::copy(vec.begin(), vec.end(), proxy->Data);
        msg.newMbimSetCommand(1, msgBuf, datalen);
        delete[] msgBuf;
    }
    else
    {
        msg.newMbimOpen(4096);
    }

    if (sendAsync(msg.toBytes(), msg.length()))
    {
        LOGE << "send setCommand fail" << ENDL;
        return -1;
    }
    return 0;
}

int MbimService::closeCommandSession(uint32_t *rid)
{
    MbimMessage msg(UUID_BASIC_CONNECT);
    msg.newMbimClose();

    if (rid)
        *rid = msg.requestId();
    if (sendAsync(msg.toBytes(), msg.length()))
    {
        LOGE << "send setCommand fail" << ENDL;
        return -1;
    }
    return 0;
}

void MbimService::processResponse(uint8_t *data, int len)
{
    MBIM_MESSAGE_HEADER *msg = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);
    int tid = le32toh(msg->TransactionId);

    auto get_processer = [&]() {
        MBIM_INDICATE_STATUS_MSG_T *ptr = reinterpret_cast<MBIM_INDICATE_STATUS_MSG_T *>(data);
        std::map<int, std::function<void(uint8_t *, int)>> handle;

        if (uuid2str(&ptr->DeviceServiceId) == UUID_MS_SARCONTROL)
            handle = mbimSarHandle;
        else
            LOGW << "cannot find handler for response" << ENDL;
        return handle;
    };

    switch (le32toh(msg->MessageType))
    {
    case MBIM_OPEN_DONE:
    case MBIM_CLOSE_DONE:
    case MBIM_COMMAND_DONE:
    case MBIM_INDICATE_STATUS_MSG:
    {
        MBIM_MESSAGE_HEADER *ptr = reinterpret_cast<MBIM_MESSAGE_HEADER*>(data);
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