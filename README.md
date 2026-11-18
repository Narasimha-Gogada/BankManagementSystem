# Bank Management System

A console-based bank management program written in C++. It is not a Node.js app, so `npm run dev` does not apply.

## Build and run on Windows

Install a C++ compiler such as MinGW-w64 and make sure `g++` is available in PowerShell. From the repository root, run:

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++17 BankManagementSystem\main.cpp BankManagementSystem\Account.cpp BankManagementSystem\Admin.cpp BankManagementSystem\Bank.cpp BankManagementSystem\BankSystem.cpp BankManagementSystem\Transaction.cpp -o build\BankManagementSystem.exe
.\build\BankManagementSystem.exe
```

The program keeps accounts and transactions in memory. They are lost when the program exits.

## Render

The current project is an interactive console application, not an HTTP web application. It cannot be deployed as a Render Web Service as-is. See [deploy.md](deploy.md) for the requirements and free-plan limitations if you later convert it into a web app.