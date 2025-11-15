#include <iostream>
#include "cxxopts.hpp"
#include "stream.h"
#include "remuxerHandler.h"
#include "config.h"
#include "mmtTlvDemuxer.h"
#include "aribUtil.h"
#include "casProxyClient.h"
#include "acasHandler.h"
#include "smartCard.h"

namespace {

struct Args {
    std::string input;
    std::string output;
    std::string casProxyHost;
    uint16_t casProxyPort{0};
    std::string smartCardReaderName;
    std::string customWinscardDLL;
    bool disableADTSConversion{false};
    bool listSmartCardReader{false};
};


Args parseArguments(int argc, char* argv[]) {
    Args args;

    try {
        cxxopts::Options options("dantto4k", "MMT/TLV to MPEG-2 TS Converter (https://github.com/nekohkr/dantto4k)");

        options.add_options()
            ("input", "Input file ('-' for stdin)", cxxopts::value<std::string>()->default_value(""))
            ("output", "Output file ('-' for stdout)", cxxopts::value<std::string>()->default_value(""))
            ("listSmartCardReader", "List available smart card readers", cxxopts::value<bool>()->default_value("false"))
            ("casProxyServer", "Specify the address of a CasProxyServer", cxxopts::value<std::string>()->default_value(""))
            ("smartCardReaderName", "Specify the smart card reader to use", cxxopts::value<std::string>()->default_value(""))
#ifdef WIN32
            ("customWinscardDLL", "Specify the path to a custom winscard.dll", cxxopts::value<std::string>()->default_value(""))
#endif
            ("disableADTSConversion", "Disable ADTS conversion", cxxopts::value<bool>()->default_value("false"))
            ("help", "Show help");

        options.parse_positional({ "input", "output" });
        options.positional_help("input output ('-' for stdin/stdout)");
        auto result = options.parse(argc, argv);

        if (result.count("help") || (!result.count("listSmartCardReader") && (!result.count("input") || !result.count("output")))) {
            std::cout << options.help() << std::endl;
            std::exit(1);
        }

        args.input = result["input"].as<std::string>();
        args.output = result["output"].as<std::string>();

        std::string casProxyServer = result["casProxyServer"].as<std::string>();
        if (!casProxyServer.empty()) {
            auto parsed = casproxy::parseAddress(casProxyServer);
            if (!parsed) {
                std::cerr << "Invalid CasProxyServer address" << std::endl;
                std::exit(1);
            }
            args.casProxyHost = parsed->first;
            args.casProxyPort = parsed->second;
        }

        args.smartCardReaderName = result["smartCardReaderName"].as<std::string>();
        args.listSmartCardReader = result["listSmartCardReader"].as<bool>();
#ifdef WIN32
        args.customWinscardDLL = result["customWinscardDLL"].as<std::string>();
#endif

        if (!args.listSmartCardReader) {
            if (!result.count("input") || !result.count("output")) {
                std::cerr << "input and output arguments are required.\n\n"
                    << options.help() << '\n';
                std::exit(1);
            }

            if (args.input != "-" && args.input == args.output) {
                std::cerr << "Input and output paths cannot be the same.\n\n"
                    << options.help() << '\n';
                std::exit(1);
            }
        }

        args.disableADTSConversion = result["disableADTSConversion"].as<bool>();
    }
    catch (const cxxopts::exceptions::exception& e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    return args;
}

void printReaderList(const Args& args) {
    try {
        std::unique_ptr<ISmartCard> smartCard;
        if (args.casProxyHost.empty()) {
            smartCard = std::make_unique<LocalSmartCard>();
        }
        else {
            smartCard = std::make_unique<RemoteSmartCard>(args.casProxyHost, args.casProxyPort);
        }

        smartCard->init();
        auto list = smartCard->getReaders();

        for (const auto& reader : list) {
            std::cerr << " - " << reader << std::endl;
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }

}

}

int main(int argc, char* argv[]) {
    const auto startTime = std::chrono::high_resolution_clock::now();
    constexpr size_t chunkSize = 1024 * 1024 * 5; // 5MB

    Args args = parseArguments(argc, argv);
#ifdef WIN32
    config.customWinscardDLL = args.customWinscardDLL;
#endif
    config.disableADTSConversion = args.disableADTSConversion;

    bool useStdin = (args.input == "-");
    bool useStdout = (args.output == "-");

    if (args.listSmartCardReader) {
        printReaderList(args);
        return 0;
    }

    std::istream* inputStream;
    std::unique_ptr<std::ifstream> inputFs;
    if (useStdin) {
        inputStream = &std::cin;
    }
    else {
        inputFs = std::make_unique<std::ifstream>(args.input, std::ios::binary);
        if (!inputFs->is_open()) {
            std::cerr << "Unable to open input file: " << args.input << std::endl;
            return 1;
        }
        inputStream = inputFs.get();
    }

    std::ostream* outputStream;
    std::unique_ptr<std::ofstream> outputFs;
    if (useStdout) {
        outputStream = &std::cout;
    }
    else {
        outputFs = std::make_unique<std::ofstream>(args.output, std::ios::binary);
        if (!outputFs->is_open()) {
            std::cerr << "Unable to open output file: " << args.output << std::endl;
            return 1;
        }
        outputStream = outputFs.get();
    }

    MmtTlv::MmtTlvDemuxer demuxer;
    RemuxerHandler handler(demuxer);
    handler.setOutputCallback([&](const uint8_t* data, size_t size) {
        assert(size == 188);
        outputStream->write(reinterpret_cast<const char*>(data), size);
    });
    demuxer.setDemuxerHandler(handler);

    try {
        // Create ACAS handler and initialize the smart card
        std::unique_ptr<AcasHandler> acasHandler = std::make_unique<AcasHandler>();
        std::unique_ptr<ISmartCard> smartCard;
        if (args.casProxyHost.empty()) {
            smartCard = std::make_unique<LocalSmartCard>();
        }
        else {
            smartCard = std::make_unique<RemoteSmartCard>(args.casProxyHost, args.casProxyPort);
        }
        
        smartCard->setSmartCardReaderName(args.smartCardReaderName);
        acasHandler->setSmartCard(std::move(smartCard));
        demuxer.setCasHandler(std::move(acasHandler));
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::vector<uint8_t> inputBuffer;
    inputBuffer.reserve(chunkSize * 2);
    while (true) {
        if (!useStdin && inputStream->eof()) {
            break;
        }

        if (inputBuffer.size() < chunkSize) {
            std::vector<uint8_t> buffer(chunkSize);
            inputStream->read(reinterpret_cast<char*>(buffer.data()), chunkSize);
            std::streamsize bytes = inputStream->gcount();

            if (bytes > 0) {
                inputBuffer.insert(inputBuffer.end(), buffer.begin(), buffer.begin() + bytes);
            }
        }

        MmtTlv::Common::ReadStream stream(inputBuffer);
        while (!stream.isEof()) {
            MmtTlv::DemuxStatus status = demuxer.demux(stream);

            if (status == MmtTlv::DemuxStatus::NotEnoughBuffer) {
                break;
            }
        }

        inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + (inputBuffer.size() - stream.leftBytes()));
    }

    demuxer.printStatistics();
    demuxer.clear();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cerr << "Elapsed time: " << elapsed.count() << " seconds\n";

    return 0;
}
