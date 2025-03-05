#pragma once
#include <cstdint>

namespace B24ControlSet {

    static constexpr uint8_t NUL = 0x00;
    static constexpr uint8_t BEL = 0x07;
    static constexpr uint8_t APB = 0x08;
    static constexpr uint8_t APF = 0x09;
    static constexpr uint8_t APD = 0x0A;
    static constexpr uint8_t APU = 0x0B;
    static constexpr uint8_t CS = 0x0C;
    static constexpr uint8_t APR = 0x0D;
    static constexpr uint8_t LS1 = 0x0E;
    static constexpr uint8_t LS0 = 0x0F;
    static constexpr uint8_t PAPF = 0x16;
    static constexpr uint8_t CAN = 0x18;
    static constexpr uint8_t SS2 = 0x19;
    static constexpr uint8_t ESC = 0x1B;
    static constexpr uint8_t APS = 0x1C;
    static constexpr uint8_t SS3 = 0x1D;
    static constexpr uint8_t RS = 0x1E;
    static constexpr uint8_t US = 0x1F;
    static constexpr uint8_t SP = 0x20;
    static constexpr uint8_t SWF = 0x53;
    static constexpr uint8_t SDF = 0x56;
    static constexpr uint8_t SSM = 0x57;
    static constexpr uint8_t SHS = 0x58;
    static constexpr uint8_t SVS = 0x59;
    static constexpr uint8_t SDP = 0x5F;
    static constexpr uint8_t ORN = 0x63;
    static constexpr uint8_t DEL = 0x7F;
    static constexpr uint8_t BKF = 0x80;
    static constexpr uint8_t RDF = 0x81;
    static constexpr uint8_t GRF = 0x82;
    static constexpr uint8_t YLF = 0x83;
    static constexpr uint8_t BLF = 0x84;
    static constexpr uint8_t MGF = 0x85;
    static constexpr uint8_t CNF = 0x86;
    static constexpr uint8_t WHF = 0x87;
    static constexpr uint8_t SSZ = 0x88;
    static constexpr uint8_t MSZ = 0x89;
    static constexpr uint8_t NSZ = 0x8A;
    static constexpr uint8_t SZX = 0x8B;
    static constexpr uint8_t COL = 0x90;
    static constexpr uint8_t FLC = 0x91;
    static constexpr uint8_t CDC = 0x92;
    static constexpr uint8_t POL = 0x93;
    static constexpr uint8_t WMM = 0x94;
    static constexpr uint8_t MACRO = 0x95;
    static constexpr uint8_t HLC = 0x97;
    static constexpr uint8_t RPC = 0x98;
    static constexpr uint8_t SPL = 0x99;
    static constexpr uint8_t STL = 0x9A;
    static constexpr uint8_t CSI = 0x9B;
    static constexpr uint8_t TIME = 0x9D;

}