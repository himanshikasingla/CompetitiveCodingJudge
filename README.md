# 🏆 Competitive Coding Judge (DSA Evaluation Engine)

A lightweight **C++ based coding judge** that can:  
- Compile and run solutions in **C++**, **Java**, and **Python**  
- Validate outputs against predefined **test cases** (inbuilt problems)  
- Work with **user-provided code, input, and output files** for custom testing  
- Generate detailed **execution logs and final reports**  

------
## 🚀 Features
- ✅ Supports **multiple languages**: C++, Java, Python  
- ✅ **3 modes of usage**:
  1. **Inbuilt Problems** → Choose from structured problems with descriptions & testcases  
  2. **User Provided Code** → Run your own `.cpp`, `.java`, or `.py` file with custom input/output files
  3. **Compare output** →  Compare outputs with expected results
- ✅ Executes code safely with **timeout protection** (prevents infinite loops)  
- ✅ Captures **compilation errors, runtime errors, and output mismatches**  
- ✅ Generates **final reports** for each submission (pass/fail summary, execution time, memory usage)  

---

## 🎥 Demo
Here’s a quick look at the judge in action:

🚧 *Demo GIF/Screenshots coming soon...*  

*(Will showcase running a C++/Python/Java solution with test cases and report generation)*  

---
## 📂 Project Structure
DSAJudge/
│──
│ ├── main.cpp
│ ├── TestExecutor.cpp
│ ├── DSAJudgeMenu.cpp
│ └── ...
│
│── submissions/ # in-build problems
│ └── problemX/
│ ├── solution.cpp / solution.java / problem.py
│ ├── description.txt
│ ├── input1.txt
│ ├── output1.txt
│ └── ...
│
│── logs/ # Generated logs and reports
│── bin/  in-build problems executable file created
│── UserCode / User Submision executable file created


---

## 🛠️ Technologies Used
- **C++17** (core judge engine)  
- **Windows API (CreateProcess)** for process management & timeout  
- **Multithreading** for execution control  
- **File I/O** for handling input/output/testcases  
- **Java (javac/java)** and **Python (py/python)** execution support  

---

## ⚡ How It Works
1. User selects a problem from the problem list (`description.txt`).  
2. Judge compiles (for C++/Java) or runs (for Python) the solution.  
3. Executes solution against testcases (`inputX.txt`).  
4. Compares generated output with expected (`outputX.txt`).  
5. Generates detailed **logs & final report**.  

---

## 📖 Example Usage
```bash
# Run the judge
./DSAJudge.exe

# Select a problem (e.g., Problem 1: Merge Sort)

# Submit a solution file (C++, Java, or Python)

# Get results:
- Compilation logs
- Execution time
- Memory usage
- Pass/Fail summary


🎯 Sample Problem Description (description.txt)
Title: Sum of Array Elements
Language: C++, Java, Python
Description: Read an array of size N and print the sum of its elements.

📊 Example Final Report
Testcase #1: PASSED
Execution Time: 10 ms
Memory Usage: 5 MB

Testcase #2: FAILED
Expected: 21
Got: 19

Summary: 1/2 testcases passed

---
