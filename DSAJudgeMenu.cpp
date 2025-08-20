
#define _CRT_SECURE_NO_WARNINGS
#include "DSAJudgeMenu.h"
#include "testExecution.h"
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

// ANSI Colors
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"

//Logger logFile;

void showMenu() {
    TestExecutor* executor = new TestExecutor();
    bool exitProgram = false;

    while (!exitProgram) {
        std::cout << CYAN << "\n=== Competitive Coding Judge ===\n" << RESET;
        std::cout << "1. Run internal problem with internal testcases\n";
        std::cout << "2. Run your own file\n";
        std::cout << "3. View final report\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter choice: ";

        int mainChoice;
        std::cin >> mainChoice;

        if (mainChoice == 1) 
        {
            int problemNumber;
            executor->listProblems();
            std::cout << "Enter the problem number: ";
            std::cin >> problemNumber;
            std::string problemName = "problem" + std::to_string(problemNumber);
            executor->runAllTestCases(problemName);
        }
        else if (mainChoice == 2) 
        {
            int langChoice;
            std::string language;
            std::cout << "Choose your language:\n";
            std::cout << "1. C++\n";
            std::cout << "2. Java\n";
            std::cout << "3. Python\n";
            std::cin >> langChoice;

            std::string codeFile;
            std::cout << "Enter path to your source code file: ";
            std::cin >> codeFile;

            fs::path UserPath = executor->baseDir / "UserCode";
            fs::create_directories(UserPath);
            fs::path exeFile;

            bool compiled = false;
            if (langChoice == 1) 
            { // C++
                language = "cpp";
                exeFile = UserPath / "UserCode.exe";
                compiled = executor->compileSubmission(codeFile, exeFile.string(), (executor->logFile).string(), language);

            }
            else if (langChoice == 2) 
            { // Java
                language = "java";
                fs::path p(codeFile);
                std::string filename = p.stem().string(); // "problem"
                exeFile = UserPath/ filename;
                compiled = executor->compileSubmission(codeFile, exeFile.string(), (executor->logFile).string(), language);
            }
            else if (langChoice == 3) 
            { // Python
                // For Python no compilation needed, just check file exists
                language = "python";
                if (fs::exists(codeFile))
                {
                    compiled = true;
                    exeFile = codeFile;
                }
            }
            else 
            {
                std::cerr << RED << "Invalid language choice.\n" << RESET;
                continue;
            }
            if (!compiled) {
                std::cerr << RED << "Compilation failed! See log for details.\n" << RESET;
                continue;
            }
            std::cout << GREEN << "Compilation successful / Ready to run.\n" << RESET;

            std::cout << "Choose mode:\n";
            std::cout << "1. Test with internal problem's testcases\n";
            std::cout << "2. Test with your own input/output files\n";
            std::cout << "3. Run a single custom testcase\n";
            int subChoice;
            std::cin >> subChoice;

            if (subChoice == 1) {
                int problemNumber;
                executor->listProblems();
                std::cout << "Enter the problem number: ";
                std::cin >> problemNumber;
                std::string problemName = "problem" + std::to_string(problemNumber);

                auto testcases = executor->loadTestcases((executor->codeFileDir / problemName).string());
                std::cout << YELLOW << "Found " << testcases.size() << " testcases.\n" << RESET;

                int passed = 0;
                for (size_t i = 0; i < testcases.size(); ++i) {
                    std::cout << "Running Testcase #" << (i + 1) << "...\n";
                    fs::path tempOutput = fs::current_path() / ("temp_output_tc" + std::to_string(i + 1) + ".txt");

                    DWORD exitCode = 0;
                    double execTime;
                    SIZE_T peakMem;
                    MessageBoxA(NULL, exeFile.string().c_str(), NULL, MB_OK);
                    int runResult = executor->runExecutableWithTimeout(
                        exeFile.string(),
                        testcases[i].inputPath,
                        tempOutput.string(),
                        2000,
                        exitCode,
                        execTime, peakMem,language);
            

                    bool passedThis = (runResult == 0) &&
                        executor->compareOutputs(tempOutput.string(), testcases[i].expectedOutputPath);

                    if (passedThis) {
                        std::cout << GREEN << "Testcase #" << (i + 1) << ": PASSED\n" << RESET;
                        passed++;
                    }
                    else {
                        std::cout << RED << "Testcase #" << (i + 1) << ": FAILED\n" << RESET;
                    }
                    // Show execution metrics
                    std::cout << YELLOW << "Execution Time: " << execTime << " ms\n"
                        << "Peak Memory Usage: " << peakMem << " KB\n" << RESET;
                    fs::remove(tempOutput);
                }
                std::cout << YELLOW << "Summary: " << passed << "/" << testcases.size() << " passed.\n" << RESET;
            }
            else if (subChoice == 2) 
            {
                std::string inputFile, outputFile;
                std::cout << "Enter path to your input file: ";
                std::cin >> inputFile;
                std::cout << "Do you want to provide an output file for comparison? (y/n): ";
                char hasOutput;
                std::cin >> hasOutput;

                fs::path outputFilePath = UserPath / "user_output.txt";
                DWORD exitCode = 0;
                double execTime;
                SIZE_T peakMem;
                int runExe = executor->runExecutableWithTimeout(exeFile.string(), inputFile, outputFilePath.string(), 2000, exitCode, execTime, peakMem,language);
                if (runExe != 0) {
                    std::cerr << RED << "Execution failed.\n" << RESET;
                    continue;
                }
                std::cout << GREEN << "Execution successful. Output saved to" << outputFilePath<< ".\n" << RESET;

                if (hasOutput == 'y' || hasOutput == 'Y') 
                {
                    std::cout << "Enter path to your output file: ";
                    std::cin >> outputFile;
                    bool match = executor->compareOutputs(outputFilePath.string(), outputFile);
                    if (!match) 
                    {
                        std::ifstream actualOut(outputFile);
                        std::ifstream userOut(outputFilePath.string());
                        std::stringstream userOutBuffer;
                        std::stringstream actualOutBuffer;
                        userOutBuffer << userOut.rdbuf();
                        actualOutBuffer << actualOut.rdbuf();
                        std::cout << "Expected Output:\n" << userOutBuffer.str() << "\n";
                        std::cout  << "Actual Output:\n" << actualOutBuffer.str() << "\n";
                        std::cout << "----------------------------------\n";

                    }
                    std::cout << (match ? GREEN "Outputs match!\n" RESET : RED "Outputs do NOT match!\n" RESET);
                    // Show execution metrics
                    std::cout << YELLOW << "Execution Time: " << execTime << " ms\n"
                        << "Peak Memory Usage: " << peakMem << " KB\n" << RESET;
                }
            }
            else if (subChoice == 3)
            {
                executor->runWithCustomTestcase(exeFile.string(), language);
            }
            else {
                std::cout << RED << "Invalid choice.\n" << RESET;
            }
        }
        else if (mainChoice == 3) {
            executor->displayFinalReport("ALL");
        }
        else if (mainChoice == 4) {
            exitProgram = true;
            std::cout << "Exiting...\n";
        }
        else {
            std::cout << RED << "Invalid choice, try again.\n" << RESET;
        }
    }
}

