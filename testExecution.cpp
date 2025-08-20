#define _CRT_SECURE_NO_WARNINGS
#include "testExecution.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <windows.h>
#include <psapi.h>  
#include <cstdio>


#define RED_     "\033[31m"
#define RESET_   "\033[0m"
#define GREEN_   "\033[32m"
#define YELLOW_  "\033[33m"
#define CYAN_    "\033[36m"
#define BLUE_    "\033[34m"
#define MAGENTA_ "\033[35m"

namespace fs = std::filesystem;

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && isspace((unsigned char)*start)) start++;
    auto end = s.end();
    do { end--; } while (distance(start, end) > 0 && isspace((unsigned char)*end));
    return std::string(start, end + 1);
}

TestExecutor::TestExecutor() {
    baseDir = fs::current_path().parent_path().parent_path();
    codeFileDir = baseDir / "submissions";
    exeFileDir = baseDir / "bin";
    logFileDir = baseDir / "logs";
    logFile = logFileDir / "result.txt";

    fs::create_directories(logFileDir);
    fs::create_directories(exeFileDir);
    fs::create_directories(codeFileDir);
}

// ===== Compile =====
bool TestExecutor::compileSubmission(const std::string& codeFilePath,
    const std::string& outputPath,
    const std::string& logPath,
    const std::string& language)
{
  
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    SECURITY_ATTRIBUTES sa{};
    si.cb = sizeof(si);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hLogFile = CreateFileA(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLogFile == INVALID_HANDLE_VALUE) return false;

    si.hStdOutput = hLogFile;
    si.hStdError = hLogFile;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmd;

    if (language == "cpp") 
    {
        cmd = "g++ \"" + codeFilePath + "\" -o \"" + outputPath + "\"";
    }
    else if (language == "java")
    {
        MessageBoxA(NULL, outputPath.c_str(), NULL, MB_OK);

        // Extract just the directory for output (bin folder)
        std::string exeDir = fs::path(outputPath).parent_path().string();
        // Compile Java: put .class inside bin/
        cmd = "javac -d \"" + exeDir + "\" \"" + codeFilePath + "\"";

    }

    else if (language == "python") 
    {
        // Python doesn’t need compilation → just check syntax
        cmd = "python -m py_compile \"" + codeFilePath + "\"";
    }
    else {
        CloseHandle(hLogFile);
        return false;
    }

    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');
    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hLogFile);
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hLogFile);

    if (exitCode != 0) {
        std::cerr << "Compilation failed. Check log file: " << logPath << "\n";
        return false;
    }
    std::cout << "Compilation successful.\n";
    return true;

}


// ===== Load Testcases =====
std::vector<Testcase> TestExecutor::loadTestcases(const std::string& testcaseDir) {
    std::vector<Testcase> testcases;
    int index = 1;
    while (true) {
        fs::path inPath = fs::path(testcaseDir) / ("input" + std::to_string(index) + ".txt");
        fs::path outPath = fs::path(testcaseDir) / ("output" + std::to_string(index) + ".txt");
        if (!fs::exists(inPath) || !fs::exists(outPath)) break;
        testcases.push_back({ inPath.string(), outPath.string() });
        ++index;
    }
    return testcases;
}
int TestExecutor::runExecutableWithTimeout(const std::string& runTarget,
    const std::string& inputPath,
    const std::string& outputPath,
    DWORD timeoutMs,
    DWORD& exitCodeOut,
    double& execTimeSec,
    SIZE_T& peakMemoryBytes,
    const std::string& language)
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    SECURITY_ATTRIBUTES sa{};
    si.cb = sizeof(si);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hInputFile = CreateFileA(inputPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
        &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hInputFile == INVALID_HANDLE_VALUE) return 2;

    HANDLE hOutputFile = CreateFileA(outputPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutputFile == INVALID_HANDLE_VALUE) {
        CloseHandle(hInputFile);
        return 2;
    }

    si.hStdInput = hInputFile;
    si.hStdOutput = hOutputFile;
    si.hStdError = hOutputFile;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmd;
    if (language == "cpp") {
        cmd = "\"" + runTarget + "\"";
    }
    else if (language == "java") 
    {
        fs::path runPath = runTarget;          // e.g. C:\...\bin\problem
        std::string classDir = runPath.parent_path().string(); // C:\...\bin
        std::string className = runPath.filename().string();   // problem

        cmd = "java -cp \"" + classDir + "\" " + className;
    }
    else if (language == "python") 
    {
        cmd = "py \"" + runTarget + "\"";
    }
    else {
        CloseHandle(hInputFile);
        CloseHandle(hOutputFile);
        return 2; // Unsupported
    }

    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    auto startTime = std::chrono::high_resolution_clock::now();

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hInputFile);
        CloseHandle(hOutputFile);
        return 2;
    }

    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);

    auto endTime = std::chrono::high_resolution_clock::now();
    execTimeSec = std::chrono::duration<double>(endTime - startTime).count();

    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, (UINT)-1);
        exitCodeOut = (DWORD)-1;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hInputFile);
        CloseHandle(hOutputFile);
        return 1; // Timeout
    }

    GetExitCodeProcess(pi.hProcess, &exitCodeOut);

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
        peakMemoryBytes = pmc.PeakWorkingSetSize;
    }
    else {
        peakMemoryBytes = 0;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hInputFile);
    CloseHandle(hOutputFile);

    return 0;
}
// ===== Compare Outputs =====
bool TestExecutor::compareOutputs(const std::string& actualPath, const std::string& expectedPath) {
    std::ifstream actual(actualPath), expected(expectedPath);
    if (!actual.is_open() || !expected.is_open()) return false;
    std::string actualLine, expectedLine;
    while (true) {
        bool hasA = static_cast<bool>(std::getline(actual, actualLine));
        bool hasE = static_cast<bool>(std::getline(expected, expectedLine));
        if (!hasA && !hasE) return true;
        if (hasA != hasE) return false;
        if (trim(actualLine) != trim(expectedLine)) return false;
    }
}

// ===== List Problems with Descriptions =====
//void TestExecutor::listProblems() {
//
//    std::cout << YELLOW_ << "\nAvailable Problems:\n"<< RESET_;
//    for (auto& entry : fs::directory_iterator(codeFileDir)) {
//        if (fs::is_directory(entry)) {
//            std::string folderName = entry.path().filename().string();
//            std::cout << "- " << folderName;
//            fs::path descFile = entry.path() / "description.txt";
//            if (fs::exists(descFile)) {
//                std::ifstream desc(descFile);
//                std::string line;
//                std::getline(desc, line);
//                std::cout << " -> " << line;
//            }
//            std::cout << "\n";
//        }
//    }
//}
// ===== List Problems with Descriptions =====
void TestExecutor::listProblems() {
    std::cout << YELLOW_ << "\n========== Available Problems ==========\n" << RESET_;

    for (auto& entry : fs::directory_iterator(codeFileDir)) {
        if (fs::is_directory(entry)) {
            std::string folderName = entry.path().filename().string();
            std::cout << CYAN_ << "\n" << folderName << RESET_ << "\n";

            fs::path descFile = entry.path() / "description.txt";
            if (fs::exists(descFile)) {
                std::ifstream desc(descFile);
                std::string line;

                while (std::getline(desc, line)) {
                    // Highlight keywords like Title, Language, Description
                    if (line.find("Title:") == 0) {
                        std::cout << GREEN_  << line << RESET_ << "\n";
                    }
                    else if (line.find("Language:") == 0) {
                        std::cout << BLUE_ << line << RESET_ << "\n";
                    }
                    else if (line.find("Description:") == 0) {
                        std::cout << MAGENTA_ << line << RESET_ << "\n";
                    }
                    else {
                        std::cout << "   " << line << "\n"; // fallback for extra lines
                    }
                }
            }
            else {
                std::cout << RED_ << " No description found\n" << RESET_;
            }
        }
    }

    std::cout << YELLOW_ << "\n========================================\n" << RESET_;
}

// ===== Display Final Report =====
void TestExecutor::displayFinalReport(const std::string& problemName)
{
    if (!fs::exists(logFileDir) || !fs::is_directory(logFileDir)) {
        std::cerr<< RED_ << "Log directory doesn't exist.\n" << RESET_;
        return;
    }

    bool foundReport = false;

    for (const auto& entry : fs::directory_iterator(logFileDir))
    {
        if (!entry.is_regular_file())
            continue;

        std::string filename = entry.path().filename().string();

        const std::string suffix = "_final_report.txt";
        if (filename.size() >= suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            foundReport = true;

            std::string problemName = filename.substr(0, filename.size() - suffix.size());

            std::cout << "==== " << problemName << " ====\n";

            std::ifstream in(entry.path());
            if (!in.is_open()) {
                std::cerr << "Cannot open report file: " << filename << "\n";
                continue;
            }

            std::string line;
            while (std::getline(in, line)) {
                std::cout << line << "\n";
            }
            std::cout << "\n";
        }
    }

    if (!foundReport) {
        std::cout << RED_ << "No final report files found.\n"<< RESET_;
    }
}



void TestExecutor::generateFinalReport(const fs::path& outputResultPath, const fs::path& reportPath) {
    std::ifstream inFile(outputResultPath);
    if (!inFile.is_open()) {
        std::cerr << "Error: Cannot open " << outputResultPath << "\n";
        return;
    }

    std::ofstream reportFile(reportPath);
    if (!reportFile.is_open()) {
        std::cerr << "Error: Cannot create " << reportPath << "\n";
        return;
    }

    // Timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    reportFile << "=== FINAL TEST REPORT ===\n";
    reportFile << "Generated on: " << std::ctime(&currentTime) << "\n";
    reportFile << "---------------------------\n";

    std::string line;
    int totalCases = 0, passedCases = 0, failedCases = 0;
    double totalExecTime = 0.0;
    size_t totalPeakMem = 0;

    // Color codes
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string YELLOW = "\033[33m";
    const std::string RESET = "\033[0m";

    // Process each line from the result file
    while (std::getline(inFile, line)) {
        reportFile << line << "\n"; // Write to file as plain text

        // Console output with colors
        if (line.find("PASSED") != std::string::npos) {
            std::cout << GREEN << line << RESET << "\n";
        }
        else if (line.find("FAILED") != std::string::npos) {
            std::cout << RED << line << RESET << "\n";
        }
        else {
            std::cout << line << "\n";
        }

        if (line.find("Testcase #") != std::string::npos) {
            totalCases++;
            if (line.find("PASSED") != std::string::npos) passedCases++;
            else if (line.find("FAILED") != std::string::npos) failedCases++;
        }
        else if (line.find("Execution Time:") != std::string::npos) {
            double execTime = 0.0;
            sscanf(line.c_str(), "Execution Time: %lf ms", &execTime);
            totalExecTime += execTime;
        }
        else if (line.find("Peak Memory Usage:") != std::string::npos) {
            size_t mem = 0;
            sscanf(line.c_str(), "Peak Memory Usage: %zu KB", &mem);
            totalPeakMem += mem;
        }
    }

    // Summary for file
    reportFile << "---------------------------\n";
    reportFile << "Summary: " << passedCases << "/" << totalCases << " passed.\n";
    reportFile << "Total Testcases: " << totalCases << "\n";
    reportFile << "Passed: " << passedCases << "\n";
    reportFile << "Failed: " << failedCases << "\n";
    reportFile << "Average Execution Time: "
        << (totalCases > 0 ? totalExecTime / totalCases : 0.0) << " ms\n";
    reportFile << "Average Peak Memory Usage: "
        << (totalCases > 0 ? totalPeakMem / totalCases : 0) << " KB\n";
    reportFile << "===========================\n";

    // Summary for console
    std::cout << YELLOW << "---------------------------\n";
    std::cout << "Summary: " << passedCases << "/" << totalCases << " passed.\n";
    std::cout << "Total Testcases: " << totalCases << "\n";
    std::cout << "Passed: " << passedCases << "\n";
    std::cout << "Failed: " << failedCases << "\n";
    std::cout << "Average Execution Time: "
        << (totalCases > 0 ? totalExecTime / totalCases : 0.0) << " ms\n";
    std::cout << "Average Peak Memory Usage: "
        << (totalCases > 0 ? totalPeakMem / totalCases : 0) << " KB\n";
    std::cout << "===========================" << RESET << "\n";

    inFile.close();
    reportFile.close();
}



// ===== Custom Testcase Runner =====
void TestExecutor::runWithCustomTestcase(const std::string& exePath, const std::string& language)
{
    std::string inputFile, expectedFile;
    std::cout << "Enter path to input file: ";
    std::cin >> inputFile;
    std::cout << "Enter path to expected output file: ";
    std::cin >> expectedFile;

    fs::path tempOutput = fs::current_path() / "temp_custom_output.txt";
    double execTime;
    SIZE_T peakMem;
    DWORD exitCode = 0;

    int runResult = runExecutableWithTimeout(exePath, inputFile, tempOutput.string(), 2000, exitCode, execTime, peakMem,language);

    bool passed = (runResult == 0) && compareOutputs(tempOutput.string(), expectedFile);

    if (passed) std::cout<<GREEN_ << "[CUSTOM TEST] PASSED\n"<< RESET_;
    else std::cout<<RED_ << "[CUSTOM TEST] FAILED\n"<< RESET_;

    // Show execution metrics
    std::cout << YELLOW_ << "Execution Time: " << execTime << " ms\n"
        << "Peak Memory Usage: " << peakMem << " KB\n" << RESET_;
    fs::remove(tempOutput);
}
std::string readFile(const std::string& filePath)
{
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        return "[Error: Could not read file " + filePath + "]";
    }
    std::ostringstream ss;
    ss << inFile.rdbuf();
    return ss.str();
}
void TestExecutor::runAllTestCases(const std::string& problemName)
{
    fs::path problemDir = codeFileDir / problemName;
    auto testcases = loadTestcases(problemDir.string());

    if (testcases.empty()) {
        std::cerr << "No testcases found for " << problemName << ".\n";
        return;
    }

    std::cout << "Found " << testcases.size() << " testcases.\n";

    // -------- Detect Language & Source File --------
    std::string language;
    fs::path sourceFile;
    fs::path exeFile;

    if (fs::exists(problemDir / "problem.cpp")) {
        language = "cpp";
        sourceFile = problemDir / "problem.cpp";
        exeFile = exeFileDir / (problemName + ".exe");
    }
    else if (fs::exists(problemDir / "problem.java")) 
    {
        language = "java";
        sourceFile = problemDir / "problem.java";

        // Extract class name (remove extension)
        std::string filename = sourceFile.stem().string(); // "problem"
        exeFile = exeFileDir/( filename) ;  // store just class name, not .class
    }
    else if (fs::exists(problemDir / "problem.py")) 
    {
        language = "python";
        sourceFile = problemDir / "problem.py";
        exeFile = sourceFile; // no compilation needed
    }
    else {
        std::cerr << "No solution file found for " << problemName << ".\n";
        return;
    }

    // -------- Compile if Required --------
    if (language == "cpp" || language == "java") 
    {
        if (!compileSubmission(sourceFile.string(), exeFile.string(), logFile.string(), language)) {
            std::cerr << "Compilation failed for " << problemName << ".\n";
            return;
        }
        std::cout << "Compilation successful.\n";
    }
    else 
    {
        std::cout << "Python detected. Skipping compilation.\n";
    }

    // -------- Open Log File --------
    std::ofstream resultLog(logFile);
    if (!resultLog.is_open()) {
        std::cerr << "Could not open log file: " << logFile << "\n";
        return;
    }

    int passed = 0;

    // -------- Run All Testcases --------
    for (size_t i = 0; i < testcases.size(); ++i) {
        fs::path tempOutput = fs::current_path() / ("temp_output_tc" + std::to_string(i + 1) + ".txt");

        double execTime;
        SIZE_T peakMem;
        DWORD exitCode = 0;

        int result = runExecutableWithTimeout(
            exeFile.string(),
            testcases[i].inputPath,
            tempOutput.string(),
            2000,      // timeout (ms)
            exitCode,
            execTime,
            peakMem,
            language   // NEW: pass language so runner knows how to execute
        );
        std::cout << exeFile.string() << "\n";

        bool pass = (result == 0) && compareOutputs(tempOutput.string(), testcases[i].expectedOutputPath);

        if (pass) {
            std::cout << "\033[32mTestcase #" << (i + 1) << ": PASSED\033[0m\n";
            resultLog << "Testcase #" << (i + 1) << ": PASSED\n";
            passed++;
        }
        else {
            std::cout << "\033[31mTestcase #" << (i + 1) << ": FAILED\033[0m\n";

            std::string expectedOutput = readFile(testcases[i].expectedOutputPath);
            std::string actualOutput = readFile(tempOutput.string());

            resultLog << "Testcase #" << (i + 1) << ": FAILED\n";
            resultLog << "Expected Output:\n" << expectedOutput << "\n";
            resultLog << "Actual Output:\n" << actualOutput << "\n";
            resultLog << "----------------------------------\n";
        }

        // Execution Metrics
        std::cout << "\033[36mExecution Time: " << execTime << " ms\n"
            << "Peak Memory Usage: " << peakMem << " KB\033[0m\n";
        resultLog << "Execution Time: " << execTime << " ms\n";
        resultLog << "Peak Memory Usage: " << peakMem << " KB\n";
        resultLog << "----------------------------------\n";

        fs::remove(tempOutput);
    }

    // -------- Summary --------
    std::cout << "\033[33mSummary: " << passed << "/" << testcases.size() << " passed.\033[0m\n";
    resultLog << "Summary: " << passed << "/" << testcases.size() << " passed.\n";
    resultLog.close();

    // -------- Final Report --------
    fs::path reportPath = logFileDir / (problemName + "_final_report.txt");
    generateFinalReport(logFile, reportPath);

    std::cout << YELLOW_ << "Final report generated: " << reportPath << "\n" << RESET_;
}

