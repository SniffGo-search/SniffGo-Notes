// SniffGo Notes.cpp
// Simple console notes app: create, list, view, edit, delete notes saved as .txt files.
//
// Build: g++ -std=c++17 "SniffGo Notes.cpp" -o sniffgo-notes
// Run: ./sniffgo-notes

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

const std::string NOTES_DIR = "notes";

static std::string sanitize_filename(const std::string &s) {
    std::string out;
    for (char c : s) {
        // allow alnum, space, dash, underscore, dot
        if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_' || c == '.') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    // trim spaces from ends
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.front()))) out.erase(out.begin());
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) out.pop_back();
    if (out.empty()) out = "note";
    return out;
}

static fs::path unique_note_path(const std::string &base_title) {
    std::string base = sanitize_filename(base_title);
    std::string name = base + ".txt";
    fs::path p = fs::path(NOTES_DIR) / name;
    int idx = 1;
    while (fs::exists(p)) {
        name = base + " (" + std::to_string(idx) + ").txt";
        p = fs::path(NOTES_DIR) / name;
        ++idx;
    }
    return p;
}

static std::vector<fs::path> list_notes() {
    std::vector<fs::path> notes;
    if (!fs::exists(NOTES_DIR)) return notes;
    for (auto &entry : fs::directory_iterator(NOTES_DIR)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        if (p.extension() == ".txt") notes.push_back(p);
    }
    std::sort(notes.begin(), notes.end());
    return notes;
}

static void show_notes_indexed() {
    auto notes = list_notes();
    if (notes.empty()) {
        std::cout << "No notes found.\n";
        return;
    }
    for (size_t i = 0; i < notes.size(); ++i) {
        std::cout << (i + 1) << ") " << notes[i].filename().string() << '\n';
    }
}

static void create_note() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear
    std::string title;
    std::cout << "Enter note title: ";
    std::getline(std::cin, title);
    if (title.empty()) title = "note";
    fs::path p = unique_note_path(title);

    std::cout << "Enter note content. End with a single line containing only a dot (.)\n";
    std::ofstream ofs(p);
    if (!ofs) {
        std::cerr << "Failed to create note file: " << p << '\n';
        return;
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == ".") break;
        ofs << line << '\n';
    }
    ofs.close();
    std::cout << "Saved: " << p.string() << '\n';
}

static int pick_note_index() {
    auto notes = list_notes();
    if (notes.empty()) return -1;
    show_notes_indexed();
    std::cout << "Choose note number: ";
    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return -1;
    }
    if (choice < 1 || static_cast<size_t>(choice) > notes.size()) return -1;
    return choice - 1;
}

static void view_note() {
    int idx = pick_note_index();
    if (idx < 0) { std::cout << "Invalid selection.\n"; return; }
    auto notes = list_notes();
    fs::path p = notes[idx];
    std::ifstream ifs(p);
    if (!ifs) {
        std::cerr << "Failed to open: " << p << '\n';
        return;
    }
    std::cout << "---- " << p.filename().string() << " ----\n";
    std::string line;
    while (std::getline(ifs, line)) std::cout << line << '\n';
    std::cout << "---- end ----\n";
}

static void edit_note() {
    int idx = pick_note_index();
    if (idx < 0) { std::cout << "Invalid selection.\n"; return; }
    auto notes = list_notes();
    fs::path p = notes[idx];

    std::cout << "Edit options:\n1) Overwrite\n2) Append\nChoose: ";
    int opt = 0;
    if (!(std::cin >> opt)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input.\n";
        return;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // clear
    if (opt == 1) {
        std::ofstream ofs(p, std::ios::trunc);
        if (!ofs) { std::cerr << "Failed to open for writing.\n"; return; }
        std::cout << "Enter new content. End with a single line containing only a dot (.)\n";
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == ".") break;
            ofs << line << '\n';
        }
        std::cout << "Overwritten.\n";
    } else if (opt == 2) {
        std::ofstream ofs(p, std::ios::app);
        if (!ofs) { std::cerr << "Failed to open for appending.\n"; return; }
        std::cout << "Enter content to append. End with a single line containing only a dot (.)\n";
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == ".") break;
            ofs << line << '\n';
        }
        std::cout << "Appended.\n";
    } else {
        std::cout << "Unknown option.\n";
    }
}

static void delete_note() {
    int idx = pick_note_index();
    if (idx < 0) { std::cout << "Invalid selection.\n"; return; }
    auto notes = list_notes();
    fs::path p = notes[idx];
    std::cout << "Delete '" << p.filename().string() << "'? (y/N): ";
    char c;
    std::cin >> c;
    if (c == 'y' || c == 'Y') {
        std::error_code ec;
        fs::remove(p, ec);
        if (ec) std::cerr << "Failed to delete: " << ec.message() << '\n';
        else std::cout << "Deleted.\n";
    } else {
        std::cout << "Canceled.\n";
    }
}

int main() {
    try {
        fs::create_directories(NOTES_DIR);
    } catch (...) {
        std::cerr << "Failed to ensure notes directory exists.\n";
        return 1;
    }

    while (true) {
        std::cout << "\nSniffGo Notes - menu\n"
                  << "1) List notes\n"
                  << "2) Create note\n"
                  << "3) View note\n"
                  << "4) Edit note (overwrite/append)\n"
                  << "5) Delete note\n"
                  << "6) Exit\n"
                  << "Choose: ";
        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input.\n";
            continue;
        }
        switch (choice) {
            case 1: show_notes_indexed(); break;
            case 2: create_note(); break;
            case 3: view_note(); break;
            case 4: edit_note(); break;
            case 5: delete_note(); break;
            case 6: std::cout << "Goodbye.\n"; return 0;
            default: std::cout << "Unknown option.\n"; break;
        }
    }
    return 0;
}
