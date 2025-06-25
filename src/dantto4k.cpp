#include <iostream>
#include "stream.h"
#include "remuxerHandler.h"
#include "config.h"
#include "dantto4k.h"
#ifdef _WIN32
#include "logger.h"
#include <dbghelp.h>
#include "bonTuner.h"
#endif
#include "aribUtil.h"

MmtTlv::MmtTlvDemuxer demuxer;
std::vector<uint8_t> output;
RemuxerHandler handler(demuxer, output);

#ifdef _WIN32
CBonTuner bonTuner;
HINSTANCE hDantto4kModule = nullptr;

std::string getConfigFilePath(void* hModule) {
    char g_IniFilePath[_MAX_FNAME];
    GetModuleFileNameA((HMODULE)hModule, g_IniFilePath, _MAX_FNAME);

    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    _splitpath_s(g_IniFilePath, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), NULL, NULL);
    sprintf(g_IniFilePath, "%s%s%s.ini\0", drive, dir, fname);

    return g_IniFilePath;
}

extern "C" __declspec(dllexport) IBonDriver* CreateBonDriver() {
    try {
        std::string path = getConfigFilePath(hDantto4kModule);
        config = loadConfig(path);

        demuxer.setDemuxerHandler(handler);
        demuxer.setSmartCardReaderName(config.smartCardReaderName);
        demuxer.init();

        bonTuner.init();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
    return &bonTuner;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    {
        hDantto4kModule = hModule;
        break;
    }
    }

    return true;
}

void PrintStackTrace(CONTEXT* context) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymInitialize(process, NULL, TRUE);

    STACKFRAME64 stackFrame = { 0 };
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;

    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Rsp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;

    while (StackWalk64(machineType, process, thread, &stackFrame, context, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        DWORD64 address = stackFrame.AddrPC.Offset;

        DWORD64 baseAddress = SymGetModuleBase64(process, address);
        if (baseAddress) {
            char moduleName[MAX_PATH];
            if (GetModuleFileNameA((HMODULE)baseAddress, moduleName, sizeof(moduleName))) {
                std::string fullPath(moduleName);
                size_t pos = fullPath.find_last_of("\\/");
                std::string fileName = (pos == std::string::npos) ? fullPath : fullPath.substr(pos + 1);

                SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = MAX_SYM_NAME;

                if (SymFromAddr(process, address, NULL, symbol)) {
                    DWORD64 offset = address - baseAddress;
                    log_debug("%s+0x%llx", fileName.c_str(), offset);
                }
                else {
                    log_debug("0x%llx", address);
                }

                free(symbol);
            }
        }
    }

    SymCleanup(process);
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    log_debug("An exception occurred!");
    PrintStackTrace(exceptionInfo->ContextRecord);
    return EXCEPTION_EXECUTE_HANDLER;
}

static MmtTlv::DemuxStatus demuxWithHandler(MmtTlv::Common::ReadStream& input) {
    __try {
        return demuxer.demux(input);
    }
    __except (ExceptionHandler(GetExceptionInformation())) {
    }

    return MmtTlv::DemuxStatus::Error;
}

#endif

size_t getLeftBytes(std::istream* inputStream) {
    std::streampos currentPos = inputStream->tellg();

    inputStream->seekg(0, std::ios::end);
    std::streampos fileSize = inputStream->tellg();

    inputStream->seekg(currentPos);

    return static_cast<size_t>(fileSize - currentPos);
}

void printReaderList() {
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

int main(int argc, char* argv[]) {
    constexpr size_t chunkSize = 1024 * 1024 * 5; // 5MB
    auto start = std::chrono::high_resolution_clock::now();

    std::string inputPath, outputPath;
    bool useStdin = false, useStdout = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--disableADTSConversion") {
            config.disableADTSConversion = true;
        }
        else if (arg.find("--smartCardReaderName=") == 0) {
            config.smartCardReaderName = arg.substr(std::string("--smartCardReaderName=").length());
        }
        else if (arg == "--listSmartCardReader") {
            printReaderList();
            return 1;
        }
        else {
            if (inputPath == "") {
                inputPath = arg;
                if (inputPath == "-") {
                    useStdin = true;
                }
            }
            else if (outputPath == "") {
                outputPath = arg;
                if (outputPath == "-") {
                    useStdout = true;
                }
            }
        }
    }

    if (inputPath == "" || outputPath == "") {
        std::cerr << "dantto4k.exe <input.mmts> <output.ts> [options]" << std::endl;
        std::cerr << "\t'-' can be used instead of a file path to enable piping via stdin or stdout." << std::endl;
        std::cerr << "options:" << std::endl;
        std::cerr << "\t--disableADTSConversion: Use the raw LATM format without converting to ADTS." << std::endl;
        std::cerr << "\t--listSmartCardReader: List the available smart card readers." << std::endl;
        std::cerr << "\t--smartCardReaderName=<name>: Set the smart card reader to use." << std::endl;
        return 1;
    }

    if (inputPath != "-" && inputPath == outputPath) {
        std::cerr << "Input and output paths cannot be the same." << std::endl;
        return 1;
    }

    std::unique_ptr<std::istream> inputStream;
    std::unique_ptr<std::ifstream> inputFs;
    if (useStdin) {
        inputStream.reset(&std::cin);
    }
    else {
        inputFs = std::make_unique<std::ifstream>(inputPath, std::ios::binary);
        if (!inputFs->is_open()) {
            std::cerr << "Unable to open input file: " << inputPath << std::endl;
            return 1;
        }
        inputStream = std::move(inputFs);
    }

    std::unique_ptr<std::ofstream> outputFs;
    if (!useStdout) {
        outputFs = std::make_unique<std::ofstream>(outputPath, std::ios::binary);
        if (!outputFs->is_open()) {
            std::cerr << "Unable to open output file: " << outputPath << std::endl;
            return 1;
        }
    }

    demuxer.setDemuxerHandler(handler);
    demuxer.setSmartCardReaderName(config.smartCardReaderName);
    demuxer.init();

    std::vector<uint8_t> inputBuffer;
    inputBuffer.reserve(chunkSize * 2);
    while (true) {
        if (!useStdin && inputStream->eof()) {
            break;
        }

        if (inputBuffer.size() < chunkSize) {
            size_t oldSize = inputBuffer.size();
            inputBuffer.resize(oldSize + chunkSize);
            inputStream->read(reinterpret_cast<char*>(inputBuffer.data() + oldSize), chunkSize);
            inputBuffer.resize(oldSize + inputStream->gcount());
        }

        MmtTlv::Common::ReadStream stream(inputBuffer);
        while (!stream.isEof()) {
            MmtTlv::DemuxStatus status;
#ifdef _WIN32
            status = demuxWithHandler(stream);
#else
            status = demuxer.demux(stream);
#endif

            if (status == MmtTlv::DemuxStatus::NotEnoughBuffer) {
                break;
            }
        }

        inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + (inputBuffer.size() - stream.leftBytes()));

        if (useStdout) {
            std::cout.write(reinterpret_cast<const char*>(output.data()), output.size());
        }
        else {
            outputFs->write(reinterpret_cast<const char*>(output.data()), output.size());
        }
        
        output.clear();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    demuxer.printStatistics();
    demuxer.clear();
    demuxer.release();

    std::cerr << "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
    return 0;
}
