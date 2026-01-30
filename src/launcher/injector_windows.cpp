// Copyright (c) 2024 QtMCP Contributors
// SPDX-License-Identifier: MIT

// Windows implementation of probe injection using CreateRemoteThread.
// This file is only compiled on Windows (see CMakeLists.txt).
//
// Injection pattern based on Pattern 4 from 01-RESEARCH.md:
// 1. Create process suspended
// 2. Allocate memory in target for DLL path
// 3. Write DLL path to target memory
// 4. Get address of LoadLibraryW in kernel32
// 5. Create remote thread to call LoadLibraryW with DLL path
// 6. Wait for injection thread to complete
// 7. Resume main thread
// 8. Optionally wait for process to exit

#include "injector.h"

#ifdef Q_OS_WIN

#include <QDir>
#include <QFileInfo>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <cstdio>

namespace {

/// @brief RAII wrapper for Windows HANDLE to ensure cleanup.
class HandleGuard {
public:
    explicit HandleGuard(HANDLE h = nullptr) : m_handle(h) {}
    ~HandleGuard() {
        if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
        }
    }
    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;
    HandleGuard(HandleGuard&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }
    HandleGuard& operator=(HandleGuard&& other) noexcept {
        if (this != &other) {
            if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_handle);
            }
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    HANDLE get() const { return m_handle; }
    HANDLE release() {
        HANDLE h = m_handle;
        m_handle = nullptr;
        return h;
    }
    bool valid() const { return m_handle && m_handle != INVALID_HANDLE_VALUE; }

private:
    HANDLE m_handle;
};

/// @brief Build a command line string for CreateProcessW.
/// @param executable The executable path.
/// @param args The command line arguments.
/// @return A properly quoted command line string.
std::wstring buildCommandLine(const QString& executable, const QStringList& args) {
    QString cmdLine;

    // Quote the executable path if it contains spaces
    if (executable.contains(QLatin1Char(' '))) {
        cmdLine = QStringLiteral("\"%1\"").arg(executable);
    } else {
        cmdLine = executable;
    }

    // Append arguments, quoting as needed
    for (const QString& arg : args) {
        cmdLine += QLatin1Char(' ');
        if (arg.contains(QLatin1Char(' ')) || arg.contains(QLatin1Char('"'))) {
            // Quote and escape embedded quotes
            QString escaped = arg;
            escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
            cmdLine += QStringLiteral("\"%1\"").arg(escaped);
        } else {
            cmdLine += arg;
        }
    }

    return cmdLine.toStdWString();
}

/// @brief Print a Windows error message.
/// @param prefix Message prefix.
/// @param errorCode The Windows error code (from GetLastError).
void printWindowsError(const char* prefix, DWORD errorCode) {
    wchar_t* msgBuf = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&msgBuf),
        0,
        nullptr);

    if (msgBuf) {
        fprintf(stderr, "[injector] %s: %ls (error %lu)\n", prefix, msgBuf, errorCode);
        LocalFree(msgBuf);
    } else {
        fprintf(stderr, "[injector] %s: error %lu\n", prefix, errorCode);
    }
}

}  // namespace

namespace qtmcp {

qint64 launchWithProbe(const LaunchOptions& options) {
    // 1. Set up environment for the target process
    // Pass port to probe via QTMCP_PORT environment variable

    // Get current environment and add QTMCP_PORT
    QString portStr = QString::number(options.port);
    if (!SetEnvironmentVariableW(L"QTMCP_PORT", portStr.toStdWString().c_str())) {
        if (!options.quiet) {
            printWindowsError("Failed to set QTMCP_PORT", GetLastError());
        }
        // Continue anyway - probe has default port
    }

    // 2. Build command line
    std::wstring cmdLine = buildCommandLine(options.targetExecutable, options.targetArgs);

    if (!options.quiet) {
        fprintf(stderr, "[injector] Command line: %ls\n", cmdLine.c_str());
    }

    // 3. Create target process in suspended state
    STARTUPINFOW si = {};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi = {};

    // Need a mutable copy of cmdLine for CreateProcessW
    std::vector<wchar_t> cmdLineBuf(cmdLine.begin(), cmdLine.end());
    cmdLineBuf.push_back(L'\0');

    BOOL createResult = CreateProcessW(
        nullptr,                    // Application name (use command line)
        cmdLineBuf.data(),          // Command line (mutable)
        nullptr,                    // Process security attributes
        nullptr,                    // Thread security attributes
        FALSE,                      // Inherit handles
        CREATE_SUSPENDED,           // Creation flags - start suspended
        nullptr,                    // Environment (inherit current)
        nullptr,                    // Current directory
        &si,                        // Startup info
        &pi                         // Process information
    );

    if (!createResult) {
        if (!options.quiet) {
            printWindowsError("CreateProcessW failed", GetLastError());
        }
        return -1;
    }

    // RAII guards for handles
    HandleGuard processHandle(pi.hProcess);
    HandleGuard threadHandle(pi.hThread);
    qint64 processId = static_cast<qint64>(pi.dwProcessId);

    if (!options.quiet) {
        fprintf(stderr, "[injector] Created process %lld (suspended)\n",
                static_cast<long long>(processId));
    }

    // 4. Allocate memory in target process for DLL path
    std::wstring dllPath = options.probePath.toStdWString();
    size_t dllPathSize = (dllPath.size() + 1) * sizeof(wchar_t);

    void* remoteMem = VirtualAllocEx(
        processHandle.get(),
        nullptr,
        dllPathSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!remoteMem) {
        if (!options.quiet) {
            printWindowsError("VirtualAllocEx failed", GetLastError());
        }
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    if (!options.quiet) {
        fprintf(stderr, "[injector] Allocated %zu bytes in target at %p\n",
                dllPathSize, remoteMem);
    }

    // 5. Write DLL path to target process memory
    SIZE_T bytesWritten = 0;
    BOOL writeResult = WriteProcessMemory(
        processHandle.get(),
        remoteMem,
        dllPath.c_str(),
        dllPathSize,
        &bytesWritten
    );

    if (!writeResult || bytesWritten != dllPathSize) {
        if (!options.quiet) {
            printWindowsError("WriteProcessMemory failed", GetLastError());
        }
        VirtualFreeEx(processHandle.get(), remoteMem, 0, MEM_RELEASE);
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    if (!options.quiet) {
        fprintf(stderr, "[injector] Wrote DLL path: %ls\n", dllPath.c_str());
    }

    // 6. Get address of LoadLibraryW from kernel32.dll
    // This works because kernel32.dll is mapped at the same address in all processes
    // due to ASLR being applied at boot time, not per-process for system DLLs
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) {
        if (!options.quiet) {
            printWindowsError("GetModuleHandleW(kernel32) failed", GetLastError());
        }
        VirtualFreeEx(processHandle.get(), remoteMem, 0, MEM_RELEASE);
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    auto loadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(kernel32, "LoadLibraryW"));

    if (!loadLibraryW) {
        if (!options.quiet) {
            printWindowsError("GetProcAddress(LoadLibraryW) failed", GetLastError());
        }
        VirtualFreeEx(processHandle.get(), remoteMem, 0, MEM_RELEASE);
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    if (!options.quiet) {
        fprintf(stderr, "[injector] LoadLibraryW at %p\n",
                reinterpret_cast<void*>(loadLibraryW));
    }

    // 7. Create remote thread to call LoadLibraryW with DLL path
    HandleGuard remoteThread(CreateRemoteThread(
        processHandle.get(),
        nullptr,                    // Thread security attributes
        0,                          // Stack size (default)
        loadLibraryW,               // Start address (LoadLibraryW)
        remoteMem,                  // Parameter (pointer to DLL path)
        0,                          // Creation flags (run immediately)
        nullptr                     // Thread ID (don't need it)
    ));

    if (!remoteThread.valid()) {
        if (!options.quiet) {
            printWindowsError("CreateRemoteThread failed", GetLastError());
        }
        VirtualFreeEx(processHandle.get(), remoteMem, 0, MEM_RELEASE);
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    if (!options.quiet) {
        fprintf(stderr, "[injector] Created remote thread for DLL injection\n");
    }

    // 8. Wait for injection thread to complete
    DWORD waitResult = WaitForSingleObject(remoteThread.get(), 10000);  // 10 second timeout
    if (waitResult != WAIT_OBJECT_0) {
        if (!options.quiet) {
            if (waitResult == WAIT_TIMEOUT) {
                fprintf(stderr, "[injector] Injection thread timed out\n");
            } else {
                printWindowsError("WaitForSingleObject failed", GetLastError());
            }
        }
        VirtualFreeEx(processHandle.get(), remoteMem, 0, MEM_RELEASE);
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    // Check if LoadLibrary succeeded by getting thread exit code
    DWORD exitCode = 0;
    if (GetExitCodeThread(remoteThread.get(), &exitCode) && exitCode == 0) {
        if (!options.quiet) {
            fprintf(stderr, "[injector] Warning: LoadLibraryW returned NULL (DLL load may have failed)\n");
        }
        // Continue anyway - the process might still work
    }

    if (!options.quiet) {
        fprintf(stderr, "[injector] DLL injection completed\n");
    }

    // 9. Free the remote memory (no longer needed)
    VirtualFreeEx(processHandle.get(), remoteMem, 0, MEM_RELEASE);

    // 10. Resume the main thread
    DWORD suspendCount = ResumeThread(threadHandle.get());
    if (suspendCount == static_cast<DWORD>(-1)) {
        if (!options.quiet) {
            printWindowsError("ResumeThread failed", GetLastError());
        }
        TerminateProcess(processHandle.get(), 1);
        return -1;
    }

    if (!options.quiet) {
        fprintf(stderr, "[injector] Resumed main thread (suspend count was %lu)\n", suspendCount);
    }

    // 11. Wait for process if not detaching
    if (!options.detach) {
        if (!options.quiet) {
            fprintf(stderr, "[injector] Waiting for process to exit...\n");
        }
        WaitForSingleObject(processHandle.get(), INFINITE);

        DWORD processExitCode = 0;
        GetExitCodeProcess(processHandle.get(), &processExitCode);

        if (!options.quiet) {
            fprintf(stderr, "[injector] Process exited with code %lu\n", processExitCode);
        }
    }

    return processId;
}

}  // namespace qtmcp

#endif  // Q_OS_WIN
