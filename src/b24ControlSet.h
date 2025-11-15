#pragma once
#include <cstdint>

namespace B24ControlSet {

constexpr uint8_t NUL = 0x00;
constexpr uint8_t BEL = 0x07;
constexpr uint8_t APB = 0x08;
constexpr uint8_t APF = 0x09;
constexpr uint8_t APD = 0x0A;
constexpr uint8_t APU = 0x0B;
constexpr uint8_t CS = 0x0C;
constexpr uint8_t APR = 0x0D;
constexpr uint8_t LS1 = 0x0E;
constexpr uint8_t LS0 = 0x0F;
constexpr uint8_t PAPF = 0x16;
constexpr uint8_t CAN = 0x18;
constexpr uint8_t SS2 = 0x19;
constexpr uint8_t ESC = 0x1B;
constexpr uint8_t APS = 0x1C;
constexpr uint8_t SS3 = 0x1D;
constexpr uint8_t RS = 0x1E;
constexpr uint8_t US = 0x1F;
constexpr uint8_t SP = 0x20;
constexpr uint8_t SWF = 0x53;
constexpr uint8_t SDF = 0x56;
constexpr uint8_t SSM = 0x57;
constexpr uint8_t SHS = 0x58;
constexpr uint8_t SVS = 0x59;
constexpr uint8_t SDP = 0x5F;
constexpr uint8_t ORN = 0x63;
constexpr uint8_t DEL = 0x7F;
constexpr uint8_t BKF = 0x80;
constexpr uint8_t RDF = 0x81;
constexpr uint8_t GRF = 0x82;
constexpr uint8_t YLF = 0x83;
constexpr uint8_t BLF = 0x84;
constexpr uint8_t MGF = 0x85;
constexpr uint8_t CNF = 0x86;
constexpr uint8_t WHF = 0x87;
constexpr uint8_t SSZ = 0x88;
constexpr uint8_t MSZ = 0x89;
constexpr uint8_t NSZ = 0x8A;
constexpr uint8_t SZX = 0x8B;
constexpr uint8_t COL = 0x90;
constexpr uint8_t FLC = 0x91;
constexpr uint8_t CDC = 0x92;
constexpr uint8_t POL = 0x93;
constexpr uint8_t WMM = 0x94;
constexpr uint8_t MACRO = 0x95;
constexpr uint8_t HLC = 0x97;
constexpr uint8_t RPC = 0x98;
constexpr uint8_t SPL = 0x99;
constexpr uint8_t STL = 0x9A;
constexpr uint8_t CSI = 0x9B;
constexpr uint8_t TIME = 0x9D;
constexpr uint8_t ACPS = 0x61;

}