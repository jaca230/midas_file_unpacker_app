#include "midas_file_unpacker_app/CLIOptions.h"
#include "midas_file_unpacker_app/UnpackerApp.h"
#include "midas_file_unpacker_app/profiles/ProfileRegistry.h"

#include <exception>
#include <iostream>

using namespace midas_file_unpacker_app;

int main(int argc, char** argv) {
    ProfileRegistry registry;

    CLIOptions options;
    try {
        options = parseCommandLine(argc, argv, registry);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        printUsage(argv[0], registry);
        return EXIT_FAILURE;
    }

    if (options.showHelp) {
        printUsage(argv[0], registry);
        return EXIT_SUCCESS;
    }

    try {
        UnpackerApp app(registry);
        return app.run(options);
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
