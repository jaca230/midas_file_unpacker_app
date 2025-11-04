#include "midas_file_unpacker_app/UnpackerApp.h"

#include "midas_file_unpacker_app/CLIOptions.h"
#include "midas_file_unpacker_app/profiles/ProfileRegistry.h"

#include <TFile.h>
#include <TTree.h>

#include "analysis_pipeline/config/config_manager.h"
#include "analysis_pipeline/core/context/input_bundle.h"
#include "analysis_pipeline/pipeline/pipeline.h"

#include "midasio.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace midas_file_unpacker_app {

namespace {

constexpr std::size_t kDefaultMaxEvents = 10'000'000;

struct ReaderDeleter {
    void operator()(TMReaderInterface* reader) const {
        delete reader;
    }
};

using ReaderPtr = std::unique_ptr<TMReaderInterface, ReaderDeleter>;

std::filesystem::path resolveBaseDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}

} // namespace

UnpackerApp::UnpackerApp(const ProfileRegistry& registry)
    : registry_(registry) {}

int UnpackerApp::run(const CLIOptions& options) const {
    auto profile = registry_.getProfile(options.profileKey);
    const std::size_t max_events_requested = options.maxEvents.value_or(kDefaultMaxEvents);

    std::filesystem::path input_path(options.inputFile);
    if (!std::filesystem::exists(input_path)) {
        throw std::runtime_error("Input file does not exist: " + input_path.string());
    }

    std::filesystem::path base_dir = resolveBaseDir();
    std::filesystem::path pipeline_config_path = base_dir / profile->configRelativePath();
    if (!std::filesystem::exists(pipeline_config_path)) {
        throw std::runtime_error("Pipeline config file not found: " + pipeline_config_path.string());
    }

    std::vector<std::string> config_files = {
        (base_dir / "config/logger.json").string(),
        pipeline_config_path.string()
    };

    auto config_manager = std::make_shared<ConfigManager>();
    if (!config_manager->loadFiles(config_files) || !config_manager->validate()) {
        throw std::runtime_error("Failed to load or validate config files");
    }

    Pipeline pipeline(config_manager);
    if (!pipeline.buildFromConfig()) {
        throw std::runtime_error("Failed to build pipeline from config");
    }

    ReaderPtr count_reader(TMNewReader(input_path.string().c_str()));
    if (!count_reader) {
        throw std::runtime_error("Failed to open MIDAS file for counting: " + input_path.string());
    }

    std::size_t total_events_in_file = 0;
    while (TMReadEvent(count_reader.get()) != nullptr) {
        ++total_events_in_file;
    }
    count_reader.reset();

    const std::size_t total_events_to_process = std::min(max_events_requested, total_events_in_file);

    std::cout << "Using pipeline profile: " << profile->displayName()
              << " (" << pipeline_config_path.string() << ")\n";
    std::cout << "Input file: " << input_path.string() << "\n";
    std::cout << "Total events in file: " << total_events_in_file << "\n";
    std::cout << "Events to process: " << total_events_to_process << "\n";

    ReaderPtr reader(TMNewReader(input_path.string().c_str()));
    if (!reader) {
        throw std::runtime_error("Failed to reopen MIDAS file: " + input_path.string());
    }

    TFile output_file("output.root", "RECREATE");
    const std::string tree_title = std::string(profile->displayName()) + " unpacked events";
    TTree tree("events", tree_title.c_str());
    profile->setupTree(tree);

    std::size_t event_count = 0;
    const auto t_start = std::chrono::steady_clock::now();
    const double progress_update_percent = 5.0;
    std::size_t next_progress_event = 0;
    std::size_t progress_step = std::max<std::size_t>(1, static_cast<std::size_t>(
        total_events_to_process * progress_update_percent / 100.0));

    std::cout << "[Progress] 0.0% (0/" << total_events_to_process
              << ") | Time: 0.00 s | Rate: 0.00 events/s\n";

    while (event_count < total_events_to_process) {
        ++event_count;
        TMEvent* raw_event = TMReadEvent(reader.get());
        if (!raw_event) {
            break;
        }

        std::shared_ptr<TMEvent> wrapped_event(raw_event);

        InputBundle input;
        input.set("TMEvent", wrapped_event);
        pipeline.setInputData(std::move(input));
        pipeline.execute();

        auto& dpm = pipeline.getDataProductManager();

        if (!profile->extractEvent(dpm)) {
            profile->resetEventState();
            dpm.clear();
            continue;
        }

        tree.Fill();
        profile->resetEventState();
        dpm.clear();

        if (event_count >= next_progress_event || event_count == total_events_to_process) {
            const double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::steady_clock::now() - t_start).count();
            const double eps = (elapsed > 0.0) ? static_cast<double>(event_count) / elapsed : 0.0;
            const double percent = 100.0 * static_cast<double>(event_count) / total_events_to_process;
            const double remaining_time = (eps > 0.0)
                ? (total_events_to_process - event_count) / eps
                : 0.0;

            std::cout << std::fixed
                      << "[Progress] "
                      << std::setw(6) << std::setprecision(1) << percent << "% "
                      << "(" << std::setw(7) << event_count << "/" << total_events_to_process << ")"
                      << " | Time: " << std::setw(7) << std::setprecision(2) << elapsed << " s"
                      << " | Rate: " << std::setw(8) << std::setprecision(2) << eps << " events/s"
                      << " | ETA: " << std::setw(7) << std::setprecision(2) << remaining_time << " s\n";

            next_progress_event += progress_step;
        }
    }

    const auto t_end = std::chrono::steady_clock::now();
    const double duration_sec = std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start).count();
    const double rate = (event_count > 0)
        ? static_cast<double>(event_count) / std::max(duration_sec, 1e-9)
        : 0.0;

    output_file.cd();
    tree.Write();
    output_file.Close();
    reader.reset();

    std::cout << "\n----------------------------------------\n";
    std::cout << "           Processing Summary\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left << std::setw(25) << "Pipeline profile:" << profile->displayName() << "\n";
    std::cout << std::left << std::setw(25) << "Events processed:" << std::right << std::setw(10)
              << event_count << "\n";
    std::cout << std::left << std::setw(25) << "Elapsed time (s):" << std::right << std::setw(10)
              << std::fixed << std::setprecision(2) << duration_sec << "\n";
    std::cout << std::left << std::setw(25) << "Events per second:" << std::right << std::setw(10)
              << std::fixed << std::setprecision(2) << rate << "\n";
    std::cout << std::left << std::setw(25) << "Output written to:" << "output.root\n";
    std::cout << "----------------------------------------\n";

    return EXIT_SUCCESS;
}

} // namespace midas_file_unpacker_app
