#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

#if ANDROID
#include <android/log.h>
#endif

static class DebugStateClass
{
public:
    DebugStateClass()
        : File(nullptr)
    {
        /* Windows doesn't attach to the console by default so printing to stderr does nothing. */
#if defined(_WIN32) && _WIN32_WINNT >= 0x0501
        /* Attach to the console that started us if any */
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            /* We attached successfully, lets redirect IO to the consoles handles if not already redirected */
            if (_fileno(stdout) == -2 || _get_osfhandle(fileno(stdout)) == -2) {
                freopen("CONOUT$", "w", stdout);
            }

            if (_fileno(stderr) == -2 || _get_osfhandle(fileno(stderr)) == -2) {
                freopen("CONOUT$", "w", stderr);
            }

            if (_fileno(stdin) == -2 || _get_osfhandle(fileno(stdin)) == -2) {
                freopen("CONIN$", "r", stdin);
            }
        }
#endif
    }

    ~DebugStateClass()
    {
        if (File != nullptr) {
            fclose(File);
        }

        File = nullptr;
    }

    FILE* File;
} DebugState;

/**
 * Main log function, intended to be used from behind macros that pass in file and line details.
 */
void Debug_String_Log(unsigned level, const char* file, int line, const char* fmt, ...)
{
    assert(level <= 6);
#ifndef ANDROID
    static const char* levels[] = {"NONE", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    /* If we have a file pointer set we are logging to a file */
    if (DebugState.File != nullptr) {
        va_list args;
        fprintf(DebugState.File, "%-5s %s:%d: ", levels[level], file, line);
        va_start(args, fmt);
        vfprintf(DebugState.File, fmt, args);
        fprintf(DebugState.File, "\n");
        va_end(args);
        fflush(DebugState.File);
    }

    /* Don't print file and line numbers to stderr to avoid clogging it up with too much info */
    va_list args;
    fprintf(stderr, "%-5s: ", levels[level]);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    fflush(stderr);
#else
    static const int android_levels[] = {
            ANDROID_LOG_UNKNOWN, // NONE
            ANDROID_LOG_FATAL,   // FATAL
            ANDROID_LOG_ERROR,   // ERROR
            ANDROID_LOG_WARN,    // WARN
            ANDROID_LOG_INFO,    // INFO
            ANDROID_LOG_DEBUG,   // DEBUG
            ANDROID_LOG_VERBOSE  // TRACE
    };
    const char* TAG = "VanillaConquer";
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(android_levels[level], TAG, fmt, args);
    va_end(args);
#endif
}

void Debug_String_File(const char* file)
{
    if (DebugState.File != nullptr) {
        fclose(DebugState.File);
    }

    DebugState.File = fopen(file, "w");
}
