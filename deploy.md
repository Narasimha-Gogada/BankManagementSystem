# Deploying the Demo on Render

This repository now includes a C++ HTTP service and a browser interface. It is a learning demo only, not a real banking product. Do not enter real passwords, customer details, or financial information.

This is not a Node.js project: `npm run dev` is not a valid command here. If PowerShell reports that `npm` is not recognized, do not install Node just for this project. Render builds and runs the app from the root `Dockerfile`.

## Deploy from GitHub

1. Push the project to the `main` branch of `Narasimha-Gogada/BankManagementSystem`.
2. Sign in to [Render](https://dashboard.render.com/) and choose **New** > **Web Service**.
3. Connect the GitHub repository `Narasimha-Gogada/BankManagementSystem` and select the `main` branch.
4. Leave **Root Directory** blank. The `Dockerfile` is in the repository root.
5. Select **Docker** as the runtime. No separate build or start command is needed; Render uses the root `Dockerfile`.
6. Select the **Free** instance plan and create the service.
7. When the deployment finishes, open the `onrender.com` URL shown on the service page.

The landing page includes **Continue as demo visitor** so the main workflows can be tried without manually creating credentials. Sign-in and account creation remain available as optional demo flows. An account can be closed from the dashboard only after its balance reaches Rs. 0.00.

The Docker build installs CMake, a C++ compiler, and OpenSSL, downloads the pinned C++ libraries, and builds the HTTP server. The server binds to `0.0.0.0` and uses Render's `PORT` environment variable.

## Push updates from PowerShell

From the repository root, stage and commit your changes, then push the local `clean-main` branch to your GitHub `main` branch:

```powershell
git add -A
git commit -m "Describe your changes"
git push origin HEAD:main
```

Render will automatically deploy new commits from `main` when auto-deploy is enabled.

## Free-plan and demo limitations

- Render spins down a free web service after 15 minutes without inbound traffic. The first request after idle may take about a minute to start.
- The app stores accounts, transactions, and sessions only in memory. Restarts, deploys, and spin-downs erase them.
- If a free-service restart invalidates the browser session, the next protected action opens a fresh demo account and retries once. The previous balance and transactions cannot be restored; the page displays a reset notice.
- Passwords are hashed with PBKDF2, but the app has not had a production security review and has no durable database, rate limiting, or real banking controls.
- Do not use this demo for actual financial activity or real customer data.
- Render's free plan and limits can change. Review [Render's free-plan details](https://render.com/docs/free) before deployment.

## Official Render references

- [Deploy for free](https://render.com/docs/free)
- [Web Services](https://render.com/docs/web-services)
- [Deploying on Render](https://render.com/docs/deploys)