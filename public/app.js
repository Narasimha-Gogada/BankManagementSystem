const authView = document.querySelector("#auth-view");
const dashboardView = document.querySelector("#dashboard-view");
const authMessage = document.querySelector("#auth-message");
const transactionMessage = document.querySelector("#transaction-message");
const passwordMessage = document.querySelector("#password-message");
const tokenKey = "fieldnote-demo-token";
let activeAction = "deposit";
let demoSessionWasRestored = false;

function showMessage(element, message, isError = false) {
  element.textContent = message;
  element.classList.toggle("error", isError);
}

async function fetchApi(path, options = {}) {
  const headers = { "Content-Type": "application/json", ...options.headers };
  const token = sessionStorage.getItem(tokenKey);
  if (token) headers.Authorization = `Bearer ${token}`;

  return fetch(path, { ...options, headers });
}

async function startDemoSession() {
  const response = await fetch("/api/demo-session", { method: "POST" });
  const data = await response.json().catch(() => ({}));
  if (!response.ok || !data.token) {
    throw new Error(data.error || "Could not open the demo workspace.");
  }
  sessionStorage.setItem(tokenKey, data.token);
  return data;
}

async function api(path, options = {}) {
  let response = await fetchApi(path, options);
  if (response.status === 401 && path !== "/api/login" && path !== "/api/logout" &&
      path !== "/api/accounts" && path !== "/api/demo-session") {
    try {
      await startDemoSession();
      demoSessionWasRestored = true;
      response = await fetchApi(path, options);
    } catch (error) {
      sessionStorage.removeItem(tokenKey);
      signOut();
      throw new Error("The demo restarted. Choose Continue as demo visitor to open a fresh workspace.");
    }
  }

  const data = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(data.error || "The request could not be completed.");
  }
  return data;
}

function signOut(showLogin = true) {
  sessionStorage.removeItem(tokenKey);
  dashboardView.classList.add("hidden");
  if (showLogin) authView.classList.remove("hidden");
}

function setAuthMode(mode) {
  const createMode = mode === "create";
  document.querySelector("#login-form").classList.toggle("hidden", createMode);
  document.querySelector("#create-form").classList.toggle("hidden", !createMode);
  document.querySelector("#login-tab").classList.toggle("active", !createMode);
  document.querySelector("#create-tab").classList.toggle("active", createMode);
  document.querySelector("#login-tab").setAttribute("aria-selected", String(!createMode));
  document.querySelector("#create-tab").setAttribute("aria-selected", String(createMode));
  showMessage(authMessage, "");
}

async function loadDashboard() {
  const profile = await api("/api/me");
  document.querySelector("#customer-name").textContent = profile.name;
  document.querySelector("#account-number").textContent = profile.accountNumber;
  document.querySelector("#balance-value").textContent = formatMoney(profile.balancePaise);

  const transactions = await api("/api/statement");
  const body = document.querySelector("#statement-body");
  if (!transactions.length) {
    body.innerHTML = '<tr><td class="empty-state" colspan="4">No transactions yet.</td></tr>';
  } else {
    body.replaceChildren(...transactions.slice().reverse().map((item) => {
      const row = document.createElement("tr");
      const type = document.createElement("td");
      const date = document.createElement("td");
      const status = document.createElement("td");
      const amount = document.createElement("td");
      type.textContent = item.type;
      date.textContent = item.date;
      status.textContent = item.status;
      status.className = "status";
      amount.textContent = formatMoney(item.amountPaise);
      amount.className = "amount";
      row.append(type, date, status, amount);
      return row;
    }));
  }

  if (demoSessionWasRestored) {
    showMessage(transactionMessage, "The free demo restarted, so its previous temporary data was cleared. You are in a fresh demo workspace.");
    demoSessionWasRestored = false;
  }
}

function formatMoney(paise) {
  return `Rs. ${(paise / 100).toLocaleString("en-IN", { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
}

async function openDashboard() {
  await loadDashboard();
  authView.classList.add("hidden");
  dashboardView.classList.remove("hidden");
}

document.querySelector("#login-tab").addEventListener("click", () => setAuthMode("login"));
document.querySelector("#create-tab").addEventListener("click", () => setAuthMode("create"));

document.querySelector("#demo-button").addEventListener("click", async () => {
  const button = document.querySelector("#demo-button");
  button.disabled = true;
  showMessage(authMessage, "Opening a temporary demo workspace...");
  try {
    await startDemoSession();
    await openDashboard();
  } catch (error) {
    showMessage(authMessage, error.message, true);
  } finally {
    button.disabled = false;
  }
});

document.querySelector("#login-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  showMessage(authMessage, "Signing in...");
  const form = new FormData(event.currentTarget);
  try {
    const result = await api("/api/login", {
      method: "POST",
      body: JSON.stringify({
        accountNumber: Number(form.get("accountNumber")),
        password: form.get("password"),
      }),
    });
    sessionStorage.setItem(tokenKey, result.token);
    await openDashboard();
  } catch (error) {
    showMessage(authMessage, error.message, true);
  }
});

document.querySelector("#create-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  showMessage(authMessage, "Creating demo account...");
  const form = new FormData(event.currentTarget);
  const name = form.get("name");
  const password = form.get("password");
  try {
    const account = await api("/api/accounts", {
      method: "POST",
      body: JSON.stringify({ name, password }),
    });
    const login = await api("/api/login", {
      method: "POST",
      body: JSON.stringify({ accountNumber: account.accountNumber, password }),
    });
    sessionStorage.setItem(tokenKey, login.token);
    await openDashboard();
  } catch (error) {
    showMessage(authMessage, error.message, true);
  }
});

document.querySelectorAll(".action-tab").forEach((button) => {
  button.addEventListener("click", () => {
    activeAction = button.dataset.action;
    document.querySelectorAll(".action-tab").forEach((tab) => tab.classList.toggle("selected", tab === button));
    const destination = document.querySelector("#destination-field");
    destination.classList.toggle("hidden", activeAction !== "transfer");
    destination.querySelector("input").required = activeAction === "transfer";
    showMessage(transactionMessage, "");
  });
});

document.querySelector("#transaction-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const formElement = event.currentTarget;
  const form = new FormData(formElement);
  const payload = { amount: form.get("amount") };
  let endpoint = `/api/${activeAction}`;
  if (activeAction === "transfer") {
    endpoint = "/api/transfer";
    payload.destinationAccountNumber = Number(form.get("destinationAccountNumber"));
  }

  try {
    await api(endpoint, { method: "POST", body: JSON.stringify(payload) });
    formElement.reset();
    showMessage(transactionMessage, `${activeAction[0].toUpperCase()}${activeAction.slice(1)} completed.`);
    await loadDashboard();
  } catch (error) {
    showMessage(transactionMessage, error.message, true);
  }
});

document.querySelector("#password-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const formElement = event.currentTarget;
  const form = new FormData(formElement);
  try {
    await api("/api/password", {
      method: "POST",
      body: JSON.stringify({ oldPassword: form.get("oldPassword"), newPassword: form.get("newPassword") }),
    });
    formElement.reset();
    showMessage(passwordMessage, "Password updated.");
  } catch (error) {
    showMessage(passwordMessage, error.message, true);
  }
});

document.querySelector("#logout-button").addEventListener("click", async () => {
  try { await api("/api/logout", { method: "POST" }); } catch { /* Local sign-out still clears the session. */ }
  signOut();
});

document.querySelector("#refresh-button").addEventListener("click", async () => {
  try { await loadDashboard(); } catch (error) { showMessage(transactionMessage, error.message, true); }
});

document.querySelector("#close-account-button").addEventListener("click", async (event) => {
  if (!window.confirm("Close this account? It can only be closed with a zero balance, and this action cannot be undone.")) {
    return;
  }

  const button = event.currentTarget;
  button.disabled = true;
  showMessage(document.querySelector("#close-account-message"), "Closing account...");
  try {
    await api("/api/close-account", { method: "POST" });
    signOut();
    showMessage(authMessage, "Your zero-balance demo account has been closed.");
    setAuthMode("create");
  } catch (error) {
    showMessage(document.querySelector("#close-account-message"), error.message, true);
  } finally {
    button.disabled = false;
  }
});

if (sessionStorage.getItem(tokenKey)) {
  openDashboard().catch((error) => {
    signOut();
    showMessage(authMessage, error.message, true);
  });
}