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

// Читает один логический символ с клавиатуры и нормализует его.
// Консоль в UTF-8: русские буквы приходят двумя байтами (0xD0/0xD1 + второй байт).
// Возвращает латинскую строчную букву для русских клавиш, иначе сам символ.
static int normalizeKey(int first) {
    unsigned char b1 = (unsigned char)first;

    // UTF-8 двухбайтовая последовательность — читаем второй байт
    if (b1 == 0xD0 || b1 == 0xD1) {
        unsigned char b2 = (unsigned char)_getch();
        // Маппинг UTF-8 кириллица -> физическая QWERTY-клавиша (латиница строчная)
        // Формат: {b1, b2, lat}
        static const struct { unsigned char c1, c2; char lat; } map[] = {
            {0xD0,0xB9,'q'}, // й
            {0xD1,0x86,'w'}, // ц
            {0xD1,0x83,'e'}, // у
            {0xD0,0xBA,'r'}, // к
            {0xD0,0xB5,'t'}, // е
            {0xD0,0xBD,'y'}, // н
            {0xD0,0xB3,'u'}, // г
            {0xD1,0x88,'i'}, // ш
            {0xD1,0x89,'o'}, // щ
            {0xD0,0xB7,'p'}, // з
            {0xD1,0x84,'a'}, // ф
            {0xD1,0x8B,'s'}, // ы
            {0xD0,0xB2,'d'}, // в
            {0xD0,0xB0,'f'}, // а
            {0xD0,0xBF,'g'}, // п
            {0xD1,0x80,'h'}, // р
            {0xD0,0xBE,'j'}, // о
            {0xD0,0xBB,'k'}, // л
            {0xD0,0xB4,'l'}, // д
            {0xD1,0x8F,'z'}, // я
            {0xD1,0x87,'x'}, // ч
            {0xD1,0x81,'c'}, // с
            {0xD0,0xBC,'v'}, // м
            {0xD0,0xB8,'b'}, // и
            {0xD1,0x82,'n'}, // т
            {0xD1,0x8C,'m'}, // ь
            // Заглавные
            {0xD0,0x99,'q'}, // Й
            {0xD0,0xA6,'w'}, // Ц
            {0xD0,0xA3,'e'}, // У
            {0xD0,0x9A,'r'}, // К
            {0xD0,0x95,'t'}, // Е
            {0xD0,0x9D,'y'}, // Н
            {0xD0,0x93,'u'}, // Г
            {0xD0,0xA8,'i'}, // Ш
            {0xD0,0xA9,'o'}, // Щ
            {0xD0,0x97,'p'}, // З
            {0xD0,0xA4,'a'}, // Ф
            {0xD0,0xAB,'s'}, // Ы
            {0xD0,0x92,'d'}, // В
            {0xD0,0x90,'f'}, // А
            {0xD0,0x9F,'g'}, // П
            {0xD0,0xA0,'h'}, // Р
            {0xD0,0x9E,'j'}, // О
            {0xD0,0x9B,'k'}, // Л
            {0xD0,0x94,'l'}, // Д
            {0xD0,0xAF,'z'}, // Я
            {0xD0,0xA7,'x'}, // Ч
            {0xD0,0xA1,'c'}, // С
            {0xD0,0x9C,'v'}, // М
            {0xD0,0x98,'b'}, // И
            {0xD0,0xA2,'n'}, // Т
            {0xD0,0xAC,'m'}, // Ь
            {0,0,0}
        };
        for (int i = 0; map[i].c1; ++i)
            if (map[i].c1 == b1 && map[i].c2 == b2) return map[i].lat;
        return -1; // неизвестная последовательность
    }

    // Обычная латиница — привести к нижнему регистру
    if (first >= 'A' && first <= 'Z') return first + 32;
    return first;
}

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
void screenViewOrders(OrderManager& om, const std::string& masterName) {
    std::string statusFilter = "";
    int sortMode = 0;
    bool showIssued = false;
    bool myOnly = false;

    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Просмотр заказов ===", Color::LOGO);

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

        if (showIssued) {
            std::vector<Order> issued = om.getFiltered("Выдан", 0, 0);
            applyMyFilter(issued);
            UIManager::printCentered(5, "Выданных: " + std::to_string(issued.size()), Color::DIM);
            UIManager::drawOrdersTable(issued, 6);
        } else {
            std::vector<Order> orders = om.getFiltered(statusFilter, 0, sortMode == 0 ? 0 : 1);
            if (statusFilter.empty())
                orders.erase(std::remove_if(orders.begin(), orders.end(),
                    [](const Order& o){ return o.status == "Выдан"; }), orders.end());
            applyMyFilter(orders);
            if (sortMode == 2) std::reverse(orders.begin(), orders.end());
            UIManager::printCentered(5, "Показано: " + std::to_string(orders.size()), Color::MENU);
            UIManager::drawOrdersTable(orders, 6);
        }

        int ch = normalizeKey(_getch());
        if (ch == KEY_ESC) return;
        if      (ch == 'm') { myOnly = !myOnly; }
        else if (ch == 'd') { sortMode = (sortMode + 1) % 3; }
        else if (ch == 'v') { showIssued = !showIssued; statusFilter = ""; sortMode = 0; }
        else if (ch == 'r') { statusFilter = ""; sortMode = 0; showIssued = false; myOnly = false; }
        else if (!showIssued) {
            if      (ch == 'n') statusFilter = (statusFilter == "Новый")    ? "" : "Новый";
            else if (ch == 'w') statusFilter = (statusFilter == "В работе") ? "" : "В работе";
            else if (ch == 'g') statusFilter = (statusFilter == "Готов")    ? "" : "Готов";
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
            UIManager::printCentered(menuRow++, "  [1] Изменить статус  ", Color::MENU);
            UIManager::printCentered(menuRow++, "  [2] Редактировать    ", Color::MENU);
            UIManager::printCentered(menuRow++, "  [3] Удалить заказ    ", Color::OVERDUE);
        } else {
            UIManager::printCentered(menuRow++, "  Только просмотр      ", Color::DIM);
            UIManager::printCentered(menuRow++, "  (чужой заказ)        ", Color::DIM);
            menuRow++;
        }
        UIManager::printCentered(menuRow++, "  [ESC] Назад          ", Color::DEFAULT);

        int ch = normalizeKey(_getch());
        if (ch == KEY_ESC) return;

        if (isOwner && ch == '1') {
            UIManager::clearScreen();
            UIManager::printCentered(1, "=== Изменение статуса ===", Color::LOGO);
            UIManager::printCentered(2, "Заказ #" + std::to_string(o->id) + " — " + o->clientName, Color::DEFAULT);
            UIManager::printCentered(3, "Текущий статус: " + o->status, Color::HIGHLIGHT);
            UIManager::printHLine(4, 36, '-', Color::DIM);
            for (int i = 0; i < ORDER_STATUS_COUNT; i++)
                UIManager::printCentered(5 + i, "  [" + std::to_string(i+1) + "] " + ORDER_STATUSES[i], Color::MENU);
            UIManager::printCentered(5 + ORDER_STATUS_COUNT, "  [ESC] Отмена", Color::DIM);
            while (true) {
                int sch = _getch();
                if (sch == KEY_ESC) break;
                int idx = sch - '1';
                if (idx >= 0 && idx < ORDER_STATUS_COUNT) {
                    om.updateStatus(id, ORDER_STATUSES[idx]);
                    UIManager::printSuccess(6 + ORDER_STATUS_COUNT, "Статус изменён на: " + ORDER_STATUSES[idx]);
                    UIManager::waitKey(8 + ORDER_STATUS_COUNT);
                    break;
                }
            }
        } else if (isOwner && ch == '2') {
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
        } else if (isOwner && ch == '3') {
            UIManager::clearScreen();
            UIManager::printCentered(3, "Удалить заказ #" + std::to_string(o->id) + " — " + o->clientName + "?", Color::OVERDUE);
            UIManager::printCentered(5, "  [Y] Да, удалить   [N / ESC] Отмена  ", Color::MENU);
            int conf = normalizeKey(_getch());
            if (conf == 'y') {
                om.deleteOrder(id);
                UIManager::printSuccess(7, "Заказ удалён.");
                UIManager::waitKey(9);
                return;
            }
        }
    }
}

// ============================================================
// Экран: Менеджмент заказов — список + ввод ID
// ============================================================
void screenManageOrders(OrderManager& om, const std::string& masterName) {
    bool showIssued = false;
    bool myOnly = false;

    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Менеджмент заказов ===", Color::LOGO);

        std::string modeStr = myOnly ? "[M]все  " : "[M]ои  ";
        modeStr += showIssued ? "[V]активные" : "[V]выданные";
        modeStr += "  [ESC]назад";
        UIManager::printCentered(2, modeStr, Color::MENU);
        UIManager::printHLine(3, 80, '-', Color::DIM);

        std::vector<Order> orders;
        if (showIssued) {
            orders = om.getFiltered("Выдан", 0, 0);
        } else {
            orders = om.getAllOrders();
            orders.erase(std::remove_if(orders.begin(), orders.end(),
                [](const Order& o){ return o.status == "Выдан"; }), orders.end());
        }
        if (myOnly)
            orders.erase(std::remove_if(orders.begin(), orders.end(),
                [&](const Order& o){ return o.master != masterName; }), orders.end());

        WORD countColor = showIssued ? Color::DIM : Color::MENU;
        std::string countLabel = (showIssued ? "Выданных: " : "Активных: ") + std::to_string(orders.size());
        if (myOnly) countLabel += "  (мои)";
        UIManager::printCentered(4, countLabel, countColor);
        UIManager::drawOrdersTable(orders, 5);

        int tableBottom = 5 + (int)orders.size() + 2;
        UIManager::printCentered(tableBottom + 1, "  Введите ID заказа:", Color::MENU);

        UIManager::showCursor();
        int first = normalizeKey(_getch());
        UIManager::hideCursor();

        if (first == KEY_ESC) return;
        if (first == 'v') { showIssued = !showIssued; continue; }
        if (first == 'm') { myOnly = !myOnly; continue; }
        if (first < '0' || first > '9') continue;

        std::string idStr(1, (char)first);
        UIManager::setCursor((UIManager::getConsoleSize().X - 20) / 2 + 20, tableBottom + 1);
        UIManager::setColor(Color::DEFAULT);
        std::cout << idStr;
        while (true) {
            int ch = _getch();
            if (ch == 13) break;
            if (ch == KEY_ESC) { idStr = ""; break; }
            if (ch == 8 && !idStr.empty()) { idStr.pop_back(); }
            else if (ch >= '0' && ch <= '9') idStr += (char)ch;
            UIManager::setCursor((UIManager::getConsoleSize().X - 20) / 2 + 20, tableBottom + 1);
            UIManager::setColor(Color::DEFAULT);
            std::cout << idStr << "  ";
        }
        if (idStr.empty()) continue;

        int id = 0;
        try { id = std::stoi(idStr); } catch (...) {}
        if (id <= 0 || !om.findById(id)) {
            UIManager::printError(tableBottom + 3, "Заказ не найден!");
            UIManager::waitKey(tableBottom + 5);
            continue;
        }
        screenManageOrder(om, id, masterName);
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

// forward declaration
void screenManageOrder(OrderManager& om, int id, const std::string& masterName);

// ============================================================
// Экран: Поиск заказов
// ============================================================
void screenSearch(OrderManager& om, const std::string& masterName) {
    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Поиск заказов ===", Color::LOGO);
        UIManager::printHLine(2, 40, '-', Color::DIM);
        UIManager::printCentered(4, "  [1]  По мастеру (выбор из списка)  ", Color::MENU);
        UIManager::printCentered(5, "  [2]  По ID заказа                  ", Color::MENU);
        UIManager::printCentered(6, "  [3]  По совпадению текста           ", Color::MENU);
        UIManager::printCentered(8, "  [ESC] Назад  ", Color::DEFAULT);

        int ch = _getch();
        if (ch == KEY_ESC) return;
        if (ch == 0 || ch == 224) { _getch(); continue; }
        if (ch != '1' && ch != '2' && ch != '3') continue;

        std::vector<Order> results;

        // ---- По мастеру ----
        if (ch == '1') {
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
        else if (ch == '2') {
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

            COORD sz = UIManager::getConsoleSize();
            const int C_ID=5,C_NAME=18,C_TEL=14,C_DESC=20,C_PRC=9,C_STS=10,C_DATE=12,C_DL=12,C_MST=16;
            const int TW=C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS+C_DATE+C_DL+C_MST;
            int indent=(sz.X-TW)/2; if(indent<0)indent=0;

            UIManager::setColor(Color::TABLE_HDR);
            UIManager::setCursor(indent,4);                                                    std::cout<<"ID";
            UIManager::setCursor(indent+C_ID,4);                                               std::cout<<"ФИО клиента";
            UIManager::setCursor(indent+C_ID+C_NAME,4);                                        std::cout<<"Телефон";
            UIManager::setCursor(indent+C_ID+C_NAME+C_TEL,4);                                  std::cout<<"Описание";
            UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC,4);                           std::cout<<"Цена";
            UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC,4);                     std::cout<<"Статус";
            UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS,4);              std::cout<<"Приём";
            UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS+C_DATE,4);       std::cout<<"Дедлайн";
            UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS+C_DATE+C_DL,4);  std::cout<<"Мастер";
            UIManager::setCursor(indent,5); UIManager::setColor(Color::DIM);
            std::cout<<std::string(TW,'-');

            SYSTEMTIME st2; GetLocalTime(&st2);
            int today2=st2.wYear*10000+st2.wMonth*100+st2.wDay;
            auto d2i=[](const std::string& d)->int{
                if(d.size()<10)return 0;
                try{return std::stoi(d.substr(6,4))*10000+std::stoi(d.substr(3,2))*100+std::stoi(d.substr(0,2));}
                catch(...){return 0;}
            };

            for(int i=0;i<(int)results.size();++i){
                const Order& o=results[i];
                std::ostringstream ps; ps<<std::fixed<<std::setprecision(2)<<o.price;
                bool overdue=(d2i(o.deadline)>0&&d2i(o.deadline)<today2&&o.status!="Выдан");
                WORD rowColor=(i==sel)?Color::HIGHLIGHT:(overdue?Color::OVERDUE:Color::DEFAULT);
                if(i==sel){ UIManager::setCursor(indent-2,6+i); UIManager::setColor(Color::HIGHLIGHT); std::cout<<">"; }
                UIManager::setColor(rowColor);
                UIManager::setCursor(indent,6+i);                                                   std::cout<<o.id;
                UIManager::setCursor(indent+C_ID,6+i);                                              std::cout<<UIManager::trimVisual(o.clientName,C_NAME-1);
                UIManager::setCursor(indent+C_ID+C_NAME,6+i);                                       std::cout<<UIManager::trimVisual(o.phone,C_TEL-1);
                UIManager::setCursor(indent+C_ID+C_NAME+C_TEL,6+i);                                 std::cout<<UIManager::trimVisual(o.description,C_DESC-1);
                UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC,6+i);                          std::cout<<ps.str();
                UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC,6+i);                    std::cout<<UIManager::trimVisual(o.status,C_STS-1);
                UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS,6+i);             std::cout<<UIManager::trimVisual(o.dateReceived,C_DATE-1);
                UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS+C_DATE,6+i);      std::cout<<UIManager::trimVisual(o.deadline.empty()?"—":o.deadline,C_DL-1);
                UIManager::setCursor(indent+C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS+C_DATE+C_DL,6+i); std::cout<<UIManager::trimVisual(o.master.empty()?"—":o.master,C_MST-1);
            }

            int k=_getch();
            if(k==KEY_ESC) break;
            if(k==13){
                screenManageOrder(om,results[sel].id,masterName);
                // обновляем список (заказ мог измениться/удалиться)
                std::vector<Order> upd;
                for(const auto& r:results){ Order* op=om.findById(r.id); if(op) upd.push_back(*op); }
                results=upd;
                if(results.empty()) break;
                if(sel>=(int)results.size()) sel=(int)results.size()-1;
            }
            if(k==0||k==224){
                int k2=_getch();
                if(k2==72&&sel>0) --sel;
                if(k2==80&&sel<(int)results.size()-1) ++sel;
            }
        }
    }
}

void screenMainMenu(const std::string& login, const std::string& masterName, OrderManager& om) {
    while (true) {
        UIManager::drawMainMenuFull(masterName);
        int ch = normalizeKey(_getch());
        if      (ch == KEY_ESC) return;
        else if (ch == '1') screenAddOrder(om, masterName);
        else if (ch == '2') screenViewOrders(om, masterName);
        else if (ch == '3') screenManageOrders(om, masterName);
        else if (ch == '4') screenStats(om);
        else if (ch == '5') screenSearch(om, masterName);
    }
}

// ============================================================
// Экран: Гостевое меню (только просмотр)
// ============================================================
void screenGuestMenu(OrderManager& om) {
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
        UIManager::printCentered(menuRow++, "  [1]  Просмотр заказов         ", Color::MENU);
        UIManager::printCentered(menuRow++, "  [2]  Статистика               ", Color::MENU);
        UIManager::printCentered(menuRow++, "  [3]  Поиск                    ", Color::MENU);
        UIManager::printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        UIManager::printCentered(menuRow++, "  [ESC] Назад                   ", Color::DEFAULT);

        int ch = normalizeKey(_getch());
        if      (ch == KEY_ESC) return;
        else if (ch == '1') screenViewOrders(om, "");
        else if (ch == '2') screenStats(om);
        else if (ch == '3') screenSearch(om, "");
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
        UIManager::drawStartMenu(startRow + 12);

        int ch = normalizeKey(_getch());
        if (ch == KEY_ESC) {
            UIManager::clearScreen();
            UIManager::printCentered(sz.Y / 2, "До свидания! Спасибо за использование J.A.M.", Color::LOGO);
            UIManager::setColor(Color::DEFAULT);
            Sleep(1500);
            return;
        } else if (ch == '1') {
            std::string login = screenLogin(um);
            if (!login.empty()) screenMainMenu(login, um.getFullName(login), om);
        } else if (ch == '2') {
            screenRegister(um);
        } else if (ch == '3') {
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
