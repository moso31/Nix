#include "NXLog.h"
#include <filesystem>
#include "NXConverter.h"

namespace fs = std::filesystem;

std::filesystem::path NXLog::filePath;
std::ofstream NXLog::logFile;

void NXLog::Init()
{
    std::filesystem::path folderPath(".\\logs");
    std::filesystem::path fileName("Log.txt");
    
    if (!fs::exists(folderPath)) 
    {
        fs::create_directories(folderPath);
    }

    filePath = folderPath / fileName;
    logFile.open(filePath);

    if (!logFile.is_open()) 
    {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
    }

    // Initialize symbol handler for the process
    HANDLE process = GetCurrentProcess();
    if (!SymInitialize(process, NULL, TRUE))
    {
        std::cerr << "SymInitialize failed with error code: " << GetLastError() << std::endl;
    }
}

void NXLog::Log(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logFile << buffer << std::endl;
}

void NXLog::LogWithStackTrace(const char* format, ...)
{    
    // Capture stack trace
    void* stack[100];
    unsigned short frames = CaptureStackBackTrace(0, 100, stack, NULL);
    SYMBOL_INFO* symbol;
    HANDLE process;

    process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    logFile << "Stack trace:" << std::endl;

    for (int i = 1; i < frames; i++)
    {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        logFile << frames - i - 1 << ": " << symbol->Name << " at 0x" << symbol->Address << std::endl;
    }

    free(symbol);

    // Existing log code
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logFile << "Message: " << buffer << std::endl << std::endl;
}

void NXLog::Release()
{
    // Cleanup
    HANDLE process = GetCurrentProcess();
    SymCleanup(process);

    // Close log file if open
    if (logFile.is_open())
    {
        logFile.close();
    }
}
