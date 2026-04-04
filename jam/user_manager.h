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

class UserManager {
private:
    std::vector<User> users;

    // Загружает пользователей из файла в память.
    // Формат: login|password|fullName (по одной записи на строку)
    void loadFromFile() {
        users.clear();
        std::ifstream file(USERS_FILE);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            User u;
            std::getline(ss, u.login,    '|');
            std::getline(ss, u.password, '|');
            std::getline(ss, u.fullName, '|');
            if (!u.login.empty()) users.push_back(u);
        }
        file.close();
    }

    // Сохраняет всех пользователей из памяти в файл (перезапись)
    void saveToFile() const {
        std::ofstream file(USERS_FILE, std::ios::trunc);
        for (const auto& u : users)
            file << u.login << "|" << u.password << "|" << u.fullName << "\n";
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
