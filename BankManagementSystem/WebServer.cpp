#include "Bank.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>

#include <cstdlib>
#include <iomanip>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace
{
    using Json = nlohmann::json;

    Bank bank;
    std::mutex bankMutex;
    std::mutex sessionsMutex;
    std::unordered_map<std::string, int> sessions;

    void sendJson(httplib::Response& response, int status, const Json& body)
    {
        response.status = status;
        response.set_content(body.dump(), "application/json; charset=utf-8");
    }

    void sendError(httplib::Response& response, int status, const std::string& message)
    {
        sendJson(response, status, {{"error", message}});
    }

    bool parseJson(const httplib::Request& request, Json& body)
    {
        try
        {
            body = Json::parse(request.body);
            return body.is_object();
        }
        catch (const Json::exception&)
        {
            return false;
        }
    }

    std::string createToken()
    {
        unsigned char bytes[32];
        if (RAND_bytes(bytes, sizeof(bytes)) != 1)
        {
            return "";
        }

        std::ostringstream token;
        token << std::hex << std::setfill('0');
        for (unsigned char byte : bytes)
        {
            token << std::setw(2) << static_cast<unsigned int>(byte);
        }
        return token.str();
    }

    std::optional<int> authenticatedAccount(const httplib::Request& request)
    {
        if (!request.has_header("Authorization"))
        {
            return std::nullopt;
        }

        const std::string header = request.get_header_value("Authorization");
        const std::string prefix = "Bearer ";
        if (header.compare(0, prefix.size(), prefix) != 0)
        {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(sessionsMutex);
        const auto session = sessions.find(header.substr(prefix.size()));
        if (session == sessions.end())
        {
            return std::nullopt;
        }

        return session->second;
    }

    bool parseAmount(const Json& body, long long& amount)
    {
        if (!body.contains("amount") || !body["amount"].is_string())
        {
            return false;
        }

        const std::string input = body["amount"].get<std::string>();
        const std::size_t decimal = input.find('.');
        if (decimal != std::string::npos && input.find('.', decimal + 1) != std::string::npos)
        {
            return false;
        }

        const std::string whole = input.substr(0, decimal);
        const std::string fraction = decimal == std::string::npos
            ? ""
            : input.substr(decimal + 1);

        if (whole.empty() || fraction.size() > 2 ||
            (decimal != std::string::npos && fraction.empty()))
        {
            return false;
        }

        long long rupees = 0;
        const long long maxRupees = std::numeric_limits<long long>::max() / 100;
        for (char digit : whole)
        {
            if (digit < '0' || digit > '9')
            {
                return false;
            }
            const int value = digit - '0';
            if (rupees > (maxRupees - value) / 10)
            {
                return false;
            }
            rupees = rupees * 10 + value;
        }

        int paise = 0;
        if (!fraction.empty())
        {
            if (fraction[0] < '0' || fraction[0] > '9')
            {
                return false;
            }
            paise = (fraction[0] - '0') * 10;
        }
        if (fraction.size() == 2)
        {
            if (fraction[1] < '0' || fraction[1] > '9')
            {
                return false;
            }
            paise += fraction[1] - '0';
        }

        if (rupees == 0 && paise == 0 ||
            rupees > (std::numeric_limits<long long>::max() - paise) / 100)
        {
            return false;
        }

        amount = rupees * 100 + paise;
        return true;
    }

    Json transactionJson(const Transaction& transaction)
    {
        return {
            {"id", transaction.getTransactionId()},
            {"type", transaction.getTransactionTypeName()},
            {"amountPaise", transaction.getAmount()},
            {"date", transaction.getFormattedTimestamp()},
            {"status", transaction.getStatusName()}
        };
    }
}

int main()
{
    httplib::Server server;
    server.set_payload_max_length(1024 * 16);

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& response)
    {
        sendJson(response, 200, {{"status", "ok"}});
    });

    server.Post("/api/accounts", [](const httplib::Request& request, httplib::Response& response)
    {
        Json body;
        if (!parseJson(request, body) || !body.contains("name") ||
            !body["name"].is_string() || !body.contains("password") ||
            !body["password"].is_string())
        {
            sendError(response, 400, "Enter a name and password.");
            return;
        }

        const std::string name = body["name"].get<std::string>();
        const std::string password = body["password"].get<std::string>();
        if (name.empty() || name.size() > 80 || password.size() < 8 || password.size() > 128)
        {
            sendError(response, 400, "Use a name under 80 characters and a password of 8 to 128 characters.");
            return;
        }

        try
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            const int accountNumber = bank.createAccount(name, password);
            sendJson(response, 201, {{"accountNumber", accountNumber}});
        }
        catch (const std::exception&)
        {
            sendError(response, 500, "Could not securely create the account.");
        }
    });

    server.Post("/api/login", [](const httplib::Request& request, httplib::Response& response)
    {
        Json body;
        if (!parseJson(request, body) || !body.contains("accountNumber") ||
            !body["accountNumber"].is_number_integer() || !body.contains("password") ||
            !body["password"].is_string())
        {
            sendError(response, 400, "Enter an account number and password.");
            return;
        }

        const int accountNumber = body["accountNumber"].get<int>();
        const std::string password = body["password"].get<std::string>();
        std::string name;
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            if (!bank.authenticate(accountNumber, password))
            {
                sendError(response, 401, "Account number or password is incorrect.");
                return;
            }
            name = bank.getCustomerName(accountNumber);
        }

        const std::string token = createToken();
        if (token.empty())
        {
            sendError(response, 500, "Could not create a secure session.");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions[token] = accountNumber;
        }
        sendJson(response, 200, {
            {"token", token},
            {"accountNumber", accountNumber},
            {"name", name}
        });
    });

    server.Post("/api/logout", [](const httplib::Request& request, httplib::Response& response)
    {
        if (request.has_header("Authorization"))
        {
            const std::string header = request.get_header_value("Authorization");
            const std::string prefix = "Bearer ";
            if (header.compare(0, prefix.size(), prefix) == 0)
            {
                std::lock_guard<std::mutex> lock(sessionsMutex);
                sessions.erase(header.substr(prefix.size()));
            }
        }
        sendJson(response, 200, {{"status", "signed out"}});
    });

    server.Get("/api/me", [](const httplib::Request& request, httplib::Response& response)
    {
        const auto accountNumber = authenticatedAccount(request);
        if (!accountNumber)
        {
            sendError(response, 401, "Sign in to continue.");
            return;
        }

        long long balance = 0;
        std::string name;
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            if (!bank.checkBalance(*accountNumber, balance))
            {
                sendError(response, 404, "Account not found.");
                return;
            }
            name = bank.getCustomerName(*accountNumber);
        }
        sendJson(response, 200, {
            {"accountNumber", *accountNumber},
            {"name", name},
            {"balancePaise", balance}
        });
    });

    const auto moneyAction = [](const httplib::Request& request,
                                httplib::Response& response,
                                const std::string& action)
    {
        const auto accountNumber = authenticatedAccount(request);
        if (!accountNumber)
        {
            sendError(response, 401, "Sign in to continue.");
            return;
        }

        Json body;
        long long amount = 0;
        if (!parseJson(request, body) || !parseAmount(body, amount))
        {
            sendError(response, 400, "Enter a valid positive amount, such as 25.00.");
            return;
        }

        bool succeeded = false;
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            if (action == "deposit")
            {
                succeeded = bank.deposit(*accountNumber, amount);
            }
            else
            {
                succeeded = bank.withdraw(*accountNumber, amount);
            }
        }

        if (!succeeded)
        {
            sendError(response, 400, action == "deposit"
                ? "Deposit could not be completed."
                : "Withdrawal failed. Check your balance and amount.");
            return;
        }
        sendJson(response, 200, {{"status", "success"}});
    };

    server.Post("/api/deposit", [moneyAction](const httplib::Request& request, httplib::Response& response)
    {
        moneyAction(request, response, "deposit");
    });
    server.Post("/api/withdraw", [moneyAction](const httplib::Request& request, httplib::Response& response)
    {
        moneyAction(request, response, "withdraw");
    });

    server.Post("/api/transfer", [](const httplib::Request& request, httplib::Response& response)
    {
        const auto accountNumber = authenticatedAccount(request);
        if (!accountNumber)
        {
            sendError(response, 401, "Sign in to continue.");
            return;
        }

        Json body;
        long long amount = 0;
        if (!parseJson(request, body) || !body.contains("destinationAccountNumber") ||
            !body["destinationAccountNumber"].is_number_integer() ||
            !parseAmount(body, amount))
        {
            sendError(response, 400, "Enter a valid recipient account number and amount.");
            return;
        }

        const int destination = body["destinationAccountNumber"].get<int>();
        bool succeeded = false;
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            succeeded = bank.transfer(*accountNumber, destination, amount);
        }

        if (!succeeded)
        {
            sendError(response, 400, "Transfer failed. Check the recipient and available balance.");
            return;
        }
        sendJson(response, 200, {{"status", "success"}});
    });

    server.Get("/api/statement", [](const httplib::Request& request, httplib::Response& response)
    {
        const auto accountNumber = authenticatedAccount(request);
        if (!accountNumber)
        {
            sendError(response, 401, "Sign in to continue.");
            return;
        }

        Json statement = Json::array();
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            for (const Transaction& transaction : bank.getMiniStatement(*accountNumber, 10))
            {
                statement.push_back(transactionJson(transaction));
            }
        }
        sendJson(response, 200, statement);
    });

    server.Post("/api/password", [](const httplib::Request& request, httplib::Response& response)
    {
        const auto accountNumber = authenticatedAccount(request);
        if (!accountNumber)
        {
            sendError(response, 401, "Sign in to continue.");
            return;
        }

        Json body;
        if (!parseJson(request, body) || !body.contains("oldPassword") ||
            !body["oldPassword"].is_string() || !body.contains("newPassword") ||
            !body["newPassword"].is_string() || body["newPassword"].get<std::string>().size() < 8)
        {
            sendError(response, 400, "Enter your current password and a new password of at least 8 characters.");
            return;
        }

        bool changed = false;
        try
        {
            std::lock_guard<std::mutex> lock(bankMutex);
            changed = bank.changePassword(
                *accountNumber,
                body["oldPassword"].get<std::string>(),
                body["newPassword"].get<std::string>());
        }
        catch (const std::exception&)
        {
            sendError(response, 500, "Could not securely update the password.");
            return;
        }

        if (!changed)
        {
            sendError(response, 400, "Password update failed. Check your current password.");
            return;
        }
        sendJson(response, 200, {{"status", "success"}});
    });

    server.set_mount_point("/", "public");

    const char* portValue = std::getenv("PORT");
    int port = 10000;
    if (portValue != nullptr)
    {
        try
        {
            port = std::stoi(portValue);
        }
        catch (const std::exception&)
        {
            return 1;
        }
    }

    if (port < 1 || port > 65535 || !server.listen("0.0.0.0", port))
    {
        return 1;
    }

    return 0;
}