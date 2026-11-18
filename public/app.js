const authView = document.querySelector("#auth-view");
const dashboardView = document.querySelector("#dashboard-view");
const authMessage = document.querySelector("#auth-message");
const transactionMessage = document.querySelector("#transaction-message");
const passwordMessage = document.querySelector("#password-message");
const tokenKey = "fieldnote-demo-token";
let activeAction = "deposit";

function showMessage(element, message, isError = false) {
  element.textContent = message;
  element.classList.toggle("error", isError);
}

async function api(path, options = {}) {
  const headers = { "Content-Type": "application/json", ...options.headers };
  const token = sessionStorage.getItem(tokenKey);
  if (token) headers.Authorization = `Bearer ${token}`;

  const response = await fetch(path, { ...options, headers });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) {
    if (response.status === 401) signOut();
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
  const form = new FormData(event.currentTarget);
  const payload = { amount: form.get("amount") };
  let endpoint = `/api/${activeAction}`;
  if (activeAction === "transfer") {
    endpoint = "/api/transfer";
    payload.destinationAccountNumber = Number(form.get("destinationAccountNumber"));
  }

  try {
    await api(endpoint, { method: "POST", body: JSON.stringify(payload) });
    event.currentTarget.reset();
    showMessage(transactionMessage, `${activeAction[0].toUpperCase()}${activeAction.slice(1)} completed.`);
    await loadDashboard();
  } catch (error) {
    showMessage(transactionMessage, error.message, true);
  }
});

document.querySelector("#password-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  try {
    await api("/api/password", {
      method: "POST",
      body: JSON.stringify({ oldPassword: form.get("oldPassword"), newPassword: form.get("newPassword") }),
    });
    event.currentTarget.reset();
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

if (sessionStorage.getItem(tokenKey)) {
  openDashboard().catch(() => signOut());
}