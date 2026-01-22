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
#include "bufferedOutput.h"
#include "progressReporter.h"

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
    bool noProgress{false};
    bool noStats{false};
};

Args parseArguments(int argc, char* argv[]) {
    Args args;

    try {
        cxxopts::Options options("dantto4k", "MMT/TLV to MPEG-2 TS Converter (https://github.com/nekohkr/dantto4k)");

        options.add_options()
            ("input", "Input file ('-' for stdin)", cxxopts::value<std::string>()->default_value(""))
            ("output", "Output file ('-' for stdout)", cxxopts::value<std::string>()->default_value(""))
            ("listSmartCardReader", "List available smart card readers", cxxopts::value<bool>()->default_value("false"))
            ("casProxyServer", "Specify the address of a CasProxyServer", cxxopts::value<std::string>())
            ("smartCardReaderName", "Specify the smart card reader to use", cxxopts::value<std::string>())
#ifdef WIN32
            ("customWinscardDLL", "Specify the path to a winscard.dll", cxxopts::value<std::string>())
#endif
            ("disableADTSConversion", "Disable ADTS conversion", cxxopts::value<bool>()->default_value("false"))
            ("no-progress", "Disable progress display", cxxopts::value<bool>()->default_value("false"))
            ("no-stats", "Disable packet statistics", cxxopts::value<bool>()->default_value("false"))
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

        if (result["casProxyServer"].count()) {
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
        }

        if (result["smartCardReaderName"].count()) {
            args.smartCardReaderName = result["smartCardReaderName"].as<std::string>();
        }
        if (result["listSmartCardReader"].count()) {
            args.listSmartCardReader = result["listSmartCardReader"].as<bool>();
        }
#ifdef WIN32
        if (result["customWinscardDLL"].count()) {
            args.customWinscardDLL = result["customWinscardDLL"].as<std::string>();
        }
#endif

        if (!args.listSmartCardReader) {
            if (!result.count("input") || !result.count("output")) {
                std::cerr << "input and output arguments are required" << std::endl;
                std::exit(1);
            }

            if (args.input != "-" && args.input == args.output) {
                std::cerr << "Input and output paths cannot be the same" << std::endl;
                std::exit(1);
            }
        }

        if (result["disableADTSConversion"].count()) {
            args.disableADTSConversion = result["disableADTSConversion"].as<bool>();
        }
        if (result["no-progress"].count()) {
            args.noProgress = result["no-progress"].as<bool>();
        }
        if (result["no-stats"].count()) {
            args.noStats = result["no-stats"].as<bool>();
        }

        // Disable progress and stats when using stdin/stdout
        if (args.input == "-" || args.output == "-") {
            args.noProgress = true;
            args.noStats = true;
        }
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

    uint64_t fileSize = 0;
    if (!useStdin) {
        auto currentPos = inputFs->tellg();
        inputFs->seekg(0, std::ios::end);
        fileSize = inputFs->tellg();
        inputFs->seekg(currentPos);
    }
    ProgressReporter progressReporter(fileSize, !args.noProgress);

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
    std::unique_ptr<BufferedOutput> bufferedOutput;

    if (useStdout) {
        handler.setOutputCallback([&](const uint8_t* data, size_t size) {
            assert(size == 188);
            outputStream->write(reinterpret_cast<const char*>(data), size);
        });
    }
    else {
        bufferedOutput = std::make_unique<BufferedOutput>(*outputStream);
        handler.setOutputCallback([&, bo = bufferedOutput.get()](const uint8_t* data, size_t size) {
            assert(size == 188);
            bo->write(data, size);
        });
    }

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

        size_t oldSize = inputBuffer.size();
        size_t bytesToRead = chunkSize;
        if (oldSize < chunkSize) {
            inputBuffer.resize(oldSize + bytesToRead);
            inputStream->read(reinterpret_cast<char*>(inputBuffer.data() + oldSize), chunkSize);
            std::streamsize bytesRead = inputStream->gcount();
            inputBuffer.resize(oldSize + bytesRead);
        }

        MmtTlv::Common::ReadStream stream(inputBuffer);
        while (!stream.isEof()) {
            MmtTlv::DemuxStatus status = demuxer.demux(stream);

            if (status == MmtTlv::DemuxStatus::NotEnoughBuffer) {
                break;
            }
        }
        
        auto consumed = inputBuffer.size() - stream.leftBytes();
        if (consumed > 0) {
            progressReporter.update(consumed);
        }
        inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + consumed);
    }

    progressReporter.finish();
    if (!args.noStats) {
        demuxer.printStatistics();
    }
    demuxer.clear();

    return 0;
}
