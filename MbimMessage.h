#ifndef _MBIM_MESSAGE
#define _MBIM_MESSAGE

#include <iostream>
#include <cassert>
#include <vector>

#include <string.h>

#include "logger.h"

#define MBIM_OPEN_MSG 1
#define MBIM_CLOSE_MSG 2
#define MBIM_COMMAND_MSG 3
#define MBIM_HOST_ERROR_MSG 4
#define MBIM_OPEN_DONE 0x80000001
#define MBIM_CLOSE_DONE 0x80000002
#define MBIM_COMMAND_DONE 0x80000003
#define MBIM_FUNCTION_ERROR_MSG 0x80000004
#define MBIM_INDICATE_STATUS_MSG 0x80000007

#define UUID_BASIC_CONNECT "a289cc33-bcbb-8b4f-b6b0-133ec2aae6df"
//https://docs.microsoft.com/en-us/windows-hardware/drivers/network/mb-5g-data-class-support
#define UUID_BASIC_CONNECT_EXT "3d01dcc5-fef5-4d05-0d3a-bef7058e9aaf"
#define UUID_SMS "533fbeeb-14fe-4467-9f90-33a223e56c3f"
#define UUID_USSD "e550a0c8-5e82-479e-82f7-10abf4c3351f"
#define UUID_PHONEBOOK "4bf38476-1e6a-41db-b1d8-bed289c25bdb"
#define UUID_STK "d8f20131-fcb5-4e17-8602-d6ed3816164c"
#define UUID_AUTH "1d2b5ff7-0aa1-48b2-aa52-50f15767174e"
#define UUID_DSS "c08a26dd-7718-4382-8482-6e0d583c4d0e"
#define UUID_EXT_QMUX "d1a30bc2-f97a-6e43-bf65-c7e24fb0f0d3"
#define UUID_MSHSD "883b7c26-985f-43fa-9804-27d7fb80959c"
#define UUID_QMBE "2d0c12c9-0e6a-495a-915c-8d174fe5d63c"
#define UUID_MSFWID "e9f7dea2-feaf-4009-93ce-90a3694103b6"
#define UUID_ATDS "5967bdcc-7fd2-49a2-9f5c-b2e70e527db3"
#define UUID_QDU "6427015f-579d-48f5-8c54-f43ed1e76f83"
#define UUID_MS_UICC_LOW_LEVEL "c2f6588e-f037-4bc9-8665-f4d44bd09367"
#define UUID_MS_SARCONTROL "68223D04-9F6C-4E0F-822D-28441FB72340"
#define UUID_VOICEEXTENSIONS "8d8b9eba-37be-449b-8f1e-61cb034a702e"
#define UUID_MBIMCONTEXTTYPEINTERNET "7E5E2A7E-4E6F-7272-736B-656E7E5E2A7E"
#define UUID_PROXY_CONTROL "838cf7fb-8d0d-4d7f-871e-d71dbefbb39b"

typedef struct
{
    uint32_t MessageType;   //Specifies the MBIM message type.
    uint32_t MessageLength; //Specifies the total length of this MBIM message in bytes.
    /* Specifies the MBIM message id value.  This value is used to match host sent messages with function responses. 
    This value must be unique among all outstanding transactions.    
    For notifications, the TransactionId must be set to 0 by the function */
    uint32_t TransactionId;
} MBIM_MESSAGE_HEADER;

typedef struct
{
    uint32_t TotalFragments;  //this field indicates how many fragments there are intotal.
    uint32_t CurrentFragment; //This field indicates which fragment this message is.  Values are 0 to TotalFragments?\1
} MBIM_FRAGMENT_HEADER;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    uint32_t MaxControlTransfer;
} MBIM_OPEN_MSG_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    uint32_t Status;
} MBIM_OPEN_DONE_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
} MBIM_CLOSE_MSG_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    uint32_t Status;
} MBIM_CLOSE_DONE_T;

typedef struct
{
    uint8_t uuid[16];
} UUID_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    MBIM_FRAGMENT_HEADER FragmentHeader;
    UUID_T DeviceServiceId;           //A 16 byte UUID that identifies the device service the following CID value applies.
    uint32_t CID;                     //Specifies the CID that identifies the parameter being queried for
    uint32_t CommandType;             //0 for a query operation, 1 for a Set operation
    uint32_t InformationBufferLength; //Size of the Total InformationBuffer, may be larger than current message if fragmented.
    uint8_t InformationBuffer[0];     //Data supplied to device specific to the CID
} MBIM_COMMAND_MSG_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    MBIM_FRAGMENT_HEADER FragmentHeader;
    UUID_T DeviceServiceId; //A 16 byte UUID that identifies the device service the following CID value applies.
    uint32_t CID;           //Specifies the CID that identifies the parameter being queried for
    uint32_t Status;
    uint32_t InformationBufferLength; //Size of the Total InformationBuffer, may be larger than current message if fragmented.
    uint8_t InformationBuffer[0];     //Data supplied to device specific to the CID
} MBIM_COMMAND_DONE_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    uint32_t ErrorStatusCode;
} MBIM_HOST_ERROR_MSG_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    uint32_t ErrorStatusCode;
} MBIM_FUNCTION_ERROR_MSG_T;

typedef struct
{
    MBIM_MESSAGE_HEADER MessageHeader;
    MBIM_FRAGMENT_HEADER FragmentHeader;
    UUID_T DeviceServiceId;           //A 16 byte UUID that identifies the device service the following CID value applies.
    uint32_t CID;                     //Specifies the CID that identifies the parameter being queried for
    uint32_t InformationBufferLength; //Size of the Total InformationBuffer, may be larger than current message if fragmented.
    uint8_t InformationBuffer[0];     //Data supplied to device specific to the CID
} MBIM_INDICATE_STATUS_MSG_T;

typedef struct
{
    uint32_t Offset;
    uint32_t Length;
    uint32_t Timeout;
    uint8_t Data[0];
} MBIM_PROXY_T;

static void str2uuid(UUID_T *uuid, const char *str)
{
    assert(str && uuid);
    sscanf(str, "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
           &uuid->uuid[0], &uuid->uuid[1], &uuid->uuid[2], &uuid->uuid[3],
           &uuid->uuid[4], &uuid->uuid[5], &uuid->uuid[6], &uuid->uuid[7],
           &uuid->uuid[8], &uuid->uuid[9], &uuid->uuid[10], &uuid->uuid[11],
           &uuid->uuid[12], &uuid->uuid[13], &uuid->uuid[14], &uuid->uuid[15]);
}

static std::string uuid2str(UUID_T *uuid)
{
    char str[64] = {'\0'};
    assert(uuid);
    snprintf(str, sizeof(str), "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
             uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3],
             uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
             uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11],
             uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    return std::string(str);
}

/*  convert utf8 string to utf16 endian string and return in byte array */
static std::vector<uint8_t> utf8_to_utf16(const char *str)
{
    std::vector<uint8_t> vec;
    if (nullptr == str || 0 == strlen(str))
        return vec;

    std::string string8(str);
    for (auto c : string8)
    {
        vec.emplace_back(c);
        vec.emplace_back(0);
    }

    if (vec.size() % 4)
    {
        vec.emplace_back(0);
        vec.emplace_back(0);
    }
    return vec;
}

#define MAX_DATA_SIZE (4 * 1024)
class MbimMessage
{
public:
    static const int max_message_length = MAX_DATA_SIZE;
    static uint32_t tansicationId;

private:
    const char *uuid;
    uint8_t *data;
    int datalen;

public:
    explicit MbimMessage()
        : uuid(UUID_BASIC_CONNECT),
          data(nullptr),
          datalen(0) {}

    explicit MbimMessage(const char *u)
        : uuid(u),
          data(nullptr),
          datalen(0) {}

    MbimMessage(const MbimMessage &m)
    {
        uuid = m.uuid;
        datalen = m.datalen;
        data = new (std::nothrow) uint8_t[datalen];
        std::copy(data, data + datalen, m.data);
    }

    MbimMessage &operator=(const MbimMessage &m)
    {
        if (&m == this)
            return *this;

        if (data)
            delete[] data;
        uuid = m.uuid;
        datalen = m.datalen;
        data = new (std::nothrow) uint8_t[datalen];
        std::copy(data, data + datalen, m.data);
        return *this;
    }

    ~MbimMessage()
    {
        if (data)
            delete[] data;
        datalen = 0;
    }

    MBIM_OPEN_MSG_T *newMbimOpen(int MaxControlTransfer)
    {
        datalen = sizeof(MBIM_OPEN_MSG_T);
        data = new (std::nothrow) uint8_t[datalen];

        MBIM_OPEN_MSG_T *ptr = reinterpret_cast<MBIM_OPEN_MSG_T *>(data);
        ptr->MessageHeader.MessageType = htole32(MBIM_OPEN_MSG);
        ptr->MessageHeader.MessageLength = htole32(sizeof(MBIM_OPEN_MSG_T));
        ptr->MessageHeader.TransactionId = htole32(++tansicationId);
        ptr->MaxControlTransfer = htole32(max_message_length);
        return ptr;
    }

    MBIM_CLOSE_MSG_T *newMbimClose()
    {
        datalen = sizeof(MBIM_CLOSE_MSG_T);
        data = new (std::nothrow) uint8_t[datalen];

        MBIM_CLOSE_MSG_T *ptr = reinterpret_cast<MBIM_CLOSE_MSG_T *>(data);
        ptr->MessageHeader.MessageType = htole32(MBIM_CLOSE_MSG);
        ptr->MessageHeader.MessageLength = htole32(sizeof(MBIM_CLOSE_MSG_T));
        ptr->MessageHeader.TransactionId = htole32(++tansicationId);
        return ptr;
    }

    MBIM_COMMAND_MSG_T *newMbimCommand(const char *uuidstr, int cid, bool isset, uint8_t *d = nullptr, int len = 0)
    {
        datalen = sizeof(MBIM_COMMAND_MSG_T) + len;
        MBIM_COMMAND_MSG_T *ptr = reinterpret_cast<MBIM_COMMAND_MSG_T *>(new (std::nothrow) uint8_t[datalen]);

        ptr->MessageHeader.MessageType = htole32(MBIM_COMMAND_MSG);
        ptr->MessageHeader.MessageLength = htole32(sizeof(MBIM_COMMAND_MSG_T));
        ptr->MessageHeader.TransactionId = htole32(++tansicationId);

        ptr->FragmentHeader.CurrentFragment = htole32(0);
        ptr->FragmentHeader.TotalFragments = htole32(1);

        str2uuid(&ptr->DeviceServiceId, uuidstr);
        ptr->CID = htole32(cid);
        ptr->CommandType = htole32(static_cast<int>(isset)); //0 for a query operation, 1 for a Set operation
        ptr->InformationBufferLength = htole32(len);
        std::copy(ptr->InformationBuffer, ptr->InformationBuffer + len, d);
        return ptr;
    }

    MBIM_COMMAND_MSG_T *newMbimCommand(int cid, bool isset, uint8_t *d = nullptr, int len = 0)
    {
        return newMbimCommand(uuid, cid, isset, d, len);
    }

    MBIM_COMMAND_MSG_T *newMbimSetCommand(int cid, uint8_t *d = nullptr, int len = 0)
    {
        return newMbimCommand(uuid, cid, true, d, len);
    }

    MBIM_COMMAND_MSG_T *newMbimQueryCommand(int cid, uint8_t *d = nullptr, int len = 0)
    {
        return newMbimCommand(uuid, cid, false, d, len);
    }

    uint8_t *toBytes()
    {
        return data;
    }

    int length()
    {
        return datalen;
    }

    uint32_t requestId() { return tansicationId; }

private:
};

uint32_t MbimMessage::tansicationId = 0;

static void dump_msg(uint8_t *data, int datalen)
{
    if (!data || datalen <= 0)
        return;
    MBIM_MESSAGE_HEADER *pmsg = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);

    std::string direction = (le32toh(pmsg->MessageType) & 0x80000000) ? "<<<<<< " : ">>>>>> ";

    auto hex_dump = [&]() {
        std::string buf(reinterpret_cast<char *>(data));
        LOGD << "(" << datalen << ")" << direction;
        for (auto c : buf)
            LOGD << std::hex << c << ' ';
        LOGD << std::dec << ENDL;
    };

    auto dump_header = [&]() {
        LOGD << direction << "Mbim Header" << ENDL;
        LOGD << direction << "MessageType   " << le32toh(pmsg->MessageType) << ENDL;
        LOGD << direction << "MessageLength " << le32toh(pmsg->MessageLength) << ENDL;
        LOGD << direction << "TransactionId " << le32toh(pmsg->TransactionId) << ENDL;
    };

    auto dump_body = [&]() {
        MBIM_INDICATE_STATUS_MSG_T *p = reinterpret_cast<MBIM_INDICATE_STATUS_MSG_T *>(data);
        LOGD << direction << "FragmentHeader" << ENDL;
        LOGD << direction << "CurrentFragment " << le32toh(p->FragmentHeader.CurrentFragment) << ENDL;
        LOGD << direction << "TotalFragments " << le32toh(p->FragmentHeader.TotalFragments) << ENDL;
        LOGD << direction << "UUID " << uuid2str(&p->DeviceServiceId) << ENDL;
        LOGD << direction << "CommandID " << le32toh(p->CID) << ENDL;
        LOGD << direction << "InformationLength " << le32toh(p->InformationBufferLength) << ENDL;
    };

    hex_dump();
    switch (le32toh(pmsg->MessageType))
    {
    case MBIM_OPEN_MSG:
    {
        MBIM_OPEN_MSG_T *p = reinterpret_cast<MBIM_OPEN_MSG_T *>(data);
        dump_header();
        LOGD << direction << "MaxControlTransfer " << le32toh(p->MaxControlTransfer) << ENDL;
        break;
    }
    case MBIM_OPEN_DONE:
    case MBIM_CLOSE_DONE:
    {
        MBIM_CLOSE_DONE_T *p = reinterpret_cast<MBIM_CLOSE_DONE_T *>(data);
        dump_header();
        LOGD << direction << "Status " << le32toh(p->Status) << ENDL;
        break;
    }
    case MBIM_CLOSE_MSG:
    {
        dump_header();
        break;
    }
    case MBIM_COMMAND_MSG:
    {
        MBIM_COMMAND_MSG_T *p = reinterpret_cast<MBIM_COMMAND_MSG_T *>(data);
        dump_header();
        dump_body();
        LOGD << direction << "CommandType " << le32toh(p->CommandType) << ENDL;
        break;
    }
    case MBIM_COMMAND_DONE:
    {
        MBIM_COMMAND_DONE_T *p = reinterpret_cast<MBIM_COMMAND_DONE_T *>(data);
        dump_header();
        dump_body();
        LOGD << direction << "Status " << le32toh(p->Status) << ENDL;
        break;
    }
    case MBIM_INDICATE_STATUS_MSG:
    {
        MBIM_INDICATE_STATUS_MSG_T *p = reinterpret_cast<MBIM_INDICATE_STATUS_MSG_T *>(data);
        dump_header();
        dump_body();
        break;
    }
    case MBIM_FUNCTION_ERROR_MSG:
    {
        MBIM_FUNCTION_ERROR_MSG_T *p = reinterpret_cast<MBIM_FUNCTION_ERROR_MSG_T *>(data);
        dump_header();
        LOGD << direction << "ErrorStatusCode " << le32toh(p->ErrorStatusCode) << ENDL;
        break;
    }
    default:
        LOGE << "unknow mbim message type " << le32toh(pmsg->MessageType) << ENDL;
    }
}
#endif //_MBIM_MESSAGE
