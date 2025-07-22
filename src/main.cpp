#include <iostream>
#include <filesystem>
#include <memory>
#include <string>
#include <cstdlib>
#include "analysis_pipeline/config/config_manager.h"
#include "analysis_pipeline/pipeline/pipeline.h"
#include "midasio.h"
#include <nlohmann/json.hpp>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input_midas_file max_events\n";
        return EXIT_FAILURE;
    }

    std::string input_file = argv[1];
    int max_events = std::atoi(argv[2]);
    if (max_events <= 0) {
        std::cerr << "max_events must be a positive integer\n";
        return EXIT_FAILURE;
    }

    if (!std::filesystem::exists(input_file)) {
        std::cerr << "Input file does not exist: " << input_file << "\n";
        return EXIT_FAILURE;
    }

    // Setup config directory and load configs
    std::filesystem::path base_dir = std::filesystem::path(__FILE__).parent_path().parent_path();
    std::filesystem::path config_dir = base_dir / "config";

    std::vector<std::string> config_files = {
        (config_dir / "logger.json").string(),
        (config_dir / "main_pipeline.json").string()
    };

    auto config_manager = std::make_shared<ConfigManager>();
    if (!config_manager->loadFiles(config_files) || !config_manager->validate()) {
        std::cerr << "Failed to load or validate config files\n";
        return EXIT_FAILURE;
    }

    Pipeline pipeline(config_manager);
    if (!pipeline.buildFromConfig()) {
        std::cerr << "Failed to build pipeline\n";
        return EXIT_FAILURE;
    }

    // Open MIDAS file
    TMReaderInterface* reader = TMNewReader(input_file.c_str());
    if (!reader) {
        std::cerr << "Failed to open MIDAS file: " << input_file << "\n";
        return EXIT_FAILURE;
    }

    int event_count = 0;
    while (event_count < max_events) {
        TMEvent* event = TMReadEvent(reader);
        if (!event) break; // EOF or error

        // Wrap raw pointer in shared_ptr with default deleter
        std::shared_ptr<TMEvent> event_ptr(event);

        InputBundle input;
        input.set("TMEvent", event_ptr);

        pipeline.setInputData(std::move(input));
        pipeline.execute();

        nlohmann::json output = pipeline.getDataProductManager().serializeAll();
        std::cout << "Event #" << event_count+1 << " serialized output:\n";
        std::cout << output.dump(4) << "\n\n";

        ++event_count;
    }

    delete reader; // Clean up MIDAS reader

    std::cout << "Processed " << event_count << " events.\n";

    return EXIT_SUCCESS;
}
