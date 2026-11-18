# Deploying on Render (Free Plan)

## Important: the current program is not web-deployable yet

This repository currently contains a console C++ program that reads from `std::cin` and writes to `std::cout`. Render Web Services host HTTP applications, not interactive terminal programs. Also, `BankManagementSystem/main.cpp` is currently empty. Deploying the current repository as a web service will not produce a usable website.

Before deploying, you must:

1. Add and test a `main()` entry point so the C++ application builds and runs locally.
2. Convert the console workflow into an HTTP API or web application. The server must listen on `0.0.0.0` and the port supplied in the `PORT` environment variable.
3. Add a `Dockerfile` that builds and starts that HTTP server. The server process must stay running and handle HTTP requests; simply starting the console menu is not sufficient.

## Create the Render service

After the HTTP application and `Dockerfile` are ready and pushed to the `main` branch of this GitHub repository:

1. Sign in at [Render](https://dashboard.render.com/).
2. Select **New** > **Web Service** and connect the GitHub repository `Narasimha-Gogada/BankManagementSystem`.
3. Select the `main` branch.
4. Set **Root Directory** to `BankManagementSystem` if the `Dockerfile` is inside that folder. If the `Dockerfile` is at the repository root, leave **Root Directory** blank.
5. Select **Docker** as the runtime. Configure the Dockerfile to compile the C++ sources and run the HTTP server. Do not use the console application's menu as the service start command.
6. Select the **Free** instance plan and create the service.
7. Wait for the build and deploy to finish. Open the `onrender.com` URL shown on the service page and check the deploy logs if it fails.

Render normally deploys new commits to the linked branch automatically. Push code updates to `main` to trigger another deploy.

## Free-plan limitations

- A free web service spins down after 15 minutes without inbound traffic. The first request after that can take about a minute while it starts again.
- Free services can restart, and their local filesystem is ephemeral. Files written by the app are not durable across restarts or deploys.
- This project's accounts and transactions are kept in memory, so they will be lost whenever the process restarts or spins down. Do not use it to store real customer or financial data.
- Render's free Postgres database expires after 30 days. It is not a permanent free database for a banking application.
- Render's free plan and limits can change. Check [Render's free-plan details](https://render.com/docs/free) before deploying. Stay within the included usage limits to avoid charges or service suspension.

## Official Render references

- [Deploy for free](https://render.com/docs/free)
- [Web Services](https://render.com/docs/web-services)
- [Deploying on Render](https://render.com/docs/deploys)