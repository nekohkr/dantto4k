#include <iostream>
#include "cxxopts.hpp"
#include "stream.h"
#include "remuxerHandler.h"
#include "config.h"
#include "mmtTlvDemuxer.h"
#include "aribUtil.h"

namespace {

struct Args {
    std::string input;
    std::string output;
    std::string acasServerUrl;
    std::string smartCardReaderName;
    bool disableADTSConversion = false;
    bool listSmartCardReader = false;
};

struct Args parseArguments(int argc, char* argv[]) {
    Args args;

    try {
        cxxopts::Options options("dantto4k", "");

        options.add_options()
            ("input", "Input file ('-' for stdin)", cxxopts::value<std::string>())
            ("output", "Output file ('-' for stdout)", cxxopts::value<std::string>())
            ("acasServerUrl", "Use the ACAS server instead of the local smartcard", cxxopts::value<std::string>()->default_value(""))
            ("smartCardReaderName", "Set the smart card reader to use", cxxopts::value<std::string>()->default_value(""))
            ("disableADTSConversion", "Disable ADTS conversion", cxxopts::value<bool>()->default_value("false"))
            ("listSmartCardReader", "List available smart card readers", cxxopts::value<bool>()->default_value("false"))
            ("help", "Show help");

        options.parse_positional({ "input", "output" });
        options.positional_help("input output");
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            std::exit(1);
        }

        args.input = result["input"].as<std::string>();
        args.output = result["output"].as<std::string>();
        args.acasServerUrl = result["acasServerUrl"].as<std::string>();
        args.disableADTSConversion = result["disableADTSConversion"].as<bool>();
        args.listSmartCardReader = result["listSmartCardReader"].as<bool>();

        if (!args.listSmartCardReader) {
            if (!result.count("input") || !result.count("output")) {
                std::cerr << "Error: input and output arguments are required.\n\n"
                    << options.help() << '\n';
                std::exit(1);
            }

            if (args.input != "-" && args.input == args.output) {
                std::cerr << "Error: Input and output paths cannot be the same.\n\n"
                    << options.help() << '\n';
                std::exit(1);
            }
        }
    }
    catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    return args;
}

void printReaderList() {
    if (config.acasServerUrl.empty()) {
        SCARDCONTEXT hContext;
        LONG result = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);

        DWORD readersSize = 0;
        SCardListReaders(hContext, nullptr, nullptr, &readersSize);
        if (result != SCARD_S_SUCCESS) {
            std::cerr << "Failed to get size of reader list. (result: " << result << ")" << std::endl;
            return;
        }

        std::vector<char> readersBuffer(readersSize);
        result = SCardListReaders(hContext, nullptr, readersBuffer.data(), &readersSize);
        if (result != SCARD_S_SUCCESS) {
            std::cerr << "Failed to get size of reader list. (result: " << result << ")" << std::endl;
            return;
        }

        const char* reader = readersBuffer.data();
        while (*reader != L'\0') {
            std::cerr << " - " << reader << std::endl;
            reader += strlen(reader) + 1;
        }

        SCardReleaseContext(hContext);
    }
    else {
        /*
        MmtTlv::Acas::AcasClient client(config.acasServerUrl);
        MmtTlv::Acas::GetSmartCardReadersRequest getReaderListRequest;
        auto res = client.sendRequest<MmtTlv::Acas::GetSmartCardReadersRequest>(getReaderListRequest);
        if (res.getStatus() != MmtTlv::Acas::AcasServerResponseStatus::Success) {
            std::cerr << "Failed to get smart card readers from ACAS server: " << res.getMessage() << std::endl;
            return;
        }

        for (const auto& reader : res.getData()) {
            std::cerr << " - " << reader << std::endl;
        }
        */
    }
}

}

int main(int argc, char* argv[]) {
    const auto startTime = std::chrono::high_resolution_clock::now();
    constexpr size_t chunkSize = 1024 * 1024 * 5; // 5MB

    Args args = parseArguments(argc, argv);
    config.acasServerUrl = args.acasServerUrl;
    config.smartCardReaderName = args.smartCardReaderName;
    config.disableADTSConversion = args.disableADTSConversion;

    bool useStdin = (args.input == "-");
    bool useStdout = (args.output == "-");

    if (args.listSmartCardReader) {
        printReaderList();
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
            std::cerr << "Error: Unable to open input file: " << args.input << std::endl;
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
            std::cerr << "Error: Unable to open output file: " << args.output << std::endl;
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
    demuxer.setAcasServerUrl(config.acasServerUrl);
    demuxer.setSmartCardReaderName(config.smartCardReaderName);
    demuxer.init();

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
    demuxer.release();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cerr << "Elapsed time: " << elapsed.count() << " seconds\n";

    return 0;
}
