# Bank Management System

A C++ banking-system demonstration with both a terminal interface and a browser-based demo served by a C++ HTTP API. The demo supports account creation, sign-in, deposits, withdrawals, transfers, password changes, and closing an account only when its balance is zero. This is not a real banking service. Do not enter real credentials or financial information.

## Run the web demo locally

This project does not use Node.js, npm, or an `npm run dev` script. The `npm is not recognized` message means npm is unavailable, but installing Node.js will not run this C++ application. Use Docker Desktop instead. If PowerShell currently shows the parent folder `bank_management`, run:

```powershell
cd .\BankMangementSystem
docker build -t bank-management-demo .
docker run --rm -p 10000:10000 -e PORT=10000 bank-management-demo
```

Open `http://localhost:10000`. Stop the local server with `Ctrl+C`.

## Run the console program

The original terminal interface is available as the `bank-console` CMake target. Install CMake and OpenSSL development files, then run these commands from the repository root:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
.\build\bank-console.exe
```

The CMake configure step downloads pinned versions of cpp-httplib and nlohmann/json. To run the web server directly after building, run this from the repository root and open `http://localhost:10000`:

```powershell
.\build\bank-web.exe
```

## Deploy on Render

See [deploy.md](deploy.md) for the Docker-based Render setup. The app keeps its demo accounts in memory; they disappear when the service restarts or spins down. This project is not suitable for real customer accounts or funds.