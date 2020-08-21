#include <iomanip>

#include "MbimMessage.h"

uint32_t MbimMessage::tansicationId = 0;

void str2uuid(UUID_T *uuid, const char *str)
{
    assert(str != nullptr && uuid != nullptr);
    uint32_t uuid_[16];

    memset(uuid_, 0, sizeof(uuid_));
    sscanf(str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           &uuid_[0], &uuid_[1], &uuid_[2], &uuid_[3],
           &uuid_[4], &uuid_[5], &uuid_[6], &uuid_[7],
           &uuid_[8], &uuid_[9], &uuid_[10], &uuid_[11],
           &uuid_[12], &uuid_[13], &uuid_[14], &uuid_[15]);
    std::copy(uuid_, uuid_ + 16, uuid->uuid);
}

std::string uuid2str(UUID_T *uuid)
{
    char str[64] = {'\0'};
    assert(uuid != nullptr);
    snprintf(str, sizeof(str), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3],
             uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
             uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11],
             uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    return std::string(str);
}

/*  convert utf8 string to utf16 endian string and return in byte array */
std::vector<uint8_t> utf8_to_utf16(const char *str)
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

void dump_msg(uint8_t *data, int datalen)
{
    if (!data || datalen <= 0)
        return;
    MBIM_MESSAGE_HEADER *pmsg = reinterpret_cast<MBIM_MESSAGE_HEADER *>(data);

    std::string direction = (le32toh(pmsg->MessageType) & 0x80000000) ? "<<<<<< " : ">>>>>> ";

    auto hex_dump = [&]() {
        LOGD << "(" << datalen << ")" << direction;
        for (int i = 0; i < datalen; i++)
            LOGD << std::hex << std::setw(2) << std::setfill('0') << uint32_t(data[i]) << ":";
        LOGD << std::dec << ENDL;
    };

    auto dump_header = [&]() {
        LOGD << direction << "Header" << ENDL;
        LOGD << direction << "  Type          = 0x" << std::hex << le32toh(pmsg->MessageType) << std::dec << ENDL;
        LOGD << direction << "  Length        = " << le32toh(pmsg->MessageLength) << ENDL;
        LOGD << direction << "  TransactionId = " << le32toh(pmsg->TransactionId) << ENDL;
    };

    auto dump_body = [&]() {
        MBIM_INDICATE_STATUS_MSG_T *p = reinterpret_cast<MBIM_INDICATE_STATUS_MSG_T *>(data);
        LOGD << direction << "Fragment" << ENDL;
        LOGD << direction << "  Current = " << le32toh(p->FragmentHeader.CurrentFragment) << ENDL;
        LOGD << direction << "  Total   = " << le32toh(p->FragmentHeader.TotalFragments) << ENDL;
        LOGD << direction << "Contents:" << ENDL;
        LOGD << direction << "  UUID = " << uuid2str(&p->DeviceServiceId) << ENDL;
        LOGD << direction << "  CID  = " << le32toh(p->CID) << ENDL;
        if (le32toh(p->MessageHeader.MessageType) == MBIM_COMMAND_MSG)
        {
            MBIM_COMMAND_MSG_T *pcmd = reinterpret_cast<MBIM_COMMAND_MSG_T *>(data);
            LOGD << direction << "  Type = " << le32toh(pcmd->CommandType) << ENDL;
            LOGD << direction << "Fields:" << ENDL;
            LOGD << direction << "  Length = " << le32toh(pcmd->InformationBufferLength) << ENDL;
        }
        else if (le32toh(p->MessageHeader.MessageType) == MBIM_COMMAND_DONE)
        {
            MBIM_COMMAND_DONE_T *pcmddone = reinterpret_cast<MBIM_COMMAND_DONE_T *>(data);
            LOGD << direction << "  Status = " << le32toh(pcmddone->Status) << ENDL;
            LOGD << direction << "Fields:" << ENDL;
            LOGD << direction << "  Length = " << le32toh(pcmddone->InformationBufferLength) << ENDL;
        }
        else
        {
            LOGD << direction << "Fields:" << ENDL;
            LOGD << direction << "  Length = " << le32toh(p->InformationBufferLength) << ENDL;
        }
    };

    hex_dump();
    switch (le32toh(pmsg->MessageType))
    {
    case MBIM_OPEN_MSG:
    {
        MBIM_OPEN_MSG_T *p = reinterpret_cast<MBIM_OPEN_MSG_T *>(data);
        dump_header();
        LOGD << direction << "Contents:" << ENDL;
        LOGD << direction << "  MaxControlTransfer = " << le32toh(p->MaxControlTransfer) << ENDL;
        break;
    }
    case MBIM_OPEN_DONE:
    case MBIM_CLOSE_DONE:
    {
        MBIM_CLOSE_DONE_T *p = reinterpret_cast<MBIM_CLOSE_DONE_T *>(data);
        dump_header();
        LOGD << direction << "Contents:" << ENDL;
        LOGD << direction << "  Status = " << le32toh(p->Status) << ENDL;
        break;
    }
    case MBIM_CLOSE_MSG:
    {
        dump_header();
        break;
    }
    case MBIM_COMMAND_MSG:
    case MBIM_COMMAND_DONE:
    case MBIM_INDICATE_STATUS_MSG:
    {
        dump_header();
        dump_body();
        break;
    }
    case MBIM_FUNCTION_ERROR_MSG:
    {
        MBIM_FUNCTION_ERROR_MSG_T *p = reinterpret_cast<MBIM_FUNCTION_ERROR_MSG_T *>(data);
        dump_header();
        LOGD << direction << "Contents:" << ENDL;
        LOGD << direction << "  ErrorStatusCode = " << le32toh(p->ErrorStatusCode) << ENDL;
        break;
    }
    default:
        LOGE << "unknow mbim message type " << le32toh(pmsg->MessageType) << ENDL;
    }
}