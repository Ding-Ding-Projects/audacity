/*
 * Material Audacity Squirrel.Windows shortcut launcher.
 *
 * Squirrel.Windows creates Start Menu and desktop shortcuts only for
 * executables that sit at the root of the package payload. Material Audacity
 * installs into bin\Audacity4.exe, with the data directories as siblings of
 * bin, because the application resolves its resources relative to that layout.
 * Moving the application to the package root would break resource resolution,
 * so a tiny launcher is placed at the root instead. Squirrel creates the
 * shortcuts for the launcher, and the launcher starts the real executable.
 *
 * The launcher deliberately carries no SquirrelAwareVersion resource. An
 * application that declares itself Squirrel aware is expected to handle the
 * install and update hooks itself, and Squirrel then skips its default
 * shortcut creation. Staying Squirrel unaware is what makes the default
 * shortcuts appear.
 *
 * Only Win32 APIs are used. The build links the static CRT, so the produced
 * executable depends on kernel32.dll and shell32.dll only.
 */

#include <windows.h>
#include <shellapi.h>

#define AU_MAX_PATH 32768

static WCHAR g_launcherPath[AU_MAX_PATH];
static WCHAR g_appDir[AU_MAX_PATH];
static WCHAR g_binDir[AU_MAX_PATH];
static WCHAR g_appPath[AU_MAX_PATH];
static WCHAR g_childCommandLine[AU_MAX_PATH];

/* Returns a pointer to the arguments of a command line, that is everything
 * after the program name token, or NULL when there are none. Quoted program
 * names may contain spaces, so quoting is tracked explicitly. */
static const WCHAR* argumentsOf(const WCHAR* commandLine)
{
    int inQuotes = 0;
    const WCHAR* p = commandLine;

    if (p == NULL) {
        return NULL;
    }

    while (*p != L'\0') {
        if (*p == L'"') {
            inQuotes = !inQuotes;
        } else if (!inQuotes && (*p == L' ' || *p == L'\t')) {
            break;
        }
        ++p;
    }

    while (*p == L' ' || *p == L'\t') {
        ++p;
    }

    return (*p == L'\0') ? NULL : p;
}

static void trimLastComponent(WCHAR* path)
{
    int i = lstrlenW(path);
    while (i > 0 && path[i - 1] != L'\\' && path[i - 1] != L'/') {
        --i;
    }
    while (i > 0 && (path[i - 1] == L'\\' || path[i - 1] == L'/')) {
        --i;
    }
    path[i] = L'\0';
}

static void reportFailure(const WCHAR* text)
{
    MessageBoxW(NULL, text, L"Material Audacity", MB_OK | MB_ICONERROR);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR commandLine, int showCommand)
{
    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;
    const WCHAR* arguments = NULL;
    DWORD exitCode = 0;
    DWORD length = 0;

    (void)instance;
    (void)previous;
    (void)commandLine;

    length = GetModuleFileNameW(NULL, g_launcherPath, AU_MAX_PATH);
    if (length == 0 || length >= AU_MAX_PATH) {
        reportFailure(L"Material Audacity could not determine its own location.");
        return 1;
    }

    lstrcpyW(g_appDir, g_launcherPath);
    trimLastComponent(g_appDir);

    lstrcpyW(g_binDir, g_appDir);
    lstrcatW(g_binDir, L"\\bin");

    lstrcpyW(g_appPath, g_binDir);
    lstrcatW(g_appPath, L"\\Audacity4.exe");

    if (GetFileAttributesW(g_appPath) == INVALID_FILE_ATTRIBUTES) {
        reportFailure(L"Material Audacity could not find bin\\Audacity4.exe next to this launcher.");
        return 1;
    }

    /* CreateProcessW may modify the command line buffer, so it must be
     * writable. The first token repeats the application path, as the
     * convention requires. */
    g_childCommandLine[0] = L'"';
    g_childCommandLine[1] = L'\0';
    lstrcatW(g_childCommandLine, g_appPath);
    lstrcatW(g_childCommandLine, L"\"");

    arguments = argumentsOf(GetCommandLineW());
    if (arguments != NULL) {
        if (lstrlenW(g_childCommandLine) + lstrlenW(arguments) + 2 >= AU_MAX_PATH) {
            reportFailure(L"The command line passed to Material Audacity is too long.");
            return 1;
        }
        lstrcatW(g_childCommandLine, L" ");
        lstrcatW(g_childCommandLine, arguments);
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = (WORD)showCommand;

    if (!CreateProcessW(g_appPath, g_childCommandLine, NULL, NULL, FALSE,
                        0, NULL, g_binDir, &startupInfo, &processInfo)) {
        reportFailure(L"Material Audacity could not start bin\\Audacity4.exe.");
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
        exitCode = 1;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    return (int)exitCode;
}
