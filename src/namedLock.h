#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <string>
#include <stdexcept>

class NamedLock {
public:
    NamedLock(const std::string& name) : name(name), locked(false) {
#ifdef _WIN32
        int wlen = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
        wchar_t* wname = new wchar_t[wlen];
        MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wname, wlen);

        mutex = CreateMutexW(nullptr, FALSE, wname);
        delete[] wname;

        if (mutex == nullptr) {
            throw std::runtime_error("Failed to create mutex");
        }
#endif
    }

    ~NamedLock() {
#ifdef _WIN32
        if (mutex) {
            CloseHandle(mutex);
            mutex = nullptr;
        }
#endif
    }

    void lock() {
        if (locked) {
            return;
        }
#ifdef _WIN32
        DWORD res = WaitForSingleObject(mutex, INFINITE);
        if (res != WAIT_OBJECT_0) {
            throw std::runtime_error("Failed to lock mutex");
        }
#endif
        locked = true;
    }

    void unlock() {
        if (!locked) {
            return;
        }
#ifdef _WIN32
        if (!ReleaseMutex(mutex)) {
            throw std::runtime_error("Failed to unlock mutex");
        }
#endif
        locked = false;
    }

private:
    std::string name;
    bool locked;

#ifdef _WIN32
    HANDLE mutex = nullptr;
#endif
};

extern NamedLock ipcLock;