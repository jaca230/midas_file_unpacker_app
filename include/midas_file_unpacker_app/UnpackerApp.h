#ifndef MIDAS_FILE_UNPACKER_APP_UNPACKERAPP_H
#define MIDAS_FILE_UNPACKER_APP_UNPACKERAPP_H

#include <cstddef>
#include <filesystem>

namespace midas_file_unpacker_app {

struct CLIOptions;
class ProfileRegistry;

class UnpackerApp {
public:
    explicit UnpackerApp(const ProfileRegistry& registry);
    int run(const CLIOptions& options) const;

private:
    const ProfileRegistry& registry_;
};

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_UNPACKERAPP_H
