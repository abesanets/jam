#pragma once
#include <windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "models.h"

namespace Color {
    const WORD DEFAULT   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD LOGO      = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD MENU      = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD HIGHLIGHT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD ERROR_CLR = FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD SUCCESS   = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD TABLE_HDR = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD DIM       = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD OVERDUE   = FOREGROUND_RED | FOREGROUND_INTENSITY;
}

class UIManager {
public:
    static COORD getConsoleSize() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        COORD size = {80, 25};
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            size.X = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
            size.Y = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
        }
        return size;
    }
    static void setCursor(int x, int y) {
        COORD pos = {(SHORT)x, (SHORT)y};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    }
    static void setColor(WORD color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }
    static void clearScreen() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD count;
        COORD home = {0, 0};
        if (!GetConsoleScreenBufferInfo(h, &csbi)) return;
        DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(h, ' ', cells, home, &count);
        FillConsoleOutputAttribute(h, Color::DEFAULT, cells, home, &count);
        SetConsoleCursorPosition(h, home);
    }
    static void hideCursor() {
        CONSOLE_CURSOR_INFO cci = {1, FALSE};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);
    }
    static void showCursor() {
        CONSOLE_CURSOR_INFO cci = {10, TRUE};
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);
    }
    static int visualWidth(const std::string& s) {
        int w = 0;
        for (size_t i = 0; i < s.size(); ) {
            unsigned char c = (unsigned char)s[i];
            if      (c < 0x80)           { ++i;    ++w; }
            else if ((c & 0xE0) == 0xC0) { i += 2; ++w; }
            else if ((c & 0xF0) == 0xE0) { i += 3; ++w; }
            else if ((c & 0xF8) == 0xF0) { i += 4; ++w; }
            else                         { ++i;    ++w; }
        }
        return w;
    }
    static void printCentered(int row, const std::string& text, WORD color = Color::DEFAULT) {
        COORD sz = getConsoleSize();
        int col = (sz.X - visualWidth(text)) / 2;
        if (col < 0) col = 0;
        setCursor(col, row);
        setColor(color);
        std::cout << text;
    }
    static void printHLine(int row, int width, char ch = '-', WORD color = Color::DIM) {
        printCentered(row, std::string(width, ch), color);
    }
    static void printError(int row, const std::string& msg) {
        printCentered(row, "  [!] " + msg + "  ", Color::ERROR_CLR);
    }
    static void printSuccess(int row, const std::string& msg) {
        printCentered(row, "  [OK] " + msg + "  ", Color::MENU);
    }
    static void waitKey(int row) {
        printCentered(row, "  Нажмите любую клавишу...  ", Color::DIM);
        _getch();
    }
    static std::string inputString(int row, const std::string& label) {
        showCursor();
        COORD sz = getConsoleSize();
        int col = (sz.X - 40) / 2;
        if (col < 0) col = 0;
        setCursor(col, row); setColor(Color::HIGHLIGHT);
        std::cout << label;
        setCursor(col + visualWidth(label), row); setColor(Color::DEFAULT);
        std::string result;
        std::getline(std::cin, result);
        hideCursor();
        return result;
    }
    static bool inputStringESC(int row, const std::string& label, std::string& result) {
        showCursor();
        COORD sz = getConsoleSize();
        int col = (sz.X - 40) / 2;
        if (col < 0) col = 0;
        int lw = visualWidth(label);
        setCursor(col, row); setColor(Color::HIGHLIGHT);
        std::cout << label;
        setColor(Color::DEFAULT);
        result.clear();
        while (true) {
            int rw = visualWidth(result);
            setCursor(col + lw, row);
            std::cout << result << ' ';
            setCursor(col + lw + rw, row);
            int ch = _getch();
            if (ch == 27) { hideCursor(); return false; }
            if (ch == 13) { hideCursor(); return true;  }
            if (ch == 8) {
                if (!result.empty()) {
                    size_t i = result.size();
                    while (i > 0 && (result[i-1] & 0xC0) == 0x80) --i;
                    if (i > 0) --i;
                    result = result.substr(0, i);
                    // затираем строку целиком (с запасом)
                    setCursor(col + lw, row);
                    std::cout << std::string(rw + 2, ' ');
                }
            } else if (ch == 0 || ch == 224) {
                _getch();
            } else if (ch >= 32) {
                result += (char)ch;
            }
        }
    }
    static std::string trimVisual(const std::string& s, int maxW) {
        int width = 0;
        size_t i = 0;
        while (i < s.size()) {
            unsigned char c = (unsigned char)s[i];
            int b = (c < 0x80) ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
            if (width + 1 > maxW) return s.substr(0, i) + "..";
            ++width; i += b;
        }
        return s;
    }

    // ============================================================
    // Логотип J.A.M. — J.A.M. с точкой после каждой буквы
    // Формат: J. A. M.  (figlet "big" стиль)
    // ============================================================
    static void drawLogo(int startRow) {
        COORD sz = getConsoleSize();
        const char* logoRaw[] = {
            "     \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   ",
            "     \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   ",
            "     \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   ",
            "\xe2\x96\x88\xe2\x96\x88   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   ",
            "\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91 \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97",
            " \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d     \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d",
            nullptr
        };
        std::vector<std::string> logo;
        for (int i = 0; logoRaw[i]; ++i) logo.push_back(logoRaw[i]);
        int maxW = 0;
        for (auto& l : logo) { int w = visualWidth(l); if (w > maxW) maxW = w; }
        int indent = (sz.X - maxW) / 2;
        if (indent < 0) indent = 0;
        int row = startRow;
        row++; // верхняя линия рисуется снаружи (меню/стартовый экран)
        for (auto& l : logo) {
            int col = (sz.X - visualWidth(l)) / 2;
            if (col < 0) col = 0;
            setCursor(col, row++); setColor(Color::LOGO);
            std::cout << l;
        }
        row++;
        // Точки между буквами в подписи
        const char* sub[] = {
            "J . A . M.  -  Jewelry Atelier Manager",
            "\xd0\x90\xd0\xa0\xd0\x9c \xd1\x8e\xd0\xb2\xd0\xb5\xd0\xbb\xd0\xb8\xd1\x80\xd0\xbd\xd0\xbe\xd0\xb9 \xd0\xbc\xd0\xb0\xd1\x81\xd1\x82\xd0\xb5\xd1\x80\xd1\x81\xd0\xba\xd0\xbe\xd0\xb9",
            nullptr
        };
        for (int i = 0; sub[i]; ++i) {
            std::string l(sub[i]);
            int col = (sz.X - visualWidth(l)) / 2;
            if (col < 0) col = 0;
            setCursor(col, row++); setColor(Color::HIGHLIGHT);
            std::cout << l;
        }
        row++;
        setCursor(indent, row); setColor(Color::LOGO);
    }

    static void drawStartMenu(int startRow) {
        int row = startRow;
        printCentered(row++, std::string(26, '='), Color::LOGO);
        printCentered(row++, "  Выберите действие:    ", Color::MENU);
        printCentered(row++, std::string(26, '-'), Color::LOGO);
        printCentered(row++, "  [1]  Вход в систему   ", Color::MENU);
        printCentered(row++, "  [2]  Регистрация      ", Color::MENU);
        printCentered(row++, "  [3]  Войти как гость  ", Color::DIM);
        printCentered(row++, "  [ESC] Выход           ", Color::DEFAULT);
        printCentered(row++, std::string(26, '-'), Color::LOGO);
    }

    // Главное меню — теперь с логотипом сверху
    static void drawMainMenuFull(const std::string& login) {
        clearScreen();
        hideCursor();
        COORD sz = getConsoleSize();
        int logoHeight = 11; // строк занимает drawLogo
        int startRow = (sz.Y - logoHeight - 12) / 2;
        if (startRow < 1) startRow = 1;
        drawLogo(startRow);
        int menuRow = startRow + logoHeight + 2;
        printCentered(menuRow++, std::string(38, '='), Color::LOGO);
        printCentered(menuRow++, "  Добро пожаловать, " + login + "!  ", Color::MENU);
        printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        printCentered(menuRow++, "  [1]  Добавить заказ           ", Color::MENU);
        printCentered(menuRow++, "  [2]  Просмотр заказов         ", Color::MENU);
        printCentered(menuRow++, "  [3]  Менеджмент заказов       ", Color::MENU);
        printCentered(menuRow++, "  [4]  Статистика               ", Color::MENU);
        printCentered(menuRow++, std::string(38, '-'), Color::LOGO);
        printCentered(menuRow++, "  [ESC] Выйти из аккаунта       ", Color::DEFAULT);
    }

    // Детальный просмотр одного заказа
    static void drawOrderDetail(const Order& o, int startRow) {
        COORD sz = getConsoleSize();
        int row = startRow;
        printHLine(row++, 46, '=', Color::LOGO);
        printCentered(row++, "  Детальный просмотр заказа #" + std::to_string(o.id) + "  ", Color::LOGO);
        printHLine(row++, 46, '=', Color::LOGO);
        row++;

        auto printField = [&](const std::string& label, const std::string& value, WORD col = Color::DEFAULT) {
            std::string line = "  " + label + value;
            int x = (sz.X - 46) / 2;
            if (x < 0) x = 0;
            setCursor(x, row); setColor(Color::TABLE_HDR);
            std::cout << label;
            setCursor(x + visualWidth(label), row); setColor(col);
            std::cout << value;
            row++;
        };

        printField("ID:              ", std::to_string(o.id));
        printField("ФИО клиента:     ", o.clientName);
        printField("Телефон:         ", o.phone);
        printField("Описание:        ", o.description);
        std::ostringstream ps; ps << std::fixed << std::setprecision(2) << o.price;
        printField("Цена (руб):      ", ps.str());
        printField("Статус:          ", o.status, Color::HIGHLIGHT);
        printField("Дата приёма:     ", o.dateReceived.empty() ? "—" : o.dateReceived);
        printField("Мастер:          ", o.master.empty() ? "—" : o.master, Color::HIGHLIGHT);
        {
            SYSTEMTIME st; GetLocalTime(&st);
            int today = st.wYear * 10000 + st.wMonth * 100 + st.wDay;
            auto dateToInt = [](const std::string& d) -> int {
                if (d.size() < 10) return 0;
                try { return std::stoi(d.substr(6,4))*10000 + std::stoi(d.substr(3,2))*100 + std::stoi(d.substr(0,2)); }
                catch (...) { return 0; }
            };
            int dl = dateToInt(o.deadline);
            bool overdue = (dl > 0 && dl < today && o.status != "Выдан");
            WORD dlColor = o.deadline.empty() ? Color::DIM : (overdue ? Color::OVERDUE : Color::DEFAULT);
            printField("Дедлайн:         ", o.deadline.empty() ? "—" : o.deadline, dlColor);
        }
        row++;
        printHLine(row++, 46, '-', Color::DIM);
    }

    // ============================================================
    // Таблица заказов с фильтрами
    // Возвращает строку после таблицы
    // ============================================================
    static int drawOrdersTable(const std::vector<Order>& orders, int startRow) {
        COORD sz = getConsoleSize();
        // Колонки: ID | ФИО | Телефон | Описание | Цена | Статус | Приём | Дедлайн
        const int C_ID=5, C_NAME=18, C_TEL=14, C_DESC=20, C_PRC=9, C_STS=10, C_DATE=12, C_DL=12, C_MST=16;
        const int TW = C_ID+C_NAME+C_TEL+C_DESC+C_PRC+C_STS+C_DATE+C_DL+C_MST;
        int indent = (sz.X - TW) / 2; if (indent < 0) indent = 0;
        int xID=indent, xNAME=xID+C_ID, xTEL=xNAME+C_NAME;
        int xDESC=xTEL+C_TEL, xPRC=xDESC+C_DESC, xSTS=xPRC+C_PRC;
        int xDATE=xSTS+C_STS, xDL=xDATE+C_DATE, xMST=xDL+C_DL;
        int row = startRow;

        // Заголовок
        setColor(Color::TABLE_HDR);
        setCursor(xID,   row); std::cout << "ID";
        setCursor(xNAME, row); std::cout << "ФИО клиента";
        setCursor(xTEL,  row); std::cout << "Телефон";
        setCursor(xDESC, row); std::cout << "Описание";
        setCursor(xPRC,  row); std::cout << "Цена";
        setCursor(xSTS,  row); std::cout << "Статус";
        setCursor(xDATE, row); std::cout << "Приём";
        setCursor(xDL,   row); std::cout << "Дедлайн";
        setCursor(xMST,  row); std::cout << "Мастер";
        row++;
        setCursor(indent, row++); setColor(Color::DIM);
        std::cout << std::string(TW, '-');

        if (orders.empty()) {
            setCursor(indent, row++); setColor(Color::ERROR_CLR);
            std::cout << "  Заказы не найдены.";
        }

        // Текущая дата для подсветки просроченных
        SYSTEMTIME st; GetLocalTime(&st);
        int today = st.wYear * 10000 + st.wMonth * 100 + st.wDay;
        auto dateToInt = [](const std::string& d) -> int {
            if (d.size() < 10) return 0;
            try {
                return std::stoi(d.substr(6,4))*10000 + std::stoi(d.substr(3,2))*100 + std::stoi(d.substr(0,2));
            } catch (...) { return 0; }
        };

        for (const auto& o : orders) {
            std::ostringstream ps;
            ps << std::fixed << std::setprecision(2) << o.price;
            int dl = dateToInt(o.deadline);
            bool overdue = (dl > 0 && dl < today && o.status != "Выдан");
            WORD rowColor = overdue ? Color::OVERDUE : Color::DEFAULT;
            setColor(rowColor);
            setCursor(xID,   row); std::cout << o.id;
            setCursor(xNAME, row); std::cout << trimVisual(o.clientName,  C_NAME-1);
            setCursor(xTEL,  row); std::cout << trimVisual(o.phone,       C_TEL-1);
            setCursor(xDESC, row); std::cout << trimVisual(o.description, C_DESC-1);
            setCursor(xPRC,  row); std::cout << ps.str();
            setCursor(xSTS,  row); std::cout << trimVisual(o.status,      C_STS-1);
            setCursor(xDATE, row); std::cout << trimVisual(o.dateReceived, C_DATE-1);
            setCursor(xDL,   row); std::cout << trimVisual(o.deadline.empty() ? "—" : o.deadline, C_DL-1);
            setCursor(xMST,  row); std::cout << trimVisual(o.master.empty() ? "—" : o.master, C_MST-1);
            row++;
        }
        setCursor(indent, row); setColor(Color::DIM);
        std::cout << std::string(TW, '-');
        setColor(Color::DEFAULT);
        return row + 1;
    }
};
