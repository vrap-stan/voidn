#include "core.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

struct Answer
{
    int choice;
    int argc;
    std::vector<char *> argv;
    std::vector<std::unique_ptr<char[]>> storage; // <- 메모리 유지용

    char **argv_data()
    {
        return argv.data();
    }
};

Answer getInput()
{
    int choice = 0;

    std::cout << "[1] KTX\n[2] FBX\n[3] Denoise\n Enter :";
    std::cin >> choice;

    if (choice < 1 || choice > 3)
    {
        std::cerr << "Invalid choice.\n";
        throw std::runtime_error("Invalid choice");
    }

    std::cin.ignore(); // flush

    std::string line;
    std::cout << "Enter command: ";
    std::getline(std::cin, line);

    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }

    Answer answer;
    answer.choice = choice;
    answer.argc = static_cast<int>(tokens.size());

    for (const auto &s : tokens)
    {
        std::unique_ptr<char[]> buf(new char[s.size() + 1]);
        std::copy(s.begin(), s.end(), buf.get());
        buf[s.size()] = '\0';
        answer.argv.push_back(buf.get());
        answer.storage.push_back(std::move(buf)); // 유지용
    }

    return answer;
}

int main()
{
    auto answer = getInput();

    switch (answer.choice)
    {
    case 1:
        vcpp_ktx(answer.argc, answer.argv_data());
        break;
    case 2:
        vcpp_fbx(answer.argc, answer.argv_data());
        break;
    case 3:
        vcpp_image(answer.argv_data());
        break;
    default:
        std::cerr << "Invalid choice.\n";
        return 1;
    }

    return 0;
}
