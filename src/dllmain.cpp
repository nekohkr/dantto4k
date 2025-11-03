#include "mmtTlvDemuxer.h"
#include "bonDriverContext.h"
#include "acasHandler.h"

BonDriverContext g_bonDriverContext;

namespace {

HINSTANCE hModule = nullptr;

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

}

extern "C" __declspec(dllexport) IBonDriver* CreateBonDriver() {
    std::string path = getConfigFilePath(::hModule);
    config = loadConfig(path);

    {
        std::unique_ptr<AcasHandler> acasHandler = std::make_unique<AcasHandler>();
        std::unique_ptr<ISmartCard> smartCard;
        if (config.casProxyServer.empty()) {
            smartCard = std::make_unique<LocalSmartCard>();
        }
        else {
            auto parsed = casproxy::parseAddress(config.casProxyServer);
            if (!parsed) {
                std::cerr << "Invalid CasProxyServer address" << std::endl;
                std::exit(1);
            }
            smartCard = std::make_unique<RemoteSmartCard>(parsed->first, parsed->second);
        }

        smartCard->setSmartCardReaderName(config.smartCardReaderName);

        try {
            smartCard->init();
            smartCard->connect();
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
        }
        acasHandler->setSmartCard(std::move(smartCard));
        g_bonDriverContext.demuxer.setCasHandler(std::move(acasHandler));
    }

    g_bonDriverContext.handler.setOutputCallback([&](const uint8_t* data, size_t size) {
        assert(size == 188);
        g_bonDriverContext.remuxOutput.insert(g_bonDriverContext.remuxOutput.end(), data, data + size);
    });
    g_bonDriverContext.demuxer.setDemuxerHandler(g_bonDriverContext.handler);
    g_bonDriverContext.bonTuner.init();

    return &g_bonDriverContext.bonTuner;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        ::hModule = hModule;
        break;
    }

    return true;
}



