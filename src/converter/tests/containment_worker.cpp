// Console-only synthetic adversarial worker. Never reads user data.
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <cstdlib>
#pragma comment(lib, "ws2_32.lib")

int inheritedEvent(HANDLE handle) {
    __try { DWORD flags = 0; return !GetHandleInformation(handle, &flags) && GetLastError() == ERROR_INVALID_HANDLE ? 10 : 97; }
    __except (GetExceptionCode() == 0xc0000008 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return 10; }
}
int wmain(int argc, wchar_t** argv)
{
    if (argc < 3) return 90;
    const std::wstring mode(argv[1]);
    if (mode == L"mkdir") return CreateDirectoryW(argv[2], nullptr) ? 0 : (GetLastError() == ERROR_ACCESS_DENIED ? 10 : 1000 + int(GetLastError()));
    if (mode == L"read" || mode == L"write" || mode == L"create") {
        HANDLE file = CreateFileW(argv[2], mode == L"read" ? GENERIC_READ : GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            mode == L"create" ? CREATE_NEW : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_ACCESS_DENIED ? 10 : 91;
        if (mode == L"read") {
            char bytes[7] = {}; DWORD count = 0;
            const bool matches = ReadFile(file, bytes, 6, &count, nullptr) && count == 6 && std::string(bytes, 6) == "canary";
            CloseHandle(file); return matches ? 0 : 92;
        }
        CloseHandle(file); return 0;
    }
    if (mode == L"network") {
        WSADATA data = {};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 93;
        SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketHandle == INVALID_SOCKET) return WSAGetLastError() == WSAEACCES ? 10 : 94;
        sockaddr_in address = {}; address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); address.sin_port = htons(static_cast<unsigned short>(_wtoi(argv[2])));
        u_long nonblocking = 1; ioctlsocket(socketHandle, FIONBIO, &nonblocking);
        int result = connect(socketHandle, reinterpret_cast<sockaddr*>(&address), sizeof(address));
        const int error = result == SOCKET_ERROR ? WSAGetLastError() : 0;
        closesocket(socketHandle); WSACleanup();
        // A timeout/refused/unreachable endpoint is not isolation evidence.
        return error == WSAEACCES ? 10 : 95;
    }
    if (mode == L"spawn") {
        STARTUPINFOW startup = {}; startup.cb = sizeof(startup); PROCESS_INFORMATION child = {};
        if (!CreateProcessW(argv[2], nullptr, nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                            nullptr, nullptr, &startup, &child))
            return (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_NOT_ENOUGH_QUOTA) ? 10 : 1000 + int(GetLastError());
        TerminateProcess(child.hProcess, 1); CloseHandle(child.hThread); CloseHandle(child.hProcess); return 0;
    }
    if (mode == L"inherit") {
        HANDLE handle = reinterpret_cast<HANDLE>(_wcstoui64(argv[2], nullptr, 10));
        return inheritedEvent(handle);
    }
    return 98;
}
