#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>

namespace MmtTlv {

class MmtTlvStatistics {
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
		std::string name;
		uint32_t lastPacketSequenceNumber{0};
		uint32_t assetType{0};
		uint64_t count{0};
		uint64_t drop{0};
		uint8_t videoResolution{0};
		uint8_t videoAspectRatio{0};
		uint8_t audioComponentType{0};
		uint8_t audioSamplingRate{0};

		void setName(const std::string& name) {
			this->name = name;
		}
		
		std::string getName() const {
			if (assetType != 0) {
				switch (assetType) {
				case AssetType::hev1:
					return "HEVC";
				case AssetType::mp4a:
					return "Audio";
				case AssetType::stpp:
					return "Closed Caption";
				case AssetType::aapp:
					return "Application";
				default:
					return "Unknown";
				}
			}
			else {
				return name;
			}
		}

		std::string getVideoResolution() const {
			int videoResolutionWidth = 0, videoResolutionHeight = 0;
			
			switch (videoResolution) {
			case 1:
				videoResolutionHeight = 180;
				break;
			case 2:
				videoResolutionHeight = 240;
				break;
			case 3:
				videoResolutionHeight = 480;
				break;
			case 4:
				videoResolutionHeight = 720;
				break;
			case 5:
				videoResolutionHeight = 1080;
				break;
			case 6:
				videoResolutionHeight = 2160;
				break;
			case 7:
				videoResolutionHeight = 4320;
				break;
			}
				
			videoResolutionWidth = static_cast<int>(videoResolutionHeight * ((videoAspectRatio == 2 || videoAspectRatio == 3) ? 16.f/9 : 4.f/3));
			return std::to_string(videoResolutionWidth) + "x" + std::to_string(videoResolutionHeight);;
		}
		
		std::string getAudioMode() const {
			uint8_t audioMode = audioComponentType & 0b00011111;

			switch (audioMode) {
			case 0b00001:
				return "single mono";
			case 0b00010:
				return "dual mono";
			case 0b00011:
				return "stereo";
			case 0b00100:
				return "2/1";
			case 0b00101:
				return "3ch";
			case 0b00110:
				return "2/2";
			case 0b00111:
				return "4ch";
			case 0b01000:
				return "5ch";
			case 0b01001:
				return "5.1ch";
			case 0b01010:
				return "3/3.1";
			case 0b01011:
				return "6.1ch";
			case 0b01100:
				return "7.1ch";
			case 0b01101:
				return "7.1ch";
			case 0b01110:
				return "7.1ch";
			case 0b01111:
				return "7.1ch";
			case 0b10000:
				return "10.2ch";
			case 0b10001:
				return "22.2ch";
			}

			return "Unknown";
		}
		
		std::string getAudioSamplingRate() const {
			switch (audioSamplingRate) {
			case 0b001:
				return "16kHz";
			case 0b010:
				return "22.05kHz";
			case 0b011:
				return "24kHz";
			case 0b101:
				return "32kHz";
			case 0b110:
				return "44.1kHz";
			case 0b111:
				return "48kHz";
			default:
				return "Unknown";
			}
		}

		void print() const {
			std::string output;
			std::ostringstream oss;

			oss << "0x" << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << packetId;

			output = " - PacketId: " + oss.str() + ", ";
			output += "Drop: " + std::to_string(drop) + ", ";
			output += "Count: " + std::to_string(count);

			std::string name = getName();
			if(name != "") {
				output += ", " + getName();
			}

			if (assetType != 0) {
				if (assetType == AssetType::hev1) {
					output += "(" + getVideoResolution() + ")";
				}
				else if (assetType == AssetType::mp4a) {
					output += "(" + getAudioMode() + ", " + getAudioSamplingRate() + ")";
				}
			}

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

	void print() const {
		std::cerr << "TLV Packet" << std::endl;
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