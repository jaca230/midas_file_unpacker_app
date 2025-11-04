#ifndef MIDAS_FILE_UNPACKER_APP_CLIOPTIONS_H
#define MIDAS_FILE_UNPACKER_APP_CLIOPTIONS_H

#include <cstddef>
#include <optional>
#include <string>

namespace midas_file_unpacker_app {

class ProfileRegistry;

struct CLIOptions {
    std::string inputFile;
    std::optional<std::size_t> maxEvents;
    std::string profileKey;
    bool showHelp = false;
};

CLIOptions parseCommandLine(int argc, char** argv, const ProfileRegistry& registry);
void printUsage(const char* program, const ProfileRegistry& registry);

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_CLIOPTIONS_H
