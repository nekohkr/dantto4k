#include "fragmentAssembler.h"
#include "mmtFragment.h"

namespace MmtTlv {

bool FragmentAssembler::assemble(const std::vector<uint8_t>& fragment, FragmentationIndicator fragmentationIndicator, uint32_t packetSequenceNumber)
{
    switch (fragmentationIndicator) {
    case FragmentationIndicator::NotFragmented:
        if (state == State::InFragment)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        state = State::NotStarted;
        return true;
    case FragmentationIndicator::FirstFragment:
        if (state == State::InFragment) {
            return false;
        }

        state = State::InFragment;
        data.insert(data.end(), fragment.begin(), fragment.end());
        break;
    case FragmentationIndicator::MiddleFragment:
        if (state == State::Skip) {
#ifndef _DANTTO4K_DLL
            std::cerr << "Drop packet " << packetSequenceNumber << std::endl;
#endif
            return false;
        }

        if (state != State::InFragment)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        break;
    case FragmentationIndicator::LastFragment:
        if (state == State::Skip) {
#ifndef _DANTTO4K_DLL
            std::cerr << "Drop packet " << packetSequenceNumber << std::endl;
#endif
            return false;
        }

        if (state != State::InFragment)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        state = State::NotStarted;
        return true;
    }

    return false;
}

void FragmentAssembler::checkState(uint32_t packetSequenceNumber)
{
    if (state == State::Init) {
        state = State::Skip;
    }
    else if (packetSequenceNumber != last_seq + 1) {
        if (data.size() != 0) {
#ifndef _DANTTO4K_DLL
            std::cerr << "Packet sequence number jump: " << last_seq << " + 1 != " << packetSequenceNumber << ", drop " << data.size() << " bytes\n" << std::endl;
#endif
            data.clear();
        }
        else {
#ifndef _DANTTO4K_DLL
            std::cerr << "Packet sequence number jump: " << last_seq << " + 1 != " << packetSequenceNumber << std::endl;
#endif
        }

        state = State::Skip;
    }
    last_seq = packetSequenceNumber;
}

void FragmentAssembler::clear()
{
    state = State::NotStarted;
    data.clear();
}

}