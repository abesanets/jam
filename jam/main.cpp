// ============================================================
// main.cpp — Точка входа приложения J.A.M.
// Jewelry Atelier Manager — АРМ ювелирной мастерской
//
// Компиляция (MinGW/GCC):
//   g++ -o jam.exe main.cpp -std=c++14 -finput-charset=UTF-8
// ============================================================

#include <windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>

#include "models.h"
#include "ui_manager.h"
#include "user_manager.h"
#include "order_manager.h"

const int KEY_ESC = 27;

// forward declaration
void screenManageOrder(OrderManager& om, int id, const std::string& masterName);

// ============================================================
// Экран: Добавление нового заказа
// ============================================================
void screenAddOrder(OrderManager& om, const std::string& masterName) {
    UIManager::clearScreen();
    UIManager::hideCursor();
    UIManager::printCentered(1, "=== Добавление нового заказа ===", Color::LOGO);
    UIManager::printCentered(2, "Введите данные заказа:", Color::MENU);
    UIManager::printHLine(3, 44, '-', Color::DIM);
    UIManager::printCentered(4, "[ESC] на любом поле — отмена", Color::DIM);

    std::string clientName, phone, description, priceStr, deadline;
    if (!UIManager::inputStringESC(6,  "  ФИО клиента:           ", clientName))  return;
    if (!UIManager::inputStringESC(8,  "  Телефон:               ", phone))        return;
    if (!UIManager::inputStringESC(10, "  Описание изделия:      ", description))  return;
    if (!UIManager::inputStringESC(12, "  Цена работы (руб):     ", priceStr))     return;
    if (!UIManager::inputStringESC(14, "  Дедлайн (ДД.ММ.ГГГГ): ", deadline))     return;

    if (clientName.empty() || phone.empty() || description.empty()) {
        UIManager::printError(16, "Все поля обязательны для заполнения!");
        UIManager::waitKey(18);
        return;
    }
    double price = 0.0;
    try { price = std::stod(priceStr); } catch (...) {}

    int newId = om.addOrder(clientName, phone, description, price, deadline, masterName);
    UIManager::printSuccess(16, "Заказ добавлен! ID: " + std::to_string(newId));
    UIManager::waitKey(18);
}

// ============================================================
// Экран: Просмотр заказов с фильтрами
// ============================================================
// Экран: Управление заказами (просмотр + редактирование)
// ============================================================
void screenOrders(OrderManager& om, const std::string& masterName) {
    std::string statusFilter = "";
    int sortMode = 0;
    bool showIssued = false;
    bool myOnly = false;
    int sel = 0;

    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Управление заказами ===", Color::LOGO);

        std::string fi = myOnly ? "Мои  |  " : "Все  |  ";
        fi += "Статус: ";
        fi += statusFilter.empty() ? "все активные" : statusFilter;
        fi += "  |  Дедлайн: ";
        fi += (sortMode == 0) ? "—" : (sortMode == 1 ? "ближе сначала" : "дальше сначала");
        fi += showIssued ? "  |  [выданные]" : "";
        UIManager::printCentered(2, fi, Color::DIM);
        UIManager::printCentered(3, "[M]ои [N]овый [W]работе [G]отов [V]ыданные [D]едлайн [R]сброс [ESC]назад", Color::MENU);
        UIManager::printHLine(4, 80, '-', Color::DIM);

        auto applyMyFilter = [&](std::vector<Order>& orders) {
            if (myOnly)
                orders.erase(std::remove_if(orders.begin(), orders.end(),
                    [&](const Order& o){ return o.master != masterName; }), orders.end());
        };

        std::vector<Order> orders;
        if (showIssued) {
            orders = om.getFiltered("Выдан", 0, 0);
            applyMyFilter(orders);
            UIManager::printCentered(5, "Выданных: " + std::to_string(orders.size()), Color::DIM);
        } else {
            orders = om.getFiltered(statusFilter, 0, sortMode == 0 ? 0 : 1);
            if (statusFilter.empty())
                orders.erase(std::remove_if(orders.begin(), orders.end(),
                    [](const Order& o){ return o.status == "Выдан"; }), orders.end());
            applyMyFilter(orders);
            if (sortMode == 2) std::reverse(orders.begin(), orders.end());
            UIManager::printCentered(5, "Показано: " + std::to_string(orders.size()), Color::MENU);
        }

        if (sel >= (int)orders.size()) sel = 0;

        if (orders.empty()) {
            UIManager::drawOrdersTable(orders, 6);
            int k = UIManager::normalizeKey(_getch());
            if (k == KEY_ESC) return;
            if (k == 'm') myOnly = !myOnly;
            else if (k == 'v') { showIssued = !showIssued; statusFilter = ""; sortMode = 0; }
            else if (k == 'r') { statusFilter = ""; sortMode = 0; showIssued = false; myOnly = false; }
            continue;
        }

        int res = UIManager::selectFromTable(orders, 6, sel, "mnvdrgw");
        if (res == -1) return;
        if (res >= 0) {
            screenManageOrder(om, orders[res].id, masterName);
            continue;
        }
        // extraKey нажата — sel содержит отрицательный код символа
        char ek = (char)(-(sel));
        sel = 0;
        if      (ek == 'm') myOnly = !myOnly;
        else if (ek == 'd') sortMode = (sortMode + 1) % 3;
        else if (ek == 'v') { showIssued = !showIssued; statusFilter = ""; sortMode = 0; }
        else if (ek == 'r') { statusFilter = ""; sortMode = 0; showIssued = false; myOnly = false; }
        else if (!showIssued) {
            if      (ek == 'n') statusFilter = (statusFilter == "Новый")    ? "" : "Новый";
            else if (ek == 'w') statusFilter = (statusFilter == "В работе") ? "" : "В работе";
            else if (ek == 'g') statusFilter = (statusFilter == "Готов")    ? "" : "Готов";
        }
    }
}

// ============================================================
// Экран: Подменю конкретного заказа
// ============================================================
void screenManageOrder(OrderManager& om, int id, const std::string& masterName) {
    while (true) {
        Order* o = om.findById(id);
        if (!o) return;

        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::drawOrderDetail(*o, 1);

        bool isOwner = (o->master == masterName);
        int menuRow = 15;
        UIManager::printHLine(menuRow++, 46, '-', Color::DIM);

        if (isOwner) {
            std::vector<std::string> actions = {
                " Изменить статус ",
                " Редактировать   ",
                " Удалить заказ   "
            };
            int act = UIManager::selectMenu(actions, menuRow);
            if (act == -1) return;
            if (act == 0) {
            UIManager::clearScreen();
            UIManager::printCentered(1, "=== Изменение статуса ===", Color::LOGO);
            UIManager::printCentered(2, "Заказ #" + std::to_string(o->id) + " — " + o->clientName, Color::DEFAULT);
            UIManager::printCentered(3, "Текущий статус: " + o->status, Color::HIGHLIGHT);
            UIManager::printHLine(4, 36, '-', Color::DIM);
            {
                std::vector<std::string> statuses;
                for (int i = 0; i < ORDER_STATUS_COUNT; i++) statuses.push_back(" " + ORDER_STATUSES[i] + " ");
                int sch = UIManager::selectMenu(statuses, 5);
                if (sch >= 0) {
                    om.updateStatus(id, ORDER_STATUSES[sch]);
                    UIManager::printSuccess(6 + ORDER_STATUS_COUNT, "Статус изменён на: " + ORDER_STATUSES[sch]);
                    UIManager::waitKey(8 + ORDER_STATUS_COUNT);
                }
            }
        } else if (act == 1) {
            UIManager::clearScreen();
            UIManager::hideCursor();
            UIManager::printCentered(1, "=== Редактирование заказа #" + std::to_string(o->id) + " ===", Color::LOGO);
            UIManager::printCentered(2, "Оставьте поле пустым — значение не изменится", Color::DIM);
            UIManager::printHLine(3, 50, '-', Color::DIM);

            std::string clientName, phone, description, priceStr, deadline;
            UIManager::inputStringESC(5,  "  ФИО клиента [" + o->clientName + "]: ", clientName);
            UIManager::inputStringESC(7,  "  Телефон [" + o->phone + "]: ", phone);
            UIManager::inputStringESC(9,  "  Описание [" + o->description + "]: ", description);
            std::ostringstream ps; ps << std::fixed << std::setprecision(2) << o->price;
            UIManager::inputStringESC(11, "  Цена [" + ps.str() + "]: ", priceStr);
            UIManager::inputStringESC(13, "  Дедлайн [" + o->deadline + "]: ", deadline);

            double newPrice = priceStr.empty() ? o->price : 0.0;
            if (!priceStr.empty()) try { newPrice = std::stod(priceStr); } catch (...) { newPrice = o->price; }

            om.updateOrder(id,
                clientName.empty()  ? o->clientName  : clientName,
                phone.empty()       ? o->phone       : phone,
                description.empty() ? o->description : description,
                newPrice,
                deadline.empty()    ? o->deadline    : deadline);
            UIManager::printSuccess(15, "Заказ обновлён!");
            UIManager::waitKey(17);
        } else if (act == 2) {
            UIManager::clearScreen();
            UIManager::printCentered(3, "Удалить заказ #" + std::to_string(o->id) + " — " + o->clientName + "?", Color::OVERDUE);
            std::vector<std::string> confirm = { " Да, удалить ", " Отмена      " };
            int conf = UIManager::selectMenu(confirm, 5);
            if (conf == 0) {
                om.deleteOrder(id);
                UIManager::printSuccess(8, "Заказ удалён.");
                UIManager::waitKey(10);
                return;
            }
        }
        } else {
            UIManager::printCentered(menuRow,   "  Только просмотр  ", Color::DIM);
            UIManager::printCentered(menuRow+1, "  (чужой заказ)    ", Color::DIM);
            UIManager::printCentered(menuRow+3, "  [ESC] Назад      ", Color::DEFAULT);
            _getch();
            return;
        }
    }
}

// ============================================================
// Экран: Авторизация
// ============================================================
std::string screenLogin(UserManager& um) {
    UIManager::clearScreen();
    UIManager::hideCursor();
    UIManager::printCentered(1, "=== Вход в систему ===", Color::LOGO);
    UIManager::printHLine(2, 30, '-', Color::DIM);
    UIManager::printCentered(3, "[ESC] на любом поле — назад", Color::DIM);
    std::string login, password;
    if (!UIManager::inputStringESC(5, "  Логин:    ", login))    return "";
    if (!UIManager::inputStringESC(7, "  Пароль:   ", password)) return "";
    if (um.loginUser(login, password)) {
        UIManager::printSuccess(9, "Авторизация успешна!");
        UIManager::waitKey(11);
        return login;
    }
    UIManager::printError(9, "Неверный логин или пароль!");
    UIManager::waitKey(11);
    return "";
}

// ============================================================
// Экран: Статистика
// ============================================================
void screenStats(OrderManager& om) {
    UIManager::clearScreen();
    UIManager::hideCursor();
    UIManager::printCentered(1, "=== Статистика ===", Color::LOGO);
    UIManager::printHLine(2, 44, '-', Color::DIM);

    std::vector<Order> all = om.getAllOrders();

    int total = (int)all.size();
    int cntNew = 0, cntWork = 0, cntReady = 0, cntIssued = 0, cntOverdue = 0;
    double sumTotal = 0.0, sumIssued = 0.0, sumActive = 0.0;

    SYSTEMTIME st; GetLocalTime(&st);
    int today = st.wYear * 10000 + st.wMonth * 100 + st.wDay;
    auto dateToInt = [](const std::string& d) -> int {
        if (d.size() < 10) return 0;
        try { return std::stoi(d.substr(6,4))*10000 + std::stoi(d.substr(3,2))*100 + std::stoi(d.substr(0,2)); }
        catch (...) { return 0; }
    };

    for (const auto& o : all) {
        sumTotal += o.price;
        if      (o.status == "Новый")    cntNew++;
        else if (o.status == "В работе") cntWork++;
        else if (o.status == "Готов")    cntReady++;
        else if (o.status == "Выдан")  { cntIssued++; sumIssued += o.price; }
        if (o.status != "Выдан") sumActive += o.price;
        int dl = dateToInt(o.deadline);
        if (dl > 0 && dl < today && o.status != "Выдан") cntOverdue++;
    }

    auto printStat = [](int row, const std::string& label, const std::string& val, WORD col = Color::DEFAULT) {
        COORD sz = UIManager::getConsoleSize();
        int x = (sz.X - 44) / 2; if (x < 0) x = 0;
        UIManager::setCursor(x, row); UIManager::setColor(Color::TABLE_HDR);
        std::cout << label;
        UIManager::setCursor(x + 30, row); UIManager::setColor(col);
        std::cout << val;
    };

    int row = 4;
    printStat(row++, "Всего заказов:                ", std::to_string(total));
    row++;
    printStat(row++, "  Новых:                      ", std::to_string(cntNew),    Color::MENU);
    printStat(row++, "  В работе:                   ", std::to_string(cntWork),   Color::HIGHLIGHT);
    printStat(row++, "  Готовых:                    ", std::to_string(cntReady),  Color::SUCCESS);
    printStat(row++, "  Выданных:                   ", std::to_string(cntIssued), Color::DIM);
    row++;
    printStat(row++, "  Просроченных:               ", std::to_string(cntOverdue), cntOverdue > 0 ? Color::OVERDUE : Color::DEFAULT);
    row++;

    std::ostringstream s1, s2, s3;
    s1 << std::fixed << std::setprecision(2) << sumTotal;
    s2 << std::fixed << std::setprecision(2) << sumActive;
    s3 << std::fixed << std::setprecision(2) << sumIssued;
    printStat(row++, "Сумма всех заказов (руб):     ", s1.str());
    printStat(row++, "  Активных (руб):             ", s2.str(), Color::MENU);
    printStat(row++, "  Выданных (руб):             ", s3.str(), Color::DIM);

    UIManager::printHLine(row + 1, 44, '-', Color::DIM);
    UIManager::waitKey(row + 3);
}

// ============================================================
// Экран: Регистрация
// ============================================================
void screenRegister(UserManager& um) {
    UIManager::clearScreen();
    UIManager::hideCursor();
    UIManager::printCentered(1, "=== Регистрация ===", Color::LOGO);
    UIManager::printHLine(2, 30, '-', Color::DIM);
    UIManager::printCentered(3, "[ESC] на любом поле — назад", Color::DIM);
    std::string fullName, login, password, confirm;
    if (!UIManager::inputStringESC(5, "  Ваше ФИО:          ", fullName))  return;
    if (!UIManager::inputStringESC(7, "  Придумайте логин:  ", login))     return;
    if (!UIManager::inputStringESC(9, "  Придумайте пароль: ", password))  return;
    if (!UIManager::inputStringESC(11,"  Повторите пароль:  ", confirm))   return;
    if (password != confirm) {
        UIManager::printError(13, "Пароли не совпадают!");
        UIManager::waitKey(15);
        return;
    }
    if (um.registerUser(login, password, fullName))
        UIManager::printSuccess(13, "Регистрация прошла успешно! Войдите в систему.");
    else
        UIManager::printError(13, "Логин уже занят или поля пустые!");
    UIManager::waitKey(15);
}

// ============================================================
// Вспомогательная функция: нижний регистр UTF-8 строки (для поиска)
// ============================================================
static std::string toLowerUTF8(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) {
            r += (char)(c >= 'A' && c <= 'Z' ? c + 32 : c);
            ++i;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
            unsigned char c2 = (unsigned char)s[i+1];
            if (c == 0xD0 && c2 >= 0x90 && c2 <= 0xAF) { r += (char)0xD0; r += (char)(c2 + 0x20); i += 2; }
            else if (c == 0xD0 && c2 == 0x81)           { r += (char)0xD1; r += (char)0x91;        i += 2; }
            else { r += s[i++]; r += s[i++]; }
        } else { r += s[i++]; }
    }
    return r;
}

// ============================================================
// Экран: Поиск заказов
// ============================================================
void screenSearch(OrderManager& om, const std::string& masterName) {
    static const std::vector<std::string> searchModes = {
        " По мастеру (выбор из списка) ",
        " По ID заказа                 ",
        " По совпадению текста         "
    };
    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Поиск заказов ===", Color::LOGO);
        UIManager::printHLine(2, 40, '-', Color::DIM);
        UIManager::printCentered(3, "Выберите тип поиска:", Color::MENU);
        UIManager::printCentered(8, "  [ESC] Назад  ", Color::DEFAULT);

        int ch = UIManager::selectMenu(searchModes, 4);
        if (ch == -1) return;

        std::vector<Order> results;

        // ---- По мастеру ----
        if (ch == 0) {
            std::vector<std::string> masters;
            for (const auto& o : om.getAllOrders()) {
                bool found = false;
                for (const auto& m : masters) if (m == o.master) { found = true; break; }
                if (!found && !o.master.empty()) masters.push_back(o.master);
            }
            if (masters.empty()) {
                UIManager::clearScreen();
                UIManager::printCentered(5, "Нет заказов с мастерами.", Color::DIM);
                UIManager::waitKey(7);
                continue;
            }
            int sel = 0;
            bool picked = false;
            while (true) {
                UIManager::clearScreen();
                UIManager::hideCursor();
                UIManager::printCentered(1, "=== Выбор мастера ===", Color::LOGO);
                UIManager::printCentered(3, "Стрелки — выбор, Enter — поиск, ESC — назад", Color::DIM);
                for (int i = 0; i < (int)masters.size(); ++i) {
                    WORD col = (i == sel) ? Color::HIGHLIGHT : Color::DEFAULT;
                    UIManager::printCentered(5 + i, (i == sel ? " > " : "   ") + masters[i], col);
                }
                int k = _getch();
                if (k == KEY_ESC) break;
                if (k == 13) { picked = true; break; }
                if (k == 0 || k == 224) {
                    int k2 = _getch();
                    if (k2 == 72 && sel > 0) --sel;
                    if (k2 == 80 && sel < (int)masters.size()-1) ++sel;
                }
            }
            if (!picked) continue;
            for (const auto& o : om.getAllOrders())
                if (o.master == masters[sel]) results.push_back(o);
        }

        // ---- По ID ----
        else if (ch == 1) {
            UIManager::clearScreen();
            UIManager::hideCursor();
            UIManager::printCentered(1, "=== Поиск по ID ===", Color::LOGO);
            UIManager::printHLine(2, 30, '-', Color::DIM);
            std::string idStr;
            if (!UIManager::inputStringESC(4, "  Введите ID: ", idStr)) continue;
            int id = 0;
            try { id = std::stoi(idStr); } catch (...) {}
            Order* o = om.findById(id);
            if (o) results.push_back(*o);
        }

        // ---- По тексту ----
        else {
            UIManager::clearScreen();
            UIManager::hideCursor();
            UIManager::printCentered(1, "=== Поиск по тексту ===", Color::LOGO);
            UIManager::printCentered(3, "Ищет по ФИО, телефону, описанию, мастеру", Color::DIM);
            std::string query;
            if (!UIManager::inputStringESC(5, "  Запрос: ", query)) continue;
            std::string qLow = toLowerUTF8(query);
            for (const auto& o : om.getAllOrders()) {
                auto has = [&](const std::string& f) {
                    return toLowerUTF8(f).find(qLow) != std::string::npos;
                };
                if (has(o.clientName) || has(o.phone) || has(o.description) || has(o.master))
                    results.push_back(o);
            }
        }

        // ---- Результаты со стрелочной навигацией ----
        if (results.empty()) {
            UIManager::clearScreen();
            UIManager::printCentered(5, "Ничего не найдено.", Color::DIM);
            UIManager::waitKey(7);
            continue;
        }

        int sel = 0;
        while (true) {
            UIManager::clearScreen();
            UIManager::hideCursor();
            UIManager::printCentered(1, "=== Результаты: " + std::to_string(results.size()) + " ===", Color::LOGO);
            UIManager::printCentered(2, "Стрелки — навигация  Enter — открыть  ESC — назад", Color::DIM);
            UIManager::printHLine(3, 80, '-', Color::DIM);

            int res = UIManager::selectFromTable(results, 4, sel);
            if (res == -1) break;
            if (res >= 0) {
                screenManageOrder(om, results[res].id, masterName);
                // после возврата — обновляем список и перерисовываем с нуля
                std::vector<Order> upd;
                for (const auto& r : results) { Order* op = om.findById(r.id); if (op) upd.push_back(*op); }
                results = upd;
                if (results.empty()) break;
                if (sel >= (int)results.size()) sel = (int)results.size() - 1;
                // продолжаем цикл — clearScreen в начале перерисует всё
            }
        }
    }
}

void screenMainMenu(const std::string& login, const std::string& masterName, OrderManager& om) {
    static const std::vector<std::string> items = {
        " Добавить заказ        ",
        " Управление заказами   ",
        " Статистика            ",
        " Поиск                 "
    };
    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        COORD sz = UIManager::getConsoleSize();
        int logoH = 11;
        int startRow = (sz.Y - logoH - 12) / 2;
        if (startRow < 1) startRow = 1;
        UIManager::drawLogo(startRow);
        int menuRow = startRow + logoH + 2;
        UIManager::printCentered(menuRow++, std::string(38, '='), Color::LOGO);
        UIManager::printCentered(menuRow++, "  Добро пожаловать, " + masterName + "!  ", Color::DEFAULT);
        UIManager::printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        int itemsRow = menuRow;
        UIManager::printCentered(menuRow + (int)items.size() + 1, std::string(38, '-'), Color::LOGO);
        UIManager::printCentered(menuRow + (int)items.size() + 2, "  [ESC] Выйти из аккаунта       ", Color::DEFAULT);

        int ch = UIManager::selectMenu(items, itemsRow);
        if      (ch == -1) return;
        else if (ch == 0)  screenAddOrder(om, masterName);
        else if (ch == 1)  screenOrders(om, masterName);
        else if (ch == 2)  screenStats(om);
        else if (ch == 3)  screenSearch(om, masterName);
    }
}

// ============================================================
// Экран: Гостевое меню (только просмотр)
// ============================================================
void screenGuestMenu(OrderManager& om) {
    static const std::vector<std::string> items = {
        " Просмотр заказов  ",
        " Статистика        ",
        " Поиск             "
    };
    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        COORD sz = UIManager::getConsoleSize();
        int logoH = 11;
        int startRow = (sz.Y - logoH - 12) / 2;
        if (startRow < 1) startRow = 1;
        UIManager::drawLogo(startRow);
        int menuRow = startRow + logoH + 2;
        UIManager::printCentered(menuRow++, std::string(38, '='), Color::LOGO);
        UIManager::printCentered(menuRow++, "  Гостевой режим (только просмотр)  ", Color::DIM);
        UIManager::printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        int itemsRow = menuRow;
        UIManager::printCentered(menuRow + (int)items.size() + 1, std::string(38, '-'), Color::LOGO);
        UIManager::printCentered(menuRow + (int)items.size() + 2, "  [ESC] Назад  ", Color::DEFAULT);

        int ch = UIManager::selectMenu(items, itemsRow);
        if      (ch == -1) return;
        else if (ch == 0)  screenOrders(om, "");
        else if (ch == 1)  screenStats(om);
        else if (ch == 2)  screenSearch(om, "");
    }
}

// ============================================================
// Стартовый экран
// ============================================================
void screenStart(UserManager& um, OrderManager& om) {
    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        COORD sz = UIManager::getConsoleSize();
        int startRow = (sz.Y - 11 - 7) / 2;
        if (startRow < 1) startRow = 1;
        UIManager::drawLogo(startRow);

        int menuRow = startRow + 12;
        UIManager::printCentered(menuRow++, std::string(26, '='), Color::LOGO);
        UIManager::printCentered(menuRow,   "  Выберите действие:    ", Color::MENU); menuRow++;
        UIManager::printCentered(menuRow++, std::string(26, '-'), Color::LOGO);

        int itemsRow = menuRow;
        static const std::vector<std::string> items = {
            " Вход в систему  ",
            " Регистрация     ",
            " Войти как гость "
        };
        int ch = UIManager::selectMenu(items, itemsRow);

        if (ch == -1) {
            UIManager::clearScreen();
            UIManager::printCentered(sz.Y / 2, "До свидания! Спасибо за использование J.A.M.", Color::LOGO);
            UIManager::setColor(Color::DEFAULT);
            Sleep(1500);
            return;
        } else if (ch == 0) {
            std::string login = screenLogin(um);
            if (!login.empty()) screenMainMenu(login, um.getFullName(login), om);
        } else if (ch == 1) {
            screenRegister(um);
        } else if (ch == 2) {
            screenGuestMenu(om);
        }
    }
}

// ============================================================
// main()
// ============================================================
int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    SetConsoleTitle("J.A.M. - Jewelry Atelier Manager");
    UIManager::hideCursor();

    UserManager  userManager;
    OrderManager orderManager;

    screenStart(userManager, orderManager);

    UIManager::setColor(Color::DEFAULT);
    UIManager::showCursor();
    return 0;
}
