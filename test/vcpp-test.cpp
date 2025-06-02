#include "core/core.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include <shellapi.h>
#include <locale>
#include <codecvt>

struct ArgResult
{
    int choice = 0;
    int argc = 0;
    std::vector<std::string> argv_strings;
    std::vector<char *> argv;

    // 명시적으로 반환 타입을 char**로 변환
    char **argv_data()
    {
        return argv.empty() ? nullptr : &argv[0];
    }
};

ArgResult getInput()
{
    ArgResult result;

    // 1. 명령 선택
    while (true)
    {
        std::cout << u8"명령을 선택하세요:\n  1. KTX\n  2. FBX\n  3. 이미지\n> " << std::flush;
        std::wstring choice_input;
        std::getline(std::wcin, choice_input);

        if (choice_input == L"1" || choice_input == L"2" || choice_input == L"3")
        {
            result.choice = std::stoi(choice_input);
            break;
        }
        else
        {
            std::wcout << L"잘못된 입력입니다. 1, 2, 3 중 하나를 입력하세요.\n";
        }
    }

    // 2. 명령줄 입력
    std::cout << u8"명령줄 인자를 입력하세요 (예: \"input file\" -o \"output file\"):\n> ";
    std::wstring input_line;
    std::getline(std::wcin, input_line);

    // 3. 파싱
    int argc_w = 0;
    LPWSTR *argv_w = CommandLineToArgvW(input_line.c_str(), &argc_w);
    if (!argv_w)
    {
        std::wcerr << L"CommandLineToArgvW 실패\n";
        result.argc = 0;
        return result;
    }

    for (int i = 0; i < argc_w; ++i)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1, utf8.data(), len, nullptr, nullptr);

        result.argv_strings.push_back(std::move(utf8));
    }

    LocalFree(argv_w);

    for (const auto &str : result.argv_strings)
        result.argv.push_back(const_cast<char *>(str.c_str()));

    result.argc = static_cast<int>(result.argv.size());
    return result;
}

// 🏁 Main entry
int main()
{
    // Enable UTF-8 mode in Windows console
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try
    {
        ArgResult answer = getInput();

        switch (answer.choice)
        {
        case 1:
            vcpp_ktx(answer.argc, answer.argv_data());
            break;
        case 2:
            vcpp_fbx(answer.argc, answer.argv_data());
            break;
        case 3:
            vcpp_image_denoise("{ \"input\" : \"d:/work/voidn/input.hdr\", \"output\" : \"d:/work/voidn/ggg.hdr\" }");
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

// #include <OpenImageDenoise/oidn.hpp>
// #include <iostream>

// int main()
// {
//     std::cout << "Start OIDN test\n";
//     try
//     {
//         std::cout << "Creating OIDN device...\n";
//         auto device = oidn::newDevice(oidn::DeviceType::CUDA);
//         std::cout << "New device\n";
//         device.commit();

//         std::cout << "OIDN device created successfully.\n";

//         const char *errMsg;
//         if (device.getError(errMsg) != oidn::Error::None)
//         {
//             std::cerr << "OIDN error: " << errMsg << "\n";
//         }
//         else
//         {
//             std::cout << "Device created and committed successfully.\n";
//         }
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Caught exception: " << e.what() << "\n";
//     }

//     return 0;
// }