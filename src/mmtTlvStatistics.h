#pragma once
#include <iomanip>
#include <sstream>

namespace MmtTlv {

class mmtTlvStatistics {
public:
	uint64_t tlvPacketCount{0};
	uint64_t tlvIpv4PacketCount{0};
	uint64_t tlvIpv6PacketCount{0};
	uint64_t tlvHeaderCompressedIpPacketCount{0};
	uint64_t tlvTransmissionControlSignalPacketCount{0};
	uint64_t tlvNullPacketCount{0};
	uint64_t tlvUndefinedCount{0};

	class MmtStat {
	public:
		MmtStat(uint16_t packetId) : packetId(packetId) {}
		uint16_t packetId{0};
		uint32_t lastPacketSequenceNumber{0};
		uint32_t assetType{0};
		uint64_t count{0};
		uint64_t drop{0};
		uint8_t videoResolution{0};
		uint8_t videoAspectRatio{0};
		uint8_t audioComponentType{0};
		uint8_t audioSamplingRate{0};
		
		void print() {
			std::ostringstream oss;
			oss << "0x" << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << packetId;

			std::string output;
			output = " - PacketId: " + oss.str() + ", ";

			if (assetType != 0) {
				if (assetType == AssetType::hev1) {
					output += "HEVC";
					int videoResolutionWidth = 0, videoResolutionHeight = 0;

					if (videoResolution == 1) {
						videoResolutionHeight = 180;
					}
					else if (videoResolution == 2) {
						videoResolutionHeight = 240;
					}
					else if (videoResolution == 3) {
						videoResolutionHeight = 480;
					}
					else if (videoResolution == 4) {
						videoResolutionHeight = 720;
					}
					else if (videoResolution == 5) {
						videoResolutionHeight = 1080;
					}
					else if (videoResolution == 6) {
						videoResolutionHeight = 2160;
					}
					else if (videoResolution == 7) {
						videoResolutionHeight = 4320;
					}
				
					videoResolutionWidth = static_cast<int>(videoResolutionHeight * ((videoAspectRatio == 2 || videoAspectRatio == 3) ? 16.f/9 : 4.f/3));
					output += "(" + std::to_string(videoResolutionWidth) + "x" + std::to_string(videoResolutionHeight) + "), ";
				}
				else if (assetType == AssetType::mp4a) {
					output += "Audio";
					uint8_t audioMode = audioComponentType & 0b00011111;
					if (audioMode == 0b00001) {
						output += "(single mono, ";
					}
					else if (audioMode == 0b00010) {
						output += "(dual mono, ";
					}
					else if (audioMode == 0b00011) {
						output += "(stereo, ";
					}
					else if (audioMode == 0b00100) {
						output += "(2/1, ";
					}
					else if (audioMode == 0b00101) {
						output += "(3ch, ";
					}
					else if (audioMode == 0b00110) {
						output += "(2/2, ";
					}
					else if (audioMode == 0b00111 ) {
						output += "(4ch, ";
					}
					else if (audioMode == 0b010000) {
						output += "(5ch, ";
					}
					else if (audioMode == 0b01001) {
						output += "(5.1ch, ";
					}
					else if (audioMode == 0b01010) {
						output += "(3/3.1, ";
					}
					else if (audioMode == 0b01011) {
						output += "(6.1ch, ";
					}
					else if (audioMode == 0b01100) {
						output += "(7.1ch, ";
					}
					else if (audioMode == 0b01101) {
						output += "(7.1ch, ";
					}
					else if (audioMode == 0b01110) {
						output += "(7.1ch, ";
					}
					else if (audioMode == 0b01111) {
						output += "(7.1ch, ";
					}
					else if (audioMode == 0b10000) {
						output += "(10.2ch, ";
					}
					else if (audioMode == 0b10001) {
						output += "(22.2ch, ";
					}

					if (audioSamplingRate == 0b001) {
						output += "16kHz), ";
					}
					else if (audioSamplingRate == 0b010) {
						output += "22.05kHz), ";
					}
					else if (audioSamplingRate == 0b011) {
						output += "24kHz), ";
					}
					else if (audioSamplingRate == 0b101) {
						output += "32kHz), ";
					}
					else if (audioSamplingRate == 0b110) {
						output += "44.1kHz), ";
					}
					else if (audioSamplingRate == 0b111) {
						output += "48kHz), ";
					}
				}
				else if (assetType == AssetType::stpp) {
					output += "Closed Caption, ";
				}
				else if (assetType == AssetType::aapp) {
					output += "Application, ";
				}
			}
			output += "Count: " + std::to_string(count) + ", ";
			output += "Drop: " + std::to_string(drop);
			std::cerr << output << std::endl;
		}
	};

	std::map<uint16_t, std::shared_ptr<MmtStat>> mapMmtStat;
	std::shared_ptr<MmtStat> getMmtStat(uint16_t packetId) {
		if (mapMmtStat.find(packetId) == mapMmtStat.end()) {
			auto mmtStat = std::make_shared<MmtStat>(packetId);
			mapMmtStat[packetId] = mmtStat;
			return mmtStat;
		}
		else {
			return mapMmtStat[packetId];
		}
	}

	void clear() {
		mapMmtStat.clear();
	}

	void print() {
		std::cerr << "TLV Packet: " << std::to_string(tlvPacketCount) << std::endl;
		std::cerr << " - IPv4Packet: " << std::to_string(tlvIpv4PacketCount) << std::endl;
		std::cerr << " - IPv6Packet: " << std::to_string(tlvIpv6PacketCount) << std::endl;
		std::cerr << " - HeaderCompressedIpPacket: " << std::to_string(tlvHeaderCompressedIpPacketCount) << std::endl;
		std::cerr << " - TransmissionControlSignalPacket: " << std::to_string(tlvTransmissionControlSignalPacketCount) << std::endl;
		std::cerr << " - NullPacket: " << std::to_string(tlvNullPacketCount) << std::endl;
		std::cerr << " - Undefined: " << std::to_string(tlvUndefinedCount) << std::endl;
		std::cerr << "MMT:" << std::endl;

		for (const auto& mmtStat : mapMmtStat) {
			mmtStat.second->print();
		}
	}
};

}