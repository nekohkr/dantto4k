#include "mmtTlvDemuxer.h"
#include "bonDriverContext.h"

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
    try {
        std::string path = getConfigFilePath(::hModule);
        config = loadConfig(path);


        g_bonDriverContext.handler.setOutputCallback([&](const uint8_t* data, size_t size) {
            assert(size == 188);
            g_bonDriverContext.remuxOutput.insert(g_bonDriverContext.remuxOutput.end(), data, data + size);
        });
        g_bonDriverContext.demuxer.setDemuxerHandler(g_bonDriverContext.handler);
        g_bonDriverContext.demuxer.setAcasServerUrl(config.acasServerUrl);
        g_bonDriverContext.demuxer.setSmartCardReaderName(config.smartCardReaderName);
        g_bonDriverContext.demuxer.init();
        g_bonDriverContext.bonTuner.init();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }

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



