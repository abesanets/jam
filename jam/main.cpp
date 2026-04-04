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

// ============================================================
// Экран: Добавление нового заказа
// ============================================================
void screenAddOrder(OrderManager& om) {
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

    int newId = om.addOrder(clientName, phone, description, price, deadline);
    UIManager::printSuccess(16, "Заказ добавлен! ID: " + std::to_string(newId));
    UIManager::waitKey(18);
}

// ============================================================
// Экран: Просмотр заказов с фильтрами
// ============================================================
void screenViewOrders(OrderManager& om) {
    std::string statusFilter = "";
    int sortMode = 0;    // 0=выкл, 1=ближе сначала, 2=дальше сначала
    bool showIssued = false;

    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Просмотр заказов ===", Color::LOGO);

        std::string fi = "Статус: ";
        fi += statusFilter.empty() ? "все активные" : statusFilter;
        fi += "  |  Дедлайн: ";
        fi += (sortMode == 0) ? "—" : (sortMode == 1 ? "ближе сначала" : "дальше сначала");
        fi += showIssued ? "  |  [выданные]" : "";
        UIManager::printCentered(2, fi, Color::DIM);

        UIManager::printCentered(3, "[N]овый [W]работе [G]отов [V]ыданные [D]едлайн [R]сброс [ESC]назад", Color::MENU);
        UIManager::printHLine(4, 80, '-', Color::DIM);

        if (showIssued) {
            // Только выданные
            std::vector<Order> issued = om.getFiltered("Выдан", 0, 0);
            UIManager::printCentered(5, "Выданных: " + std::to_string(issued.size()), Color::DIM);
            UIManager::drawOrdersTable(issued, 6);
        } else {
            // Активные (без выданных), с фильтром и сортировкой
            std::vector<Order> orders = om.getFiltered(statusFilter, 0, sortMode == 0 ? 0 : 1);
            // Убираем выданных если нет фильтра по статусу
            if (statusFilter.empty()) {
                orders.erase(std::remove_if(orders.begin(), orders.end(),
                    [](const Order& o){ return o.status == "Выдан"; }), orders.end());
            }
            if (sortMode == 2) std::reverse(orders.begin(), orders.end());
            UIManager::printCentered(5, "Показано: " + std::to_string(orders.size()), Color::MENU);
            UIManager::drawOrdersTable(orders, 6);
        }

        int ch = _getch();
        if (ch == KEY_ESC) return;
        if      (ch == 'd' || ch == 'D') { sortMode = (sortMode + 1) % 3; }
        else if (ch == 'v' || ch == 'V') { showIssued = !showIssued; statusFilter = ""; sortMode = 0; }
        else if (ch == 'r' || ch == 'R') { statusFilter = ""; sortMode = 0; showIssued = false; }
        else if (!showIssued) {
            if      (ch == 'n' || ch == 'N') statusFilter = (statusFilter == "Новый")    ? "" : "Новый";
            else if (ch == 'w' || ch == 'W') statusFilter = (statusFilter == "В работе") ? "" : "В работе";
            else if (ch == 'g' || ch == 'G') statusFilter = (statusFilter == "Готов")    ? "" : "Готов";
        }
    }
}

// ============================================================
// Экран: Подменю конкретного заказа
// ============================================================
void screenManageOrder(OrderManager& om, int id) {
    while (true) {
        Order* o = om.findById(id);
        if (!o) return;

        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::drawOrderDetail(*o, 1);

        int menuRow = 15;
        UIManager::printHLine(menuRow++, 46, '-', Color::DIM);
        UIManager::printCentered(menuRow++, "  [1] Изменить статус  ", Color::MENU);
        UIManager::printCentered(menuRow++, "  [2] Редактировать    ", Color::MENU);
        UIManager::printCentered(menuRow++, "  [3] Удалить заказ    ", Color::OVERDUE);
        UIManager::printCentered(menuRow++, "  [ESC] Назад          ", Color::DEFAULT);

        int ch = _getch();
        if (ch == KEY_ESC) return;

        if (ch == '1') {
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
        } else if (ch == '2') {
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
        } else if (ch == '3') {
            UIManager::clearScreen();
            UIManager::printCentered(3, "Удалить заказ #" + std::to_string(o->id) + " — " + o->clientName + "?", Color::OVERDUE);
            UIManager::printCentered(5, "  [Y] Да, удалить   [N / ESC] Отмена  ", Color::MENU);
            int conf = _getch();
            if (conf == 'y' || conf == 'Y') {
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
void screenManageOrders(OrderManager& om) {
    bool showIssued = false;

    while (true) {
        UIManager::clearScreen();
        UIManager::hideCursor();
        UIManager::printCentered(1, "=== Менеджмент заказов ===", Color::LOGO);

        std::string modeStr = showIssued ? "[V] показать активные" : "[V] показать выданные";
        UIManager::printCentered(2, modeStr + "  [ESC] назад", Color::MENU);
        UIManager::printHLine(3, 80, '-', Color::DIM);

        std::vector<Order> orders;
        if (showIssued) {
            orders = om.getFiltered("Выдан", 0, 0);
            UIManager::printCentered(4, "Выданных: " + std::to_string(orders.size()), Color::DIM);
        } else {
            orders = om.getAllOrders();
            orders.erase(std::remove_if(orders.begin(), orders.end(),
                [](const Order& o){ return o.status == "Выдан"; }), orders.end());
            UIManager::printCentered(4, "Активных: " + std::to_string(orders.size()), Color::MENU);
        }
        UIManager::drawOrdersTable(orders, 5);

        int tableBottom = 5 + (int)orders.size() + 2;
        UIManager::printCentered(tableBottom + 1, "  Введите ID заказа:", Color::MENU);

        // Читаем первый символ как команду или начало ID
        UIManager::showCursor();
        int first = _getch();
        UIManager::hideCursor();

        if (first == KEY_ESC) return;
        if (first == 'v' || first == 'V') { showIssued = !showIssued; continue; }
        if (first < '0' || first > '9') continue;

        // Собираем остаток числа
        std::string idStr(1, (char)first);
        UIManager::setCursor((UIManager::getConsoleSize().X - 20) / 2 + 20, tableBottom + 1);
        UIManager::setColor(Color::DEFAULT);
        std::cout << idStr;
        while (true) {
            int ch = _getch();
            if (ch == 13) break; // Enter
            if (ch == KEY_ESC) { idStr = ""; break; }
            if (ch == 8 && !idStr.empty()) { idStr.pop_back(); }
            else if (ch >= '0' && ch <= '9') idStr += (char)ch;
            // перерисовываем поле
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
        screenManageOrder(om, id);
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
// Экран: Регистрация
// ============================================================
void screenRegister(UserManager& um) {
    UIManager::clearScreen();
    UIManager::hideCursor();
    UIManager::printCentered(1, "=== Регистрация ===", Color::LOGO);
    UIManager::printHLine(2, 30, '-', Color::DIM);
    UIManager::printCentered(3, "[ESC] на любом поле — назад", Color::DIM);
    std::string login, password, confirm;
    if (!UIManager::inputStringESC(5, "  Придумайте логин:  ", login))    return;
    if (!UIManager::inputStringESC(7, "  Придумайте пароль: ", password)) return;
    if (!UIManager::inputStringESC(9, "  Повторите пароль:  ", confirm))  return;
    if (password != confirm) {
        UIManager::printError(11, "Пароли не совпадают!");
        UIManager::waitKey(13);
        return;
    }
    if (um.registerUser(login, password))
        UIManager::printSuccess(11, "Регистрация прошла успешно! Войдите в систему.");
    else
        UIManager::printError(11, "Логин уже занят или поля пустые!");
    UIManager::waitKey(13);
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
// Главное меню
// ============================================================
void screenMainMenu(const std::string& login, OrderManager& om) {
    while (true) {
        UIManager::drawMainMenuFull(login);
        int ch = _getch();
        if      (ch == KEY_ESC) return;
        else if (ch == '1') screenAddOrder(om);
        else if (ch == '2') screenViewOrders(om);
        else if (ch == '3') screenManageOrders(om);
        else if (ch == '4') screenStats(om);
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

        int ch = _getch();
        if (ch == KEY_ESC) {
            UIManager::clearScreen();
            UIManager::printCentered(sz.Y / 2, "До свидания! Спасибо за использование J.A.M.", Color::LOGO);
            UIManager::setColor(Color::DEFAULT);
            Sleep(1500);
            return;
        } else if (ch == '1') {
            std::string login = screenLogin(um);
            if (!login.empty()) screenMainMenu(login, om);
        } else if (ch == '2') {
            screenRegister(um);
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
