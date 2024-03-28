/**
 * @file espnow_ser_des.cpp
 * @author Dávid Benko (davidbenko@davidbenko.dev)
 * @brief ESP-NOW packet serializer and deserializer
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cinttypes>
#include <cstring>

#include "spsp/chacha20.hpp"
#include "spsp/espnow_ser_des.hpp"
#include "spsp/logger.hpp"

// Log tag
static const char* SPSP_LOG_TAG = "SPSP/Local/ESPNOW/SerDes";

namespace SPSP::LocalLayers::ESPNOW
{
    SerDes::SerDes(const Config& conf) noexcept : m_conf{conf}
    {
    }

    void SerDes::serialize(const LocalMessageT& msg, std::string& data) const noexcept
    {
        size_t topicLen = msg.topic.length();
        size_t payloadLen = msg.payload.length();
        size_t dataLen = this->getPacketLength(msg);

        // Allocate buffer
        uint8_t* dataRaw = new uint8_t[dataLen];

        Packet* p = reinterpret_cast<Packet*>(dataRaw);

        // Fill data
        p->header.ssid = m_conf.ssid;
        p->header.version = PROTO_VERSION;
        p->payload.type = msg.type;
        p->payload.checksum = 0;
        p->payload.topicLen = topicLen;
        p->payload.payloadLen = payloadLen;

        // Copy topic and payload
        memcpy(p->payload.topicAndPayload, msg.topic.c_str(), topicLen);
        memcpy(p->payload.topicAndPayload + topicLen, msg.payload.c_str(),
               payloadLen);

        // Generate nonce
        m_rand.bytes(&(p->header.nonce), NONCE_LEN);

        // Skip header
        auto dataRawNoHeader = dataRaw + sizeof(PacketHeader);
        auto dataLenNoHeader = dataLen - sizeof(PacketHeader);

        // Checksum
        p->payload.checksum = this->checksumRaw(dataRawNoHeader,
                                                dataLenNoHeader);

        // Encrypt
        this->encryptRaw(dataRawNoHeader, dataLenNoHeader, p->header.nonce);

        // Convert to std::string
        data = std::string(reinterpret_cast<char*>(dataRaw), dataLen);

        delete[] dataRaw;
    }

    bool SerDes::deserialize(const LocalAddrT& src, std::string& data,
                             LocalMessageT& msg) const noexcept
    {
        size_t dataLen = data.length();

        // Check packet length
        if (dataLen < sizeof(Packet)) {
            SPSP_LOGD("Deserialize failed: packet too short (%zu < %zu bytes)",
                      dataLen, sizeof(Packet));
            return false;
        }

        // Allocate buffer and copy data
        uint8_t* dataRaw = new uint8_t[dataLen];
        memcpy(dataRaw, data.c_str(), dataLen);

        // Treat as `Packet`
        Packet* p = reinterpret_cast<Packet*>(dataRaw);

        // Validate and decrypt
        if (!this->validatePacketHeader(p)) {
            delete[] dataRaw;
            return false;
        }
        if (!this->decryptAndValidatePacketPayload(dataRaw, dataLen)) {
            delete[] dataRaw;
            return false;
        }

        const char* topicAndPayload = reinterpret_cast<const char*>(p->payload.topicAndPayload);

        // Construct message
        msg = {};
        msg.type = p->payload.type;
        msg.addr = src;
        msg.topic = std::string{topicAndPayload, p->payload.topicLen};
        msg.payload = std::string{topicAndPayload + p->payload.topicLen,
                                  p->payload.payloadLen};

        delete[] dataRaw;
        return true;
    }

    size_t SerDes::getPacketLength(const LocalMessageT& msg) noexcept
    {
        return sizeof(Packet) + msg.topic.length() + msg.payload.length();
    }

    uint8_t SerDes::checksumRaw(uint8_t* data, size_t dataLen,
                                uint8_t existingChecksum) noexcept
    {
        uint8_t checksum = 0;

        for (size_t i = 0; i < dataLen; i++) {
            checksum += data[i];
        }

        return checksum - existingChecksum;
    }

    void SerDes::encryptRaw(uint8_t* data, size_t dataLen,
                            const uint8_t* nonce) const noexcept
    {
        Chacha20 chch{
            reinterpret_cast<const uint8_t*>(m_conf.password.c_str()),
            nonce
        };

        chch.crypt(reinterpret_cast<uint8_t*>(data), dataLen);
    }

    bool SerDes::validatePacketHeader(const Packet* p) const noexcept
    {
        // Check SSID
        if (p->header.ssid != m_conf.ssid) {
            SPSP_LOGD("Deserialize failed: different SSID (0x%" PRIx32 " != 0x%" PRIx32 ")",
                      p->header.ssid, m_conf.ssid);
            return false;
        }

        // Check version
        if (p->header.version != PROTO_VERSION) {
            SPSP_LOGD("Deserialize failed: different protocol version (%u != %u)",
                      p->header.version, PROTO_VERSION);
            return false;
        }

        return true;
    }

    bool SerDes::decryptAndValidatePacketPayload(uint8_t* data, size_t dataLen) const noexcept
    {
        // Treat as `Packet`
        Packet* p = reinterpret_cast<Packet*>(data);

        // Skip header
        data += sizeof(PacketHeader);
        dataLen -= sizeof(PacketHeader);

        // Decrypt
        this->encryptRaw(data, dataLen, p->header.nonce);

        // Check checksum
        uint8_t checksum = this->checksumRaw(data, dataLen, p->payload.checksum);

        if (p->payload.checksum != checksum) {
            SPSP_LOGD("Deserialize failed: invalid checksum (%u != %u)",
                      p->payload.checksum, checksum);
            return false;
        }

        // Assert valid payload length
        size_t payloadTotalLen = sizeof(PacketPayload) + p->payload.topicLen
                                 + p->payload.payloadLen;
        if (payloadTotalLen != dataLen) {
            SPSP_LOGD("Deserialize failed: invalid total length without header (%zu != %zu bytes)",
                      payloadTotalLen, dataLen);
            return false;
        }

        return true;
    }
} // namespace SPSP::LocalLayers::ESPNOW
