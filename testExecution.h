#pragma once
#include <windows.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

// Structure to hold a single test case's file paths
struct Testcase {
    std::string inputPath;
    std::string expectedOutputPath;
};

class TestExecutor {
public:
    fs::path baseDir;       // Base directory of project
    fs::path codeFileDir;   // Path to submissions folder
    fs::path logFileDir;    // Path to logs folder
    fs::path exeFileDir;    // Path to compiled executables
    fs::path logFile;       // Path to result log file

    // Constructor
    TestExecutor();

    // Compilation
    bool compileSubmission(const std::string& codeFilePath,
        const std::string& outputExePath,
        const std::string& logPath,
        const std::string& language);

    int runExecutableWithTimeout(const std::string& exePath,
        const std::string& inputPath,
        const std::string& outputPath,
        DWORD timeoutMs,
        DWORD& exitCodeOut,
        double& execTimeSec,
        SIZE_T& peakMemoryBytes,
        const std::string& language);

    // Testcase handling
    std::vector<Testcase> loadTestcases(const std::string& testcaseDir);
    bool compareOutputs(const std::string& actualPath, const std::string& expectedPath);

    // Reporting
   void generateFinalReport(const fs::path& outputResultPath,const fs::path& reportPath);
    void displayFinalReport(const std::string& problemNumber);

    // Running
    void runAllTestCases(const std::string& problemName);
    void runWithCustomTestcase(const std::string& exePath, const std::string& language);

    // Problem listing
    void listProblems();
};
