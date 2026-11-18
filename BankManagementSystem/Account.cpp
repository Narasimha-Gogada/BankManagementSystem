#include "Account.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr int passwordIterations = 210000;
    constexpr int saltLength = 16;
    constexpr int hashLength = 32;

    std::string toHex(const unsigned char* bytes, int length)
    {
        static const char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(static_cast<std::size_t>(length) * 2);

        for (int index = 0; index < length; ++index)
        {
            result.push_back(digits[bytes[index] >> 4]);
            result.push_back(digits[bytes[index] & 0x0f]);
        }

        return result;
    }

    bool fromHex(const std::string& value, unsigned char* output, int length)
    {
        if (value.size() != static_cast<std::size_t>(length) * 2)
        {
            return false;
        }

        for (int index = 0; index < length; ++index)
        {
            unsigned int byte = 0;
            try
            {
                byte = static_cast<unsigned int>(
                    std::stoul(value.substr(static_cast<std::size_t>(index) * 2, 2),
                               nullptr,
                               16));
            }
            catch (const std::exception&)
            {
                return false;
            }

            output[index] = static_cast<unsigned char>(byte);
        }

        return true;
    }

    std::string hashPassword(const std::string& password)
    {
        unsigned char salt[saltLength];
        unsigned char derived[hashLength];

        if (RAND_bytes(salt, sizeof(salt)) != 1 ||
            PKCS5_PBKDF2_HMAC(
                password.data(),
                static_cast<int>(password.size()),
                salt,
                sizeof(salt),
                passwordIterations,
                EVP_sha256(),
                sizeof(derived),
                derived) != 1)
        {
            throw std::runtime_error("Unable to securely process password");
        }

        return "pbkdf2$" + std::to_string(passwordIterations) + "$" +
               toHex(salt, sizeof(salt)) + "$" + toHex(derived, sizeof(derived));
    }

    bool verifyPasswordHash(
        const std::string& stored,
        const std::string& password
    )
    {
        const std::string prefix = "pbkdf2$";
        const std::size_t iterationsEnd = stored.find('$', prefix.size());

        if (stored.compare(0, prefix.size(), prefix) != 0 ||
            iterationsEnd == std::string::npos)
        {
            return false;
        }

        const std::size_t saltEnd = stored.find('$', iterationsEnd + 1);
        if (saltEnd == std::string::npos)
        {
            return false;
        }

        int iterations = 0;
        try
        {
            iterations = std::stoi(
                stored.substr(prefix.size(), iterationsEnd - prefix.size()));
        }
        catch (const std::exception&)
        {
            return false;
        }

        if (iterations < 100000 || iterations > 1000000)
        {
            return false;
        }

        unsigned char salt[saltLength];
        unsigned char expected[hashLength];
        unsigned char actual[hashLength];

        if (!fromHex(stored.substr(iterationsEnd + 1, saltEnd - iterationsEnd - 1),
                     salt,
                     sizeof(salt)) ||
            !fromHex(stored.substr(saltEnd + 1), expected, sizeof(expected)) ||
            PKCS5_PBKDF2_HMAC(
                password.data(),
                static_cast<int>(password.size()),
                salt,
                sizeof(salt),
                iterations,
                EVP_sha256(),
                sizeof(actual),
                actual) != 1)
        {
            return false;
        }

        return CRYPTO_memcmp(expected, actual, sizeof(expected)) == 0;
    }
}

// Constructor
Account::Account(
    int accountNumber,
    const std::string& customerName,
    const std::string& password
)
    : accountNumber(accountNumber),
      customerName(customerName),
    passwordHash(hashPassword(password)),
      balance(0),
      active(true)
{
}

// Get account number
int Account::getAccountNumber() const
{
    return accountNumber;
}

// Get customer name
std::string Account::getCustomerName() const
{
    return customerName;
}

// Get account balance in paise
long long Account::getBalance() const
{
    return balance;
}

// Check whether account is active
bool Account::isActive() const
{
    return active;
}

// Verify password against stored password data
bool Account::verifyPassword(const std::string& password) const
{
    return active && verifyPasswordHash(passwordHash, password);
}

// Deposit money
bool Account::deposit(long long amount)
{
    if (!active || amount <= 0)
    {
        return false;
    }

    // Prevent integer overflow
    if (balance > std::numeric_limits<long long>::max() - amount)
    {
        return false;
    }

    balance += amount;

    return true;
}

// Withdraw money
bool Account::withdraw(long long amount)
{
    if (!active || amount <= 0)
    {
        return false;
    }

    if (amount > balance)
    {
        return false;
    }

    balance -= amount;

    return true;
}

// Change password data
bool Account::changePassword(const std::string& newPasswordHash)
{
    if (!active || newPasswordHash.empty())
    {
        return false;
    }

    passwordHash = hashPassword(newPasswordHash);

    return true;
}

// Close account
bool Account::closeAccount()
{
    if (!active || balance != 0)
    {
        return false;
    }

    active = false;

    return true;
}