#pragma once

// ============================================================
// user_manager.h — Класс UserManager
// Отвечает за регистрацию, авторизацию и хранение пользователей
// в файле users.txt. Полностью изолирован от UI.
// ============================================================

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "models.h"

const std::string USERS_FILE = "users.txt";

// Простое XOR-шифрование строки с ключом
static std::string xorCipher(const std::string& data) {
    const std::string key = "J4M_s3cr3t_k3y!";
    std::string result = data;
    for (size_t i = 0; i < data.size(); ++i)
        result[i] = data[i] ^ key[i % key.size()];
    return result;
}

// Кодирует бинарную строку в hex для безопасного хранения в файле
static std::string toHex(const std::string& data) {
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (unsigned char c : data) {
        out += hex[c >> 4];
        out += hex[c & 0xF];
    }
    return out;
}

// Декодирует hex обратно в бинарную строку
static std::string fromHex(const std::string& hex) {
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        auto val = [](char c) -> unsigned char {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        out += (char)((val(hex[i]) << 4) | val(hex[i+1]));
    }
    return out;
}

static std::string encrypt(const std::string& s) { return toHex(xorCipher(s)); }
static std::string decrypt(const std::string& s) { return xorCipher(fromHex(s)); }

class UserManager {
private:
    std::vector<User> users;

    // Загружает пользователей из файла в память.
    // Формат строки: зашифрованный hex(login)|hex(password)|hex(fullName)
    void loadFromFile() {
        users.clear();
        std::ifstream file(USERS_FILE);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string eLogin, ePassword, eFullName;
            std::getline(ss, eLogin,     '|');
            std::getline(ss, ePassword,  '|');
            std::getline(ss, eFullName,  '|');
            if (eLogin.empty()) continue;
            User u;
            // Пробуем дешифровать; если строка не hex (старый формат) — берём как есть
            u.login    = (eLogin.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos && eLogin.size() % 2 == 0)
                            ? decrypt(eLogin) : eLogin;
            u.password = (ePassword.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos && ePassword.size() % 2 == 0)
                            ? decrypt(ePassword) : ePassword;
            u.fullName = (eFullName.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos && eFullName.size() % 2 == 0)
                            ? decrypt(eFullName) : eFullName;
            users.push_back(u);
        }
        file.close();
    }

    // Сохраняет всех пользователей из памяти в файл (перезапись, данные зашифрованы)
    void saveToFile() const {
        std::ofstream file(USERS_FILE, std::ios::trunc);
        for (const auto& u : users)
            file << encrypt(u.login) << "|" << encrypt(u.password) << "|" << encrypt(u.fullName) << "\n";
        file.close();
    }

public:
    // Конструктор: при создании сразу загружает данные из файла
    UserManager() { loadFromFile(); }

    // Регистрирует нового пользователя.
    // Возвращает false если логин занят или поля пустые.
    bool registerUser(const std::string& login, const std::string& password, const std::string& fullName) {
        if (login.empty() || password.empty() || fullName.empty()) return false;
        for (const auto& u : users)
            if (u.login == login) return false;
        users.push_back({login, password, fullName});
        saveToFile();
        return true;
    }

    // Проверяет пару логин/пароль.
    // Возвращает true если пользователь найден.
    bool loginUser(const std::string& login, const std::string& password) const {
        for (const auto& u : users)
            if (u.login == login && u.password == password) return true;
        return false;
    }

    // Возвращает ФИО по логину
    std::string getFullName(const std::string& login) const {
        for (const auto& u : users)
            if (u.login == login) return u.fullName;
        return login;
    }
};
