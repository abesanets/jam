#pragma once

// ============================================================
// storage.h — всё, что связано с файлами: XOR+hex, escape |,
// разбор дат дедлайна, UserManager и OrderManager.
// ============================================================

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

#include "models.h"

namespace JamFileCrypto {

constexpr char XOR_KEY[] = "J4M_s3cr3t_k3y!";

inline std::string xorCipher(const std::string& data) {
    std::string key(XOR_KEY, sizeof(XOR_KEY) - 1);
    std::string result = data;
    for (size_t i = 0; i < data.size(); ++i)
        result[i] = static_cast<char>(static_cast<unsigned char>(data[i]) ^
                                      static_cast<unsigned char>(key[i % key.size()]));
    return result;
}

inline std::string toHex(const std::string& data) {
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (unsigned char c : data) {
        out += hex[c >> 4];
        out += hex[c & 0xF];
    }
    return out;
}

inline std::string fromHex(const std::string& hex) {
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        auto val = [](char c) -> unsigned char {
            if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
            return 0;
        };
        out += static_cast<char>((val(hex[i]) << 4) | val(hex[i + 1]));
    }
    return out;
}

inline std::string encryptHex(const std::string& s) { return toHex(xorCipher(s)); }

inline std::string decryptHex(const std::string& s) { return xorCipher(fromHex(s)); }

inline bool isHexStringEven(const std::string& s) {
    if (s.empty() || (s.size() % 2) != 0) return false;
    for (char c : s) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

} // namespace JamFileCrypto

// ---------- escape полей (| и \) для внутренней plaintext-строки ----------

inline std::string escapeStorageField(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '|') out += "\\|";
        else out += static_cast<char>(c);
    }
    return out;
}

inline std::vector<std::string> splitStorageLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    for (size_t i = 0; i < line.size();) {
        if (line[i] == '\\' && i + 1 < line.size()) {
            if (line[i + 1] == '|') {
                cur += '|';
                i += 2;
                continue;
            }
            if (line[i + 1] == '\\') {
                cur += '\\';
                i += 2;
                continue;
            }
        }
        if (line[i] == '|') {
            fields.push_back(cur);
            cur.clear();
            ++i;
            continue;
        }
        cur += line[i++];
    }
    fields.push_back(cur);
    return fields;
}

inline std::string trimStorageLine(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

// Целая строка файла — hex без «голых» | (XOR+hex внутренней строки).
inline bool looksLikeEncryptedWholeLine(const std::string& line) {
    if (line.find('|') != std::string::npos) return false;
    return JamFileCrypto::isHexStringEven(line) && line.size() >= 16;
}

// ---------- даты дедлайна → ДД.ММ.ГГГГ ----------

inline std::string trimDateString(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

inline bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

inline int daysInMonth(int m, int y) {
    static const int d[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && isLeapYear(y)) return 29;
    return d[m];
}

inline bool calendarOk(int d, int m, int y) {
    if (y < 1900 || y > 2100) return false;
    if (m < 1 || m > 12) return false;
    int dim = daysInMonth(m, y);
    return d >= 1 && d <= dim;
}

inline bool parseNormalizeDeadline(const std::string& raw, std::string& out) {
    std::string t = trimDateString(raw);
    if (t.empty()) {
        out.clear();
        return false;
    }

    int d = 0, m = 0, y = 0;

    if (t.size() >= 10 && t[4] == '-' && t[7] == '-') {
        try {
            y = std::stoi(t.substr(0, 4));
            m = std::stoi(t.substr(5, 2));
            d = std::stoi(t.substr(8, 2));
        } catch (...) {
            return false;
        }
        if (!calendarOk(d, m, y)) return false;
    } else {
        std::vector<int> nums;
        std::string num;
        for (size_t i = 0; i < t.size(); ++i) {
            char c = t[i];
            if (std::isdigit(static_cast<unsigned char>(c)))
                num += c;
            else {
                if (!num.empty()) {
                    try {
                        nums.push_back(std::stoi(num));
                    } catch (...) {
                        return false;
                    }
                    num.clear();
                }
            }
        }
        if (!num.empty()) {
            try {
                nums.push_back(std::stoi(num));
            } catch (...) {
                return false;
            }
        }
        if (nums.size() != 3) return false;

        if (nums[0] > 31) {
            y = nums[0];
            m = nums[1];
            d = nums[2];
        } else {
            d = nums[0];
            m = nums[1];
            y = nums[2];
            if (y < 100) y += 2000;
        }
        if (!calendarOk(d, m, y)) return false;
    }

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d.%02d.%04d", d, m, y);
    out = buf;
    return true;
}

// ---------- файлы и менеджеры ----------

const std::string USERS_FILE = "users.jam";
const std::string ORDERS_FILE = "orders.jam";
// Если .jam ещё нет — читаем старые .txt (после сохранения появятся только .jam)
const std::string USERS_FILE_LEGACY = "users.txt";
const std::string ORDERS_FILE_LEGACY = "orders.txt";

inline std::string getCurrentDate() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[16];
    sprintf(buf, "%02d.%02d.%04d", st.wDay, st.wMonth, st.wYear);
    return std::string(buf);
}

class UserManager {
private:
    std::vector<User> users;

    static std::string composePlainLine(const User& u) {
        return escapeStorageField(u.login) + "|" + escapeStorageField(u.password) + "|" +
               escapeStorageField(u.fullName);
    }

    static bool parsePlainLineToUser(const std::string& inner, User& u) {
        std::vector<std::string> p = splitStorageLine(inner);
        if (p.size() != 3) return false;
        u.login = p[0];
        u.password = p[1];
        u.fullName = p[2];
        return true;
    }

    void loadFromFile() {
        users.clear();
        std::ifstream file(USERS_FILE);
        if (!file.is_open()) {
            file.clear();
            file.open(USERS_FILE_LEGACY);
        }
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            line = trimStorageLine(line);
            if (line.empty()) continue;

            if (looksLikeEncryptedWholeLine(line)) {
                std::string inner = JamFileCrypto::decryptHex(line);
                User u;
                if (parsePlainLineToUser(inner, u) && !u.login.empty()) users.push_back(u);
                continue;
            }

            // Старый формат: hex(login)|hex(password)|hex(fullName)
            if (line.find('|') != std::string::npos) {
                std::istringstream ss(line);
                std::string eLogin, ePassword, eFullName;
                std::getline(ss, eLogin, '|');
                std::getline(ss, ePassword, '|');
                std::getline(ss, eFullName, '|');
                if (eLogin.empty()) continue;
                User u;
                u.login = JamFileCrypto::isHexStringEven(eLogin) ? JamFileCrypto::decryptHex(eLogin) : eLogin;
                u.password =
                    JamFileCrypto::isHexStringEven(ePassword) ? JamFileCrypto::decryptHex(ePassword) : ePassword;
                u.fullName =
                    JamFileCrypto::isHexStringEven(eFullName) ? JamFileCrypto::decryptHex(eFullName) : eFullName;
                users.push_back(u);
            }
        }
        file.close();
    }

    void saveToFile() const {
        std::ofstream file(USERS_FILE, std::ios::trunc);
        for (const auto& u : users) {
            std::string plain = composePlainLine(u);
            file << JamFileCrypto::encryptHex(plain) << "\n";
        }
        file.close();
    }

public:
    UserManager() { loadFromFile(); }

    bool registerUser(const std::string& login, const std::string& password, const std::string& fullName) {
        if (login.empty() || password.empty() || fullName.empty()) return false;
        for (const auto& u : users)
            if (u.login == login) return false;
        users.push_back({login, password, fullName});
        saveToFile();
        return true;
    }

    bool loginUser(const std::string& login, const std::string& password) const {
        for (const auto& u : users)
            if (u.login == login && u.password == password) return true;
        return false;
    }

    std::string getFullName(const std::string& login) const {
        for (const auto& u : users)
            if (u.login == login) return u.fullName;
        return login;
    }
};

class OrderManager {
private:
    std::vector<Order> orders;
    int nextId;

    static std::string composePlainLine(const Order& o) {
        std::ostringstream ps;
        ps << std::fixed << std::setprecision(2) << o.price;
        return escapeStorageField(std::to_string(o.id)) + "|" + escapeStorageField(o.clientName) + "|" +
               escapeStorageField(o.phone) + "|" + escapeStorageField(o.description) + "|" +
               escapeStorageField(ps.str()) + "|" + escapeStorageField(o.status) + "|" +
               escapeStorageField(o.dateReceived) + "|" + escapeStorageField(o.deadline) + "|" +
               escapeStorageField(o.master);
    }

    static bool parsePlainLineToOrder(const std::string& inner, Order& o) {
        std::vector<std::string> p = splitStorageLine(inner);
        if (p.size() != 9) return false;
        try {
            o.id = std::stoi(p[0]);
            o.clientName = p[1];
            o.phone = p[2];
            o.description = p[3];
            o.price = std::stod(p[4]);
            o.status = p[5];
            o.dateReceived = p[6];
            o.deadline = p[7];
            o.master = p[8];
        } catch (...) {
            return false;
        }
        return true;
    }

    void loadFromFile() {
        orders.clear();
        nextId = 1;
        std::ifstream file(ORDERS_FILE);
        if (!file.is_open()) {
            file.clear();
            file.open(ORDERS_FILE_LEGACY);
        }
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            line = trimStorageLine(line);
            if (line.empty()) continue;

            std::string inner;
            if (looksLikeEncryptedWholeLine(line))
                inner = JamFileCrypto::decryptHex(line);
            else
                inner = line;

            Order o;
            if (!parsePlainLineToOrder(inner, o)) continue;
            orders.push_back(o);
            if (o.id >= nextId) nextId = o.id + 1;
        }
        file.close();
    }

    void saveToFile() const {
        std::ofstream file(ORDERS_FILE, std::ios::trunc);
        for (const auto& o : orders) {
            std::string plain = composePlainLine(o);
            file << JamFileCrypto::encryptHex(plain) << "\n";
        }
        file.close();
    }

public:
    OrderManager() { loadFromFile(); }

    int addOrder(const std::string& clientName, const std::string& phone, const std::string& description,
                 double price, const std::string& deadline, const std::string& master) {
        Order o;
        o.id = nextId++;
        o.clientName = clientName;
        o.phone = phone;
        o.description = description;
        o.price = price;
        o.status = "Новый";
        o.dateReceived = getCurrentDate();
        o.deadline = deadline;
        o.master = master;
        orders.push_back(o);
        saveToFile();
        return o.id;
    }

    std::vector<Order> getAllOrders() const { return orders; }

    Order* findById(int id) {
        for (auto& o : orders)
            if (o.id == id) return &o;
        return nullptr;
    }

    bool updateStatus(int id, const std::string& newStatus) {
        Order* o = findById(id);
        if (!o) return false;
        o->status = newStatus;
        saveToFile();
        return true;
    }

    bool deleteOrder(int id) {
        for (auto it = orders.begin(); it != orders.end(); ++it) {
            if (it->id == id) {
                orders.erase(it);
                saveToFile();
                return true;
            }
        }
        return false;
    }

    bool updateOrder(int id, const std::string& clientName, const std::string& phone,
                     const std::string& description, double price, const std::string& deadline) {
        Order* o = findById(id);
        if (!o) return false;
        o->clientName = clientName;
        o->phone = phone;
        o->description = description;
        o->price = price;
        o->deadline = deadline;
        saveToFile();
        return true;
    }

    std::vector<Order> getFiltered(const std::string& statusFilter, int overdueFilter, int sortMode) const {
        SYSTEMTIME st;
        GetLocalTime(&st);
        int today = st.wYear * 10000 + st.wMonth * 100 + st.wDay;

        auto dateToInt = [](const std::string& d) -> int {
            if (d.size() < 10) return 0;
            try {
                int day = std::stoi(d.substr(0, 2));
                int mon = std::stoi(d.substr(3, 2));
                int year = std::stoi(d.substr(6, 4));
                return year * 10000 + mon * 100 + day;
            } catch (...) {
                return 0;
            }
        };

        std::vector<Order> result;
        for (const auto& o : orders) {
            if (!statusFilter.empty() && o.status != statusFilter) continue;
            if (overdueFilter != 0) {
                int dl = dateToInt(o.deadline);
                bool overdue = (dl > 0 && dl < today && o.status != "Выдан");
                if (overdueFilter == 1 && !overdue) continue;
                if (overdueFilter == 2 && overdue) continue;
            }
            result.push_back(o);
        }

        if (sortMode == 1) {
            std::sort(result.begin(), result.end(), [&](const Order& a, const Order& b) {
                int da = dateToInt(a.deadline), db = dateToInt(b.deadline);
                if (da == 0) return false;
                if (db == 0) return true;
                return da < db;
            });
        }

        return result;
    }
};
