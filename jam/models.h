#pragma once
#include <string>

// ============================================================
// models.h — Структуры данных приложения J.A.M.
// ============================================================

struct User {
    std::string login;
    std::string password;
};

struct Order {
    int         id;
    std::string clientName;
    std::string phone;
    std::string description;
    double      price;
    std::string status;
    std::string dateReceived; // дата приёма (авто, формат DD.MM.YYYY)
    std::string deadline;     // дедлайн (вводится вручную, формат DD.MM.YYYY)
};

const std::string ORDER_STATUSES[] = {
    "Новый",
    "В работе",
    "Готов",
    "Выдан"
};
const int ORDER_STATUS_COUNT = 4;
