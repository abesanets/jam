#pragma once

// ============================================================
// order_manager.h — CRUD-операции с заказами, хранение в orders.txt
// Формат: id|clientName|phone|description|price|status|dateReceived|deadline|master
// ============================================================

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <windows.h>
#include "models.h"

const std::string ORDERS_FILE = "orders.txt";

// Возвращает текущую дату в формате DD.MM.YYYY
static std::string getCurrentDate() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[16];
    sprintf(buf, "%02d.%02d.%04d", st.wDay, st.wMonth, st.wYear);
    return std::string(buf);
}

class OrderManager {
private:
    std::vector<Order> orders;
    int nextId;

    void loadFromFile() {
        orders.clear();
        nextId = 1;
        std::ifstream file(ORDERS_FILE);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            Order o;
            std::string idStr, priceStr;
            std::getline(ss, idStr,         '|');
            std::getline(ss, o.clientName,  '|');
            std::getline(ss, o.phone,       '|');
            std::getline(ss, o.description, '|');
            std::getline(ss, priceStr,      '|');
            std::getline(ss, o.status,      '|');
            std::getline(ss, o.dateReceived,'|');
            std::getline(ss, o.deadline,    '|');
            std::getline(ss, o.master,      '|');
            try {
                o.id    = std::stoi(idStr);
                o.price = std::stod(priceStr);
            } catch (...) { continue; }
            orders.push_back(o);
            if (o.id >= nextId) nextId = o.id + 1;
        }
        file.close();
    }

    void saveToFile() const {
        std::ofstream file(ORDERS_FILE, std::ios::trunc);
        for (const auto& o : orders)
            file << o.id << "|" << o.clientName << "|" << o.phone << "|"
                 << o.description << "|" << o.price << "|" << o.status << "|"
                 << o.dateReceived << "|" << o.deadline << "|" << o.master << "\n";
        file.close();
    }

public:
    OrderManager() { loadFromFile(); }

    int addOrder(const std::string& clientName, const std::string& phone,
                 const std::string& description, double price,
                 const std::string& deadline, const std::string& master) {
        Order o;
        o.id           = nextId++;
        o.clientName   = clientName;
        o.phone        = phone;
        o.description  = description;
        o.price        = price;
        o.status       = "Новый";
        o.dateReceived = getCurrentDate();
        o.deadline     = deadline;
        o.master       = master;
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
            if (it->id == id) { orders.erase(it); saveToFile(); return true; }
        }
        return false;
    }

    bool updateOrder(int id, const std::string& clientName, const std::string& phone,
                     const std::string& description, double price, const std::string& deadline) {
        Order* o = findById(id);
        if (!o) return false;
        o->clientName   = clientName;
        o->phone        = phone;
        o->description  = description;
        o->price        = price;
        o->deadline     = deadline;
        saveToFile();
        return true;
    }

    // Фильтрация: статус (пустая строка = все), просрочен (0=все, 1=просрочен, 2=активен)
    // Сортировка: 0=по ID, 1=по дедлайну
    std::vector<Order> getFiltered(const std::string& statusFilter,
                                   int overdueFilter,
                                   int sortMode) const {
        SYSTEMTIME st;
        GetLocalTime(&st);
        // текущая дата как число YYYYMMDD для сравнения
        int today = st.wYear * 10000 + st.wMonth * 100 + st.wDay;

        auto dateToInt = [](const std::string& d) -> int {
            // формат DD.MM.YYYY
            if (d.size() < 10) return 0;
            try {
                int day  = std::stoi(d.substr(0, 2));
                int mon  = std::stoi(d.substr(3, 2));
                int year = std::stoi(d.substr(6, 4));
                return year * 10000 + mon * 100 + day;
            } catch (...) { return 0; }
        };

        std::vector<Order> result;
        for (const auto& o : orders) {
            if (!statusFilter.empty() && o.status != statusFilter) continue;
            if (overdueFilter != 0) {
                int dl = dateToInt(o.deadline);
                bool overdue = (dl > 0 && dl < today && o.status != "Выдан");
                if (overdueFilter == 1 && !overdue) continue;
                if (overdueFilter == 2 &&  overdue) continue;
            }
            result.push_back(o);
        }

        if (sortMode == 1) {
            // сортировка по дедлайну
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
