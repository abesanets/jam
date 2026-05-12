// ============================================================
// main.cpp — Точка входа приложения J.A.M.
// Jewelry Atelier Manager — АРМ ювелирной мастерской
//
// Компиляция (MinGW/GCC):
//   g++ -o jam.exe jam/main.cpp -std=c++14 -finput-charset=UTF-8
// ============================================================

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#include "models.h"
#include "ui.h"
#include "storage.h"

// forward declaration
void screenManageOrder(OrderManager& om, int id, const std::string& masterName);

// ============================================================
// Экран: Добавление нового заказа
// ============================================================
void screenAddOrder(OrderManager& om, const std::string& masterName) {
    Ui::clearScreen();
    Ui::hideCursor();
    // Блок: заголовок(4) + 5 полей по 2 строки(10) + сообщение(2) + подсказка(1) = ~17 строк
    int r = Ui::vCenter(17);
    Ui::printCentered(r,   "=== Добавление нового заказа ===", Color::LOGO);
    Ui::printCentered(r+1, "Введите данные заказа:", Color::MENU);
    Ui::printHLine(r+2, 44, '-', Color::DIM);
    Ui::printCentered(r+3, "[ESC] на любом поле — отмена", Color::DIM);

    std::string clientName, phone, description, priceStr, deadline;
    if (!Ui::inputStringESC(r+5,  "  ФИО клиента:           ", clientName))  return;
    if (!Ui::inputStringESC(r+7,  "  Телефон:               ", phone))        return;
    if (!Ui::inputStringESC(r+9,  "  Описание изделия:      ", description))  return;
    if (!Ui::inputStringESC(r+11, "  Цена работы (руб):     ", priceStr))     return;
    if (!Ui::inputStringESC(r+13, "  Дедлайн (дата):      ", deadline))     return;

    if (clientName.empty() || phone.empty() || description.empty()) {
        Ui::printError(r+15, "Все поля обязательны для заполнения!");
        Ui::waitKey(r+17);
        return;
    }
    if (priceStr.empty()) {
        Ui::printError(r+15, "Укажите цену (число, например 1500 или 1500.50)!");
        Ui::waitKey(r+17);
        return;
    }
    double price = 0.0;
    try {
        price = std::stod(priceStr);
    } catch (...) {
        Ui::printError(r+15, "Неверный формат цены!");
        Ui::waitKey(r+17);
        return;
    }
    std::string deadlineNorm = deadline;
    if (!deadline.empty()) {
        if (!parseNormalizeDeadline(deadline, deadlineNorm)) {
            Ui::printError(r+15, "Неверная дата. Примеры: 12.05.2026, 12/05/2026, 2026-05-12");
            Ui::waitKey(r+17);
            return;
        }
    }

    int newId = om.addOrder(clientName, phone, description, price, deadlineNorm, masterName);
    Ui::printSuccess(r+15, "Заказ добавлен! ID: " + std::to_string(newId));
    Ui::waitKey(r+17);
}

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
        Ui::clearScreen();
        Ui::hideCursor();
        Ui::printCentered(1, "=== Управление заказами ===", Color::LOGO);

        std::string fi = myOnly ? "Мои  |  " : "Все  |  ";
        fi += "Статус: ";
        fi += statusFilter.empty() ? "все активные" : statusFilter;
        fi += "  |  Дедлайн: ";
        fi += (sortMode == 0) ? "—" : (sortMode == 1 ? "ближе сначала" : "дальше сначала");
        fi += showIssued ? "  |  [выданные]" : "";
        Ui::printCentered(2, fi, Color::DIM);
        Ui::printCentered(3, "[M]ои [N]овый [W]работе [G]отов [V]ыданные [D]едлайн [R]сброс [ESC]назад", Color::MENU);
        Ui::printHLine(4, 80, '-', Color::DIM);
        auto applyMyFilter = [&](std::vector<Order>& orders) {
            if (myOnly)
                orders.erase(std::remove_if(orders.begin(), orders.end(),
                    [&](const Order& o){ return o.master != masterName; }), orders.end());
        };

        std::vector<Order> orders;
        if (showIssued) {
            orders = om.getFiltered("Выдан", 0, 0);
            applyMyFilter(orders);
            Ui::printCentered(5, "Выданных: " + std::to_string(orders.size()), Color::DIM);
        } else {
            orders = om.getFiltered(statusFilter, 0, sortMode == 0 ? 0 : 1);
            if (statusFilter.empty())
                orders.erase(std::remove_if(orders.begin(), orders.end(),
                    [](const Order& o){ return o.status == "Выдан"; }), orders.end());
            applyMyFilter(orders);
            if (sortMode == 2) std::reverse(orders.begin(), orders.end());
            Ui::printCentered(5, "Показано: " + std::to_string(orders.size()), Color::MENU);
        }

        if (sel >= (int)orders.size()) sel = 0;

        if (orders.empty()) {
            Ui::drawOrdersTable(orders, 6);
            Ui::KeyEvent ke = Ui::readKey();
            int k = Ui::normalizeKey(ke.vk);
            if (ke.vk == VK_ESCAPE) return;
            if (k == 'm') myOnly = !myOnly;
            else if (k == 'v') { showIssued = !showIssued; statusFilter = ""; sortMode = 0; }
            else if (k == 'r') { statusFilter = ""; sortMode = 0; showIssued = false; myOnly = false; }
            continue;
        }
        int res = Ui::selectFromTable(orders, 6, sel, "mnvdrgw");
        if (res == -1) return;
        if (res == -3) continue; // ресайз
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

        Ui::clearScreen();
        Ui::hideCursor();
        // drawOrderDetail занимает ~16 строк, меню ~6 строк
        int r = Ui::vCenter(22);
        Ui::drawOrderDetail(*o, r);

        bool isOwner = (o->master == masterName);
        int menuRow = r + 15;
        Ui::printHLine(menuRow++, 46, '-', Color::DIM);

        if (isOwner) {
            std::vector<std::string> actions = {
                " Изменить статус ",
                " Редактировать   ",
                " Удалить заказ   "
            };
            int act = Ui::selectMenu(actions, menuRow);
            if (act == -2) continue; // ресайз
            if (act == -1) return;
            if (act == 0) {
            Ui::clearScreen();
            int sr = Ui::vCenter(10 + ORDER_STATUS_COUNT);
            Ui::printCentered(sr,   "=== Изменение статуса ===", Color::LOGO);
            Ui::printCentered(sr+1, "Заказ #" + std::to_string(o->id) + " — " + o->clientName, Color::DEFAULT);
            Ui::printCentered(sr+2, "Текущий статус: " + o->status, Color::HIGHLIGHT);
            Ui::printHLine(sr+3, 36, '-', Color::DIM);
            {
                std::vector<std::string> statuses;
                for (int i = 0; i < ORDER_STATUS_COUNT; i++) statuses.push_back(" " + ORDER_STATUSES[i] + " ");
                int sch = Ui::selectMenu(statuses, sr+4);
                if (sch >= 0) {
                    om.updateStatus(id, ORDER_STATUSES[sch]);
                    Ui::printSuccess(sr+5+ORDER_STATUS_COUNT, "Статус изменён на: " + ORDER_STATUSES[sch]);
                    Ui::waitKey(sr+7+ORDER_STATUS_COUNT);
                }
            }
        } else if (act == 1) {
            Ui::clearScreen();
            Ui::hideCursor();
            int er = Ui::vCenter(17);
            Ui::printCentered(er,   "=== Редактирование заказа #" + std::to_string(o->id) + " ===", Color::LOGO);
            Ui::printCentered(er+1, "Оставьте поле пустым — значение не изменится", Color::DIM);
            Ui::printHLine(er+2, 50, '-', Color::DIM);

            std::string clientName, phone, description, priceStr, deadline;
            Ui::inputStringESC(er+4,  "  ФИО клиента [" + o->clientName + "]: ", clientName);
            Ui::inputStringESC(er+6,  "  Телефон [" + o->phone + "]: ", phone);
            Ui::inputStringESC(er+8,  "  Описание [" + o->description + "]: ", description);
            std::ostringstream ps; ps << std::fixed << std::setprecision(2) << o->price;
            Ui::inputStringESC(er+10, "  Цена [" + ps.str() + "]: ", priceStr);
            Ui::inputStringESC(er+12, "  Дедлайн [" + o->deadline + "]: ", deadline);

            double newPrice = o->price;
            if (!priceStr.empty()) {
                try {
                    newPrice = std::stod(priceStr);
                } catch (...) {
                    Ui::printError(er+14, "Неверный формат цены — заказ не изменён.");
                    Ui::waitKey(er+16);
                    continue;
                }
            }
            std::string newDeadline = o->deadline;
            if (!deadline.empty()) {
                if (!parseNormalizeDeadline(deadline, newDeadline)) {
                    Ui::printError(er+14, "Неверная дата. Примеры: 12.05.2026, 12/05/2026, 2026-05-12");
                    Ui::waitKey(er+16);
                    continue;
                }
            }

            om.updateOrder(id,
                clientName.empty()  ? o->clientName  : clientName,
                phone.empty()       ? o->phone       : phone,
                description.empty() ? o->description : description,
                newPrice,
                deadline.empty()    ? o->deadline    : newDeadline);
            Ui::printSuccess(er+14, "Заказ обновлён!");
            Ui::waitKey(er+16);
        } else if (act == 2) {
            Ui::clearScreen();
            int dr = Ui::vCenter(8);
            Ui::printCentered(dr+2, "Удалить заказ #" + std::to_string(o->id) + " — " + o->clientName + "?", Color::OVERDUE);
            std::vector<std::string> confirm = { " Да, удалить ", " Отмена      " };
            int conf = Ui::selectMenu(confirm, dr+4);
            if (conf == 0) {
                om.deleteOrder(id);
                Ui::printSuccess(dr+7, "Заказ удалён.");
                Ui::waitKey(dr+9);
                return;
            }
        }
        } else {
            Ui::printCentered(menuRow,   "  Только просмотр  ", Color::DIM);
            Ui::printCentered(menuRow+1, "  (чужой заказ)    ", Color::DIM);
            Ui::printCentered(menuRow+3, "  [ESC] Назад      ", Color::DEFAULT);
            Ui::readKey();
            return;
        }
    }
}

// ============================================================
// Экран: Авторизация
// ============================================================
std::string screenLogin(UserManager& um) {
    Ui::clearScreen();
    Ui::hideCursor();
    // Блок: заголовок(3) + 2 поля(4) + сообщение(2) = ~9 строк
    int r = Ui::vCenter(9);
    Ui::printCentered(r,   "=== Вход в систему ===", Color::LOGO);
    Ui::printHLine(r+1, 30, '-', Color::DIM);
    Ui::printCentered(r+2, "[ESC] на любом поле — назад", Color::DIM);
    std::string login, password;
    if (!Ui::inputStringESC(r+4,  "  Логин:    ", login))    return "";
    if (!Ui::inputPasswordESC(r+6, "  Пароль:   ", password)) return "";
    if (um.loginUser(login, password)) {
        Ui::printSuccess(r+8, "Авторизация успешна!");
        Ui::waitKey(r+10);
        return login;
    }
    Ui::printError(r+8, "Неверный логин или пароль!");
    Ui::waitKey(r+10);
    return "";
}

// ============================================================
// Экран: Статистика
// ============================================================
void screenStats(OrderManager& om) {
    Ui::clearScreen();
    Ui::hideCursor();
    // Блок: заголовок(2) + 12 строк статистики + разделитель(2) = ~16 строк
    int r = Ui::vCenter(16);
    Ui::printCentered(r,   "=== Статистика ===", Color::LOGO);
    Ui::printHLine(r+1, 44, '-', Color::DIM);

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

    auto printStat = [](int row, const std::string& label, const std::string& val, WORD col = Color::DEFAULT, WORD labelCol = Color::MENU) {
        COORD sz = Ui::getConsoleSize();
        int x = (sz.X - 44) / 2; if (x < 0) x = 0;
        Ui::setCursor(x, row); Ui::setColor(labelCol);
        std::cout << label;
        Ui::setCursor(x + 30, row); Ui::setColor(col);
        std::cout << val;
        Ui::setColor(Color::DEFAULT);
    };

    int row = r + 3;
    printStat(row++, "Всего заказов:                ", std::to_string(total),     Color::LOGO,    Color::LOGO);
    row++;
    printStat(row++, "  Новых:                      ", std::to_string(cntNew),    Color::MENU,    Color::MENU);
    printStat(row++, "  В работе:                   ", std::to_string(cntWork),   Color::HIGHLIGHT, Color::MENU);
    printStat(row++, "  Готовых:                    ", std::to_string(cntReady),  Color::MENU,    Color::MENU);
    printStat(row++, "  Выданных:                   ", std::to_string(cntIssued), Color::DIM,     Color::MENU);
    row++;
    printStat(row++, "  Просроченных:               ", std::to_string(cntOverdue), cntOverdue > 0 ? Color::OVERDUE : Color::DIM, Color::MENU);
    row++;

    std::ostringstream s1, s2, s3;
    s1 << std::fixed << std::setprecision(2) << sumTotal;
    s2 << std::fixed << std::setprecision(2) << sumActive;
    s3 << std::fixed << std::setprecision(2) << sumIssued;
    printStat(row++, "Сумма всех заказов (руб):     ", s1.str(),  Color::LOGO,  Color::LOGO);
    printStat(row++, "  Активных (руб):             ", s2.str(),  Color::MENU,  Color::MENU);
    printStat(row++, "  Выданных (руб):             ", s3.str(),  Color::DIM,   Color::MENU);

    Ui::printHLine(row + 1, 44, '-', Color::DIM);
    Ui::waitKey(row + 3);
}

// ============================================================
// Экран: Регистрация
// ============================================================
void screenRegister(UserManager& um) {
    Ui::clearScreen();
    Ui::hideCursor();
    // Блок: заголовок(3) + 4 поля(8) + сообщение(2) = ~13 строк
    int r = Ui::vCenter(13);
    Ui::printCentered(r,   "=== Регистрация ===", Color::LOGO);
    Ui::printHLine(r+1, 30, '-', Color::DIM);
    Ui::printCentered(r+2, "[ESC] на любом поле — назад", Color::DIM);
    std::string fullName, login, password, confirm;
    if (!Ui::inputStringESC(r+4, "  Ваше ФИО:          ", fullName))  return;
    if (!Ui::inputStringESC(r+6, "  Придумайте логин:  ", login))     return;
    if (!Ui::inputPasswordESC(r+8, "  Придумайте пароль: ", password))  return;
    if (!Ui::inputPasswordESC(r+10,"  Повторите пароль:  ", confirm))   return;
    if (password != confirm) {
        Ui::printError(r+12, "Пароли не совпадают!");
        Ui::waitKey(r+14);
        return;
    }
    if (um.registerUser(login, password, fullName))
        Ui::printSuccess(r+12, "Регистрация прошла успешно! Войдите в систему.");
    else
        Ui::printError(r+12, "Логин уже занят или поля пустые!");
    Ui::waitKey(r+14);
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
        Ui::clearScreen();
        Ui::hideCursor();
        int r = Ui::vCenter(10);
        Ui::printCentered(r,   "=== Поиск заказов ===", Color::LOGO);
        Ui::printHLine(r+1, 40, '-', Color::DIM);
        Ui::printCentered(r+2, "Выберите тип поиска:", Color::MENU);
        Ui::printCentered(r+7, "  [ESC] Назад  ", Color::DEFAULT);

        int ch = Ui::selectMenu(searchModes, r+3);
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
                Ui::clearScreen();
                int mr = Ui::vCenter(4);
                Ui::printCentered(mr+2, "Нет заказов с мастерами.", Color::DIM);
                Ui::waitKey(mr+4);
                continue;
            }
            int sel = 0;
            bool picked = false;
            while (true) {
                Ui::clearScreen();
                Ui::hideCursor();
                int mr = Ui::vCenter(5 + (int)masters.size());
                Ui::printCentered(mr,   "=== Выбор мастера ===", Color::LOGO);
                Ui::printCentered(mr+2, "Стрелки — выбор, Enter — поиск, ESC — назад", Color::DIM);
                for (int i = 0; i < (int)masters.size(); ++i) {
                    WORD col = (i == sel) ? Color::HIGHLIGHT : Color::DEFAULT;
                    Ui::printCentered(mr+4 + i, (i == sel ? " > " : "   ") + masters[i], col);
                }
                Ui::KeyEvent ke = Ui::readKey();
                if (ke.vk == VK_ESCAPE) break;
                if (ke.vk == VK_RETURN) { picked = true; break; }
                if (ke.vk == VK_UP   && sel > 0) --sel;
                if (ke.vk == VK_DOWN && sel < (int)masters.size()-1) ++sel;
            }
            if (!picked) continue;
            for (const auto& o : om.getAllOrders())
                if (o.master == masters[sel]) results.push_back(o);
        }

        // ---- По ID ----
        else if (ch == 1) {
            Ui::clearScreen();
            Ui::hideCursor();
            int ir = Ui::vCenter(5);
            Ui::printCentered(ir,   "=== Поиск по ID ===", Color::LOGO);
            Ui::printHLine(ir+1, 30, '-', Color::DIM);
            std::string idStr;
            if (!Ui::inputStringESC(ir+3, "  Введите ID: ", idStr)) continue;
            int id = 0;
            try { id = std::stoi(idStr); } catch (...) {}
            Order* o = om.findById(id);
            if (o) results.push_back(*o);
        }

        // ---- По тексту ----
        else {
            Ui::clearScreen();
            Ui::hideCursor();
            int tr = Ui::vCenter(6);
            Ui::printCentered(tr,   "=== Поиск по тексту ===", Color::LOGO);
            Ui::printCentered(tr+2, "Ищет по ФИО, телефону, описанию, мастеру", Color::DIM);
            std::string query;
            if (!Ui::inputStringESC(tr+4, "  Запрос: ", query)) continue;
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
            Ui::clearScreen();
            int er = Ui::vCenter(4);
            Ui::printCentered(er+2, "Ничего не найдено.", Color::DIM);
            Ui::waitKey(er+4);
            continue;
        }

        int sel = 0;
        while (true) {
            Ui::clearScreen();
            Ui::hideCursor();
            Ui::printCentered(1, "=== Результаты: " + std::to_string(results.size()) + " ===", Color::LOGO);
            Ui::printCentered(2, "Стрелки — навигация  Enter — открыть  ESC — назад", Color::DIM);
            Ui::printHLine(3, 80, '-', Color::DIM);

            int res = Ui::selectFromTable(results, 4, sel);
            if (res == -1) break;
            if (res == -3) continue; // ресайз
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
        Ui::clearScreen();
        Ui::hideCursor();
        COORD sz = Ui::getConsoleSize();
        int logoH = 11;
        int startRow = (sz.Y - logoH - 12) / 2;
        if (startRow < 1) startRow = 1;
        Ui::drawLogo(startRow);
        int menuRow = startRow + logoH + 2;
        Ui::printCentered(menuRow++, std::string(38, '='), Color::LOGO);
        Ui::printCentered(menuRow++, "  Добро пожаловать, " + masterName + "!  ", Color::DEFAULT);
        Ui::printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        int itemsRow = menuRow;
        Ui::printCentered(menuRow + (int)items.size() + 1, std::string(38, '-'), Color::LOGO);
        Ui::printCentered(menuRow + (int)items.size() + 2, "  [ESC] Выйти из аккаунта       ", Color::DEFAULT);

        int ch = Ui::selectMenu(items, itemsRow);
        if      (ch == -2) continue; // ресайз — перерисовать
        else if (ch == -1) return;
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
        Ui::clearScreen();
        Ui::hideCursor();
        COORD sz = Ui::getConsoleSize();
        int logoH = 11;
        int startRow = (sz.Y - logoH - 12) / 2;
        if (startRow < 1) startRow = 1;
        Ui::drawLogo(startRow);
        int menuRow = startRow + logoH + 2;
        Ui::printCentered(menuRow++, std::string(38, '='), Color::LOGO);
        Ui::printCentered(menuRow++, "  Гостевой режим (только просмотр)  ", Color::DIM);
        Ui::printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        int itemsRow = menuRow;
        Ui::printCentered(menuRow + (int)items.size() + 1, std::string(38, '-'), Color::LOGO);
        Ui::printCentered(menuRow + (int)items.size() + 2, "  [ESC] Назад  ", Color::DEFAULT);

        int ch = Ui::selectMenu(items, itemsRow);
        if      (ch == -2) continue; // ресайз
        else if (ch == -1) return;
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
        Ui::clearScreen();
        Ui::hideCursor();
        COORD sz = Ui::getConsoleSize();
        int startRow = (sz.Y - 11 - 7) / 2;
        if (startRow < 1) startRow = 1;
        Ui::drawLogo(startRow);

        int menuRow = startRow + 12;
        Ui::printCentered(menuRow++, std::string(26, '='), Color::LOGO);
        Ui::printCentered(menuRow,   "  Выберите действие:    ", Color::MENU); menuRow++;
        Ui::printCentered(menuRow++, std::string(26, '-'), Color::LOGO);

        int itemsRow = menuRow;
        static const std::vector<std::string> items = {
            " Вход в систему  ",
            " Регистрация     ",
            " Войти как гость "
        };
        int ch = Ui::selectMenu(items, itemsRow);

        if (ch == -2) continue; // ресайз — перерисовать
        if (ch == -1) {
            Ui::clearScreen();
            Ui::printCentered(sz.Y / 2, "До свидания! Спасибо за использование J.A.M.", Color::LOGO);
            Ui::setColor(Color::DEFAULT);
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
    Ui::hideCursor();

    UserManager  userManager;
    OrderManager orderManager;

    screenStart(userManager, orderManager);

    Ui::setColor(Color::DEFAULT);
    Ui::showCursor();
    return 0;
}
