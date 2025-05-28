#include "core.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include <shellapi.h>
#include <locale>
#include <codecvt>

struct Answer
{
    int choice;
    int argc;
    std::vector<std::string> argv;

    char **argv_data() const
    {
        char **arr = new char *[argv.size()];
        for (size_t i = 0; i < argv.size(); ++i)
        {
            arr[i] = _strdup(argv[i].c_str());
        }
        return arr;
    }
};

std::string ws2utf8(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

Answer getInput()
{
    Answer answer;

    // --- Step 1: Get choice
    std::cout << "Select command (1: ktx, 2: fbx, 3: image): ";
    std::cin >> answer.choice;
    std::cin.ignore(); // flush newline

    if (answer.choice < 1 || answer.choice > 3)
        throw std::runtime_error("Choice must be between 1 and 3.");

    // --- Step 2: Get raw command line string
    std::wcout << L"Enter command line string (e.g. \"한글 띄어쓰기 한 파일.tga\"):\n> ";
    std::wstring winput;
    std::getline(std::wcin, winput);

    // --- Step 3: Parse to argc/argv (wide)
    int argc_w = 0;
    LPWSTR *argv_w = CommandLineToArgvW(winput.c_str(), &argc_w);
    if (!argv_w)
        throw std::runtime_error("Failed to parse command line.");

    // --- Step 4: Convert each wide arg to UTF-8
    for (int i = 0; i < argc_w; ++i)
        answer.argv.push_back(ws2utf8(argv_w[i]));

    answer.argc = static_cast<int>(answer.argv.size());
    LocalFree(argv_w);

    return answer;
}

// 🏁 Main entry
int main()
{
    // Enable UTF-8 mode in Windows console
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try
    {
        Answer answer = getInput();

        std::unique_ptr<char *[]> argv_holder(answer.argv_data()); // auto-cleanup

        switch (answer.choice)
        {
        case 1:
            vcpp_ktx(answer.argc, argv_holder.get());
            break;
        case 2:
            vcpp_fbx(answer.argc, argv_holder.get());
            break;
        case 3:
            vcpp_image(argv_holder.get());
            break;
        default:
            std::cerr << "Invalid choice.\n";
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[error] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
