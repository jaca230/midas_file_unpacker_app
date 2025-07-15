#include <iostream>
#include <filesystem>
#include <memory>
#include <ctime>
#include "config/config_manager.h"
#include "pipeline/pipeline.h"
#include "midasio.h"
#include <nlohmann/json.hpp>

// Create a dummy MIDAS event
TMEvent makeDummyEvent(int event_id, int serial, const std::string& payload) {
    TMEvent evt;
    evt.Init(event_id, serial, 0xABCD, static_cast<uint32_t>(std::time(nullptr)));
    evt.AddBank("DUMY", TID_STRING, payload.c_str(), payload.size() + 1);
    evt.FindAllBanks();
    return evt;
}

int main(int argc, char** argv) {
    // Locate config directory
    std::filesystem::path sourcePath = __FILE__;
    std::filesystem::path configDir = sourcePath.parent_path().parent_path() / "config";

    std::vector<std::string> configPaths = {
        (configDir / "logger.json").string(),
        (configDir / "main_pipeline.json").string()
    };

    auto configManager = std::make_shared<ConfigManager>();
    if (!configManager->loadFiles(configPaths) || !configManager->validate()) {
        std::cerr << "Error: Failed to load or validate configuration." << std::endl;
        return 1;
    }

    Pipeline pipeline(configManager);
    if (!pipeline.buildFromConfig()) {
        std::cerr << "Error: Failed to build pipeline." << std::endl;
        return 1;
    }

    // Make a dummy event and feed it to the pipeline
    TMEvent dummy = makeDummyEvent(42, 999, "HELLO_FROM_TEST");
    InputBundle input;
    input.setRef("TMEvent", dummy);  // Pass reference by key into bundle
    pipeline.setInputData(std::move(input));
    pipeline.execute();

    // Serialize output
    nlohmann::json output = pipeline.getDataProductManager().serializeAll();
    nlohmann::json outJson;
    outJson["serialized_pipeline_products"] = output;

    std::cout << "\n[Pretty JSON Output from Dummy Event]\n";
    std::cout << outJson.dump(4) << std::endl;

    return 0;
}
