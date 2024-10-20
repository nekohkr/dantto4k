#include "mpuAssembler.h"

bool MpuAssembler::assemble(const std::vector<uint8_t>& fragment, uint8_t fragmentationIndicator, uint32_t packetSequenceNumber)
{
    switch (fragmentationIndicator) {
    case NOT_FRAGMENTED:
        if (state == MPU_ASSEMBLER_STATE::IN_FRAGMENT)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        state = MPU_ASSEMBLER_STATE::NOT_STARTED;
        return true;
    case FIRST_FRAGMENT:
        if (state == MPU_ASSEMBLER_STATE::IN_FRAGMENT) {
            return false;
        }

        state = MPU_ASSEMBLER_STATE::IN_FRAGMENT;
        data.insert(data.end(), fragment.begin(), fragment.end());
        break;
    case MIDDLE_FRAGMENT:
        if (state == MPU_ASSEMBLER_STATE::SKIP) {
            std::cerr << "Drop packet " << packetSequenceNumber << std::endl;
            return false;
        }

        if (state != MPU_ASSEMBLER_STATE::IN_FRAGMENT)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        break;
    case LAST_FRAGMENT:
        if (state == MPU_ASSEMBLER_STATE::SKIP) {
            std::cerr << "Drop packet " << packetSequenceNumber << std::endl;
            return false;
        }

        if (state != MPU_ASSEMBLER_STATE::IN_FRAGMENT)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        state = MPU_ASSEMBLER_STATE::NOT_STARTED;
        return true;
    }

    return false;
}

void MpuAssembler::checkState(uint32_t packetSequenceNumber)
{
    if (state == MPU_ASSEMBLER_STATE::INIT) {
        state = MPU_ASSEMBLER_STATE::SKIP;
    }
    else if (packetSequenceNumber != last_seq + 1) {
        if (data.size() != 0) {
            std::cerr << "Packet sequence number jump: " << last_seq << " + 1 != " << packetSequenceNumber << ", drop " << data.size() << " bytes\n" << std::endl;
            data.clear();
        }
        else {
            std::cerr <<
                "Packet sequence number jump: " << last_seq << " + 1 != " << packetSequenceNumber << std::endl;
        }

        state = MPU_ASSEMBLER_STATE::SKIP;
    }
    last_seq = packetSequenceNumber;
}

void MpuAssembler::clear()
{
    state = MPU_ASSEMBLER_STATE::NOT_STARTED;
    data.clear();
}
