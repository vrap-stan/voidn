// ktx_main.cpp

#include "ktx_main.h"       // For KTX_API and ktx_main declaration
#include "command.h"        // For ktx::Command, ktx::pfnBuiltinCommand, KTX_COMMAND_BUILTIN, rc, etc.
#include "platform_utils.h" // For the ORIGINAL InitUTF8CLI, version(), CONSOLE_USAGE_WIDTH
#include "stdafx.h"         // If used
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>  // For std::vector
#include <memory>  // For std::unique_ptr
#include <cstdlib> // For std::getenv

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>

#if defined(_WIN32)
// For _setmode, _fileno, _O_BINARY if you decide to add minimal init
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

// -------------------------------------------------------------------------------------------------
// ktx::Tools class and its methods (main, printUsage) - Copy from your KTX source
// -------------------------------------------------------------------------------------------------
namespace ktx
{

    class Tools : public Command
    {
        bool testrun = false;

    public:
        using Command::Command;
        virtual ~Tools() {};
        virtual int main(int argc, char *argv[]) override;
        void printUsage(std::ostream &os, const cxxopts::Options &options);
    };

    int Tools::main(int argc, char *argv[])
    {
        cxxopts::Options options("ktx", "");
        options.custom_help("[<command>] [OPTION...]");
        options.set_width(CONSOLE_USAGE_WIDTH); // Ensure CONSOLE_USAGE_WIDTH is defined
        options.add_options()("h,help", "Print this usage message and exit")(
            "v,version", "Print the version number of this program and exit")(
            "testrun",
            "Indicates test run. If enabled the tool will produce deterministic output whenever "
            "possible");
        options.allow_unrecognised_options();
        cxxopts::ParseResult args;
        try
        {
            args = options.parse(argc, argv);
        }
        catch (const std::exception &ex)
        {
            fmt::print(std::cerr, "{}: {}\n", (argc > 0 && argv[0] ? argv[0] : "ktx"), ex.what());
            printUsage(std::cerr, options);
            return +rc::INVALID_ARGUMENTS;
        }
        testrun = args["testrun"].as<bool>();
        if (args.count("help"))
        {
            fmt::print(std::cout,
                       "{}: Unified CLI frontend for the KTX-Software library with sub-commands for "
                       "specific operations.\n",
                       (argc > 0 && argv[0] ? argv[0] : "ktx"));
            printUsage(std::cout, options);
            return +rc::SUCCESS;
        }
        if (args.count("version"))
        {
            fmt::print("{} version: {}\n", (argc > 0 && argv[0] ? argv[0] : "ktx"), version(testrun));
            return +rc::SUCCESS;
        }
        if (args.unmatched().empty())
        {
            if (argc <= 1)
            { // Only program name
                fmt::print(std::cerr, "{}: Missing command.\n",
                           (argc > 0 && argv[0] ? argv[0] : "ktx"));
            }
            else
            { // Arguments were present but not matched as options or a known command pattern
                fmt::print(std::cerr, "{}: Unrecognized argument or missing command after: \"{}\"\n",
                           (argc > 0 && argv[0] ? argv[0] : "ktx"),
                           (argc > 1 && argv[1] ? argv[1] : ""));
            }
            printUsage(std::cerr, options);
        }
        else
        {
            fmt::print(std::cerr, "{}: Unrecognized argument: \"{}\"\n",
                       (argc > 0 && argv[0] ? argv[0] : "ktx"), args.unmatched()[0]);
            printUsage(std::cerr, options);
        }
        return +rc::INVALID_ARGUMENTS;
    }

    void Tools::printUsage(std::ostream &os, const cxxopts::Options &options)
    {
        fmt::print(os, "{}", options.help());
        fmt::print(os, "\n");
        fmt::print(os, "Available commands:\n");
        fmt::print(os, "  create     Create a KTX2 file from various input files\n");
        fmt::print(os, "  deflate    Deflate (supercompress) a KTX2 file\n");
        fmt::print(os, "  extract    Extract selected images from a KTX2 file\n");
        fmt::print(os, "  encode     Encode a KTX2 file\n");
        fmt::print(os, "  transcode  Transcode a KTX2 file\n");
        fmt::print(os, "  info       Print information about a KTX2 file\n");
        fmt::print(os, "  validate   Validate a KTX2 file\n");
        fmt::print(os, "  compare    Compare two KTX2 files\n");
        fmt::print(os, "  help       Display help information about the ktx tool\n");
#if KTX_DEVELOPER_FEATURE_PATCH
        fmt::print(os, "  patch      Apply certain patch operations to a KTX2 file.\n");
#endif
        fmt::print(os, "\n");
        fmt::print(os,
                   "For detailed usage and description of each subcommand use 'ktx help <command>'\n"
                   "or 'ktx <command> --help'\n");
    }
} // namespace ktx

// -------------------------------------------------------------------------------------------------
// KTX_COMMAND_BUILTIN declarations and builtinCommands map
// These must match your KTX tool's structure.
// They declare/define function pointers for each command.
// The actual command implementations (ktxCreate, ktxInfo etc.) must be linked into your DLL.
// -------------------------------------------------------------------------------------------------
KTX_COMMAND_BUILTIN(ktxCreate)
KTX_COMMAND_BUILTIN(ktxDeflate)
KTX_COMMAND_BUILTIN(ktxExtract)
KTX_COMMAND_BUILTIN(ktxEncode)
KTX_COMMAND_BUILTIN(ktxTranscode)
KTX_COMMAND_BUILTIN(ktxInfo)
KTX_COMMAND_BUILTIN(ktxValidate)
KTX_COMMAND_BUILTIN(ktxCompare)
KTX_COMMAND_BUILTIN(ktxHelp)
#if KTX_DEVELOPER_FEATURE_PATCH
KTX_COMMAND_BUILTIN(ktxPatch)
#endif

std::unordered_map<std::string, ktx::pfnBuiltinCommand> builtinCommands = {
    {"create", ktxCreate}, {"deflate", ktxDeflate}, {"extract", ktxExtract}, {"encode", ktxEncode}, {"transcode", ktxTranscode}, {"info", ktxInfo}, {"validate", ktxValidate}, {"compare", ktxCompare}, {"help", ktxHelp},
#if KTX_DEVELOPER_FEATURE_PATCH
    {"patch", ktxPatch}
#endif
};

// -------------------------------------------------------------------------------------------------
// Exported ktx_main function
// -------------------------------------------------------------------------------------------------
extern "C"
{

    KTX_API int ktx_main(int argc, char *argv[])
    {
        // print argv for argc
        // if is debug
        if (!false)
        {
            std::cout << "[KTX DLL Main] argc: " << argc << std::endl;
            for (int i = 0; i < argc; ++i)
            {
                std::cout << "[KTX DLL Main] argv[" << i << "]: " << (argv[i] ? argv[i] : "(null)")
                          << std::endl;
            }
        }

        if (argc >= 2)
        {
            std::string command_name = argv[1] ? argv[1] : "";
            const auto it = builtinCommands.find(command_name);
            if (it != builtinCommands.end())
            {
                return it->second(argc - 1, argv + 1);
            }
        }

        ktx::Tools cmd;
        return cmd.main(argc, argv);
    }

} // extern "C"

// Dummy version function - ensure your build links the actual version.cpp or similar
// if ktx::Tools::main or other parts depend on it.
std::string version(bool testrun)
{
    if (testrun)
        return "ktx_dll_test_version";
    return "ktx_dll_version_0.1.0"; // Placeholder
}

// Dummy CONSOLE_USAGE_WIDTH - ensure your build defines this if used by cxxopts
#ifndef CONSOLE_USAGE_WIDTH
#define CONSOLE_USAGE_WIDTH 80
#endif