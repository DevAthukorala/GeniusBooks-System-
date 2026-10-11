#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
using Money = long long;
constexpr int Limit = 1000000;
struct Book { int id; std::string title, author; Money price; int stock; };
struct Sale { int id, bookId, quantity; Money price; std::string title, date; };
struct Store { std::vector<Book> books; std::vector<Sale> sales; };
struct EndInput {};
std::string trim(std::string s) {
    auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
std::string input(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) throw EndInput{};
    return trim(s);
}
bool integer(const std::string& s, long long& n) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) return false;
    try { n = std::stoll(s); return true; } catch (...) { return false; }
}
int number(const std::string& prompt, int minimum, int maximum) {
    for (;;) {
        long long n;
        if (integer(input(prompt), n) && n >= minimum && n <= maximum) return static_cast<int>(n);
        std::cout << "Enter a whole number from " << minimum << " to " << maximum << ".\n";
    }
}
std::string required(const std::string& prompt) {
    for (;;) {
        auto s = input(prompt);
        if (!s.empty() && s.size() <= 200) return s;
        std::cout << "Enter 1 to 200 characters.\n";
    }
}
Money priceInput() {
    for (;;) {
        auto s = input("Price (e.g. 1250.50): ");
        auto dot = s.find('.');
        auto fraction = dot == std::string::npos ? "00" : s.substr(dot + 1);
        if (fraction.size() == 1) fraction += '0';
        long long major, minor;
        if (integer(s.substr(0, dot), major) && fraction.size() == 2 && integer(fraction, minor)
            && major <= Limit && major * 100 + minor <= Limit * 100LL) return major * 100 + minor;
        std::cout << "Enter a price from 0 to 1000000 with at most two decimal places.\n";
    }
}
std::string money(Money cents) {
    std::ostringstream out;
    out << cents / 100 << '.' << std::setw(2) << std::setfill('0') << cents % 100;
    return out.str();
}
std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
void show(const Book& b) {
    std::cout << "#" << b.id << " | " << b.title << " | " << b.author
        << " | Price: " << money(b.price) << " | Stock: " << b.stock << '\n';
}
Book* findBook(Store& store, int id) {
    for (auto& b : store.books) if (b.id == id) return &b;
    return nullptr;
}
void save(const Store& store, const fs::path& path) {
    fs::path temporary = path.wstring() + L".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open temporary data file.");
    out << "GENIUSBOOKS 1\n";
    for (const auto& b : store.books)
        out << "B " << b.id << ' ' << b.price << ' ' << b.stock << ' ' << std::quoted(b.title) << ' ' << std::quoted(b.author) << '\n';
    for (const auto& s : store.sales)
        out << "S " << s.id << ' ' << s.bookId << ' ' << s.quantity << ' ' << s.price << ' ' << std::quoted(s.title) << ' ' << std::quoted(s.date) << '\n';
    out.close();
    if (!out) throw std::runtime_error("Write failed. Existing data was preserved.");
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace data file. Existing data was preserved.");
}
Store load(const fs::path& path) {
    Store store;
    if (!fs::exists(path)) return store;
    std::ifstream in(path);
    std::string line;
    if (!std::getline(in, line) || line != "GENIUSBOOKS 1") throw std::runtime_error("Invalid or unreadable data file; it will not be overwritten.");
    while (std::getline(in, line)) {
        std::istringstream row(line);
        char type;
        bool valid = false;
        if (row >> type) {
            if (type == 'B') {
                Book b{};
                if (row >> b.id >> b.price >> b.stock >> std::quoted(b.title) >> std::quoted(b.author)) {
                    valid = b.id > 0 && b.id <= Limit && b.price >= 0 && b.price <= Limit * 100LL && b.stock >= 0 && b.stock <= Limit
                        && !trim(b.title).empty() && b.title.size() <= 200 && !trim(b.author).empty() && b.author.size() <= 200 && !findBook(store, b.id);
                    if (valid) store.books.push_back(b);
                }
            } else if (type == 'S') {
                Sale s{};
                if (row >> s.id >> s.bookId >> s.quantity >> s.price >> std::quoted(s.title) >> std::quoted(s.date)) {
                    valid = s.id > 0 && s.id <= Limit && findBook(store, s.bookId) && s.quantity > 0 && s.quantity <= Limit
                        && s.price >= 0 && s.price <= Limit * 100LL && !s.title.empty() && s.title.size() <= 200 && !s.date.empty() && s.date.size() <= 40
                        && std::none_of(store.sales.begin(), store.sales.end(), [&](const Sale& old) { return old.id == s.id; });
                    if (valid) store.sales.push_back(s);
                }
            }
        }
        row >> std::ws;
        if (!valid || !row.eof()) throw std::runtime_error("Corrupt data record; data will not be overwritten.");
    }
    if (in.bad()) throw std::runtime_error("Failed to read data file.");
    return store;
}
template<class T> int nextId(const std::vector<T>& records) {
    int id = 1;
    for (const auto& r : records) id = std::max(id, r.id + 1);
    if (id > Limit) throw std::runtime_error("Record limit reached.");
    return id;
}
std::string timestamp() {
    auto now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}
void run(Store& store, const fs::path& path) {
    for (;;) {
        std::cout << "\n=== GeniusBooks ===\n1. Add book\n2. View books\n3. Search books\n4. Update book / stock / price\n5. Record sale\n6. Sales history\n0. Exit\n";
        int choice = number("Choose: ", 0, 6);
        if (!choice) return;
        if (choice == 2 || choice == 3) {
            auto query = choice == 3 ? lower(required("Search title, author, or exact book ID: ")) : "";
            int count = 0;
            for (const auto& b : store.books) {
                if (query.empty() || lower(b.title).find(query) != std::string::npos || lower(b.author).find(query) != std::string::npos || std::to_string(b.id) == query) { show(b); ++count; }
            }
            if (!count) std::cout << "No books found.\n";
            continue;
        }
        if (choice == 6) {
            Money total = 0;
            long long units = 0;
            for (const auto& s : store.sales) {
                Money amount = s.price * s.quantity;
                std::cout << "Sale #" << s.id << " | " << s.date << " | Book #" << s.bookId << " " << s.title
                    << " | Qty: " << s.quantity << " | Unit price: " << money(s.price) << " | Total: " << money(amount) << '\n';
                // Bound the aggregate before adding even when loading a very large history.
                if (total > LLONG_MAX - amount) throw std::runtime_error("Revenue total exceeds supported range.");
                total += amount; units += s.quantity;
            }
            std::cout << "Sales: " << store.sales.size() << " | Units sold: " << units << " | Revenue: " << money(total) << '\n';
            continue;
        }
        try {
            // Only change live state after the complete snapshot has been saved.
            Store changed = store;
            if (choice == 1) {
                Book b{};
                b.id = nextId(changed.books);
                b.title = required("Title: "); b.author = required("Author: ");
                b.price = priceInput(); b.stock = number("Stock: ", 0, Limit);
                changed.books.push_back(b);
            } else {
                Book* b = findBook(changed, number("Book ID: ", 1, Limit));
                if (!b) { std::cout << "Book not found.\n"; continue; }
                show(*b);
                if (choice == 4) {
                    int field = number("Update 1. Title  2. Author  3. Price  4. Stock  0. Cancel: ", 0, 4);
                    if (!field) continue;
                    if (field == 1) b->title = required("New title: ");
                    if (field == 2) b->author = required("New author: ");
                    if (field == 3) b->price = priceInput();
                    if (field == 4) b->stock = number("New stock: ", 0, Limit);
                } else {
                    if (!b->stock) { std::cout << "Out of stock.\n"; continue; }
                    int qty = number("Quantity (0 cancels): ", 0, b->stock);
                    if (!qty) continue;
                    std::cout << "Sale total: " << money(b->price * qty) << '\n';
                    if (lower(input("Confirm sale (y/n): ")) != "y") { std::cout << "Sale cancelled.\n"; continue; }
                    changed.sales.push_back({nextId(changed.sales), b->id, qty, b->price, b->title, timestamp()});
                    b->stock -= qty;
                }
            }
            save(changed, path);
            store = std::move(changed);
            std::cout << "Saved successfully.\n";
        } catch (const std::exception& e) { std::cout << "Change not saved: " << e.what() << '\n'; }
    }
}
int main(int argc, char* argv[]) {
    try {
        if (argc > 2) { std::cerr << "Usage: GeniusBooks [data-file]\n"; return 1; }
        fs::path path;
        if (argc == 2) path = fs::absolute(argv[1]);
        else {
            wchar_t executable[32768];
            DWORD size = GetModuleFileNameW(nullptr, executable, 32768);
            if (!size || size >= 32768) throw std::runtime_error("Cannot locate application folder.");
            path = fs::path(executable).parent_path() / "geniusbooks.data";
        }
        fs::path lockPath = path.wstring() + L".lock";
        HANDLE lock = CreateFileW(lockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        if (lock == INVALID_HANDLE_VALUE) throw std::runtime_error("Data is in use or its folder is not writable.");
        struct LockGuard { HANDLE handle; ~LockGuard() { CloseHandle(handle); } } guard{lock};
        Store store = load(path);
        std::cout << "Data file: " << path.string() << "\nPrices use your store's currency.\n";
        try { run(store, path); } catch (const EndInput&) {}
        std::cout << "Goodbye. All completed changes are saved.\n";
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
    return 0;
}
