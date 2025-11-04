#include "midas_file_unpacker_app/CLIOptions.h"

#include "midas_file_unpacker_app/profiles/ProfileRegistry.h"

#include <cctype>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace midas_file_unpacker_app {

namespace {

std::size_t parsePositiveSizeT(const std::string& value) {
    std::size_t pos = 0;
    unsigned long long parsed = 0;
    try {
        parsed = std::stoull(value, &pos);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid positive integer value: '" + value + "'");
    }

    if (pos != value.size() || parsed == 0) {
        throw std::runtime_error("Invalid positive integer value: '" + value + "'");
    }

    return static_cast<std::size_t>(parsed);
}

} // namespace

CLIOptions parseCommandLine(int argc, char** argv, const ProfileRegistry& registry) {
    CLIOptions options;
    options.profileKey = registry.defaultProfileKey();

    bool treat_as_positional = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (!treat_as_positional && arg == "--") {
            treat_as_positional = true;
            continue;
        }

        if (!treat_as_positional && (arg == "-h" || arg == "--help")) {
            options.showHelp = true;
            return options;
        }

        if (!treat_as_positional && arg == "--profile") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--profile requires a value");
            }
            options.profileKey = registry.normalizeKey(argv[++i]);
            continue;
        }

        if (!treat_as_positional && arg == "--max-events") {
            if (i + 1 >= argc) {
                throw std::runtime_error("--max-events requires a positive integer value");
            }
            options.maxEvents = parsePositiveSizeT(argv[++i]);
            continue;
        }

        if (!treat_as_positional && !arg.empty() && arg.front() == '-') {
            std::ostringstream oss;
            oss << "Unknown option '" << arg << "'";
            throw std::runtime_error(oss.str());
        }

        // Positional arguments
        if (options.inputFile.empty()) {
            options.inputFile = arg;
        } else if (!options.maxEvents.has_value()) {
            options.maxEvents = parsePositiveSizeT(arg);
        } else {
            std::ostringstream oss;
            oss << "Unexpected argument '" << arg << "'";
            throw std::runtime_error(oss.str());
        }
    }

    if (options.showHelp) {
        return options;
    }

    if (options.inputFile.empty()) {
        throw std::runtime_error("Missing required <input_midas_file> argument");
    }

    if (!registry.hasProfile(options.profileKey)) {
        std::ostringstream oss;
        oss << "Unknown profile '" << options.profileKey << "'";
        throw std::runtime_error(oss.str());
    }

    return options;
}

void printUsage(const char* program, const ProfileRegistry& registry) {
    std::cout << "Usage: " << program << " [OPTIONS] <input_midas_file> [max_events]\n\n"
              << "Options:\n"
              << "  --profile <name>     Select pipeline profile\n"
              << "  --max-events <N>     Limit number of events to process\n"
              << "  --help               Show this help message\n\n"
              << "Available profiles:\n";

    for (const auto& summary : registry.profileSummaries()) {
        std::cout << "  - " << summary << "\n";
    }

    std::cout << "\nExamples:\n"
              << "  " << program << " run00156.mid.lz4\n"
              << "  " << program << " --profile HDSoC run00156.mid.lz4 --max-events 5000\n";
}

} // namespace midas_file_unpacker_app
