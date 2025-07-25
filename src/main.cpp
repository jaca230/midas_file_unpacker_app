#include <iostream>
#include <filesystem>
#include <memory>
#include <string>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include <TFile.h>
#include <TTree.h>

#include "midasio.h"

#include "analysis_pipeline/config/config_manager.h"
#include "analysis_pipeline/pipeline/pipeline.h"

#include "analysis_pipeline/unpacker_nalu/data_products/NaluEvent.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluTime.h"

int main(int argc, char** argv) {
    // ----------------------------------
    // Parse input arguments
    // ----------------------------------
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " input_midas_file [max_events]\n";
        return EXIT_FAILURE;
    }

    const std::string input_file = argv[1];
    const int max_events_arg = (argc == 3) ? std::stoi(argv[2]) : 10'000'000;
    if (max_events_arg <= 0) {
        std::cerr << "max_events must be a positive integer\n";
        return EXIT_FAILURE;
    }
    const size_t max_events = static_cast<size_t>(max_events_arg);

    if (!std::filesystem::exists(input_file)) {
        std::cerr << "Input file does not exist: " << input_file << "\n";
        return EXIT_FAILURE;
    }

    // ----------------------------------
    // Load pipeline configuration files
    // ----------------------------------
    std::filesystem::path base_dir = std::filesystem::path(__FILE__).parent_path().parent_path();
    std::vector<std::string> config_files = {
        (base_dir / "config/logger.json").string(),
        (base_dir / "config/unpacker_pipelines/HDSoC/default_unpacking_pipeline.json").string()
    };

    auto config_manager = std::make_shared<ConfigManager>();
    if (!config_manager->loadFiles(config_files) || !config_manager->validate()) {
        std::cerr << "Failed to load or validate config files\n";
        return EXIT_FAILURE;
    }

    // ----------------------------------
    // Construct pipeline from configuration
    // ----------------------------------
    Pipeline pipeline(config_manager);
    if (!pipeline.buildFromConfig()) {
        std::cerr << "Failed to build pipeline\n";
        return EXIT_FAILURE;
    }

    // ----------------------------------
    // Count total events in file
    // ----------------------------------
    TMReaderInterface* count_reader = TMNewReader(input_file.c_str());
    if (!count_reader) {
        std::cerr << "Failed to open MIDAS file for counting: " << input_file << "\n";
        return EXIT_FAILURE;
    }

    size_t total_events_in_file = 0;
    while (TMReadEvent(count_reader) != nullptr) {
        ++total_events_in_file;
    }
    delete count_reader;

    // Determine total events to process
    const size_t total_events_to_process = std::min(max_events, total_events_in_file);

    std::cout << "Total events in file: " << total_events_in_file << "\n";
    std::cout << "Events to process: " << total_events_to_process << "\n";

    // ----------------------------------
    // Setup progress update parameters
    // ----------------------------------
    const double progress_update_percent = 5.0; // update every 5%
    size_t next_progress_event = 0; // start at 0 for immediate feedback
    size_t progress_step = static_cast<size_t>(total_events_to_process * (progress_update_percent / 100.0));
    if (progress_step == 0) progress_step = 1; // Ensure at least 1

    // ----------------------------------
    // Reopen reader for actual processing
    // ----------------------------------
    TMReaderInterface* reader = TMNewReader(input_file.c_str());
    if (!reader) {
        std::cerr << "Failed to reopen MIDAS file: " << input_file << "\n";
        return EXIT_FAILURE;
    }

    // ----------------------------------
    // Setup ROOT file and TTree
    // ----------------------------------
    TFile output_file("output.root", "RECREATE");
    TTree tree("events", "Nalu unpacked events");

    dataProducts::NaluEvent* event_ptr = nullptr;
    dataProducts::NaluTime* time_ptr = nullptr;

    tree.Branch("event", &event_ptr);
    tree.Branch("times", &time_ptr);

    // ----------------------------------
    // Event loop with periodic progress updates
    // ----------------------------------
    size_t event_count = 0;
    const auto t_start = std::chrono::steady_clock::now();

    // Print initial progress
    std::cout << "[Progress] 0.0% (0/" << total_events_to_process << ") | Time: 0.00 s | Rate: 0.00 events/s\n";

    while (event_count < total_events_to_process) {
        ++event_count;
        TMEvent* raw_event = TMReadEvent(reader);
        if (!raw_event) break;

        std::shared_ptr<TMEvent> wrapped_event(raw_event);

        InputBundle input;
        input.set("TMEvent", wrapped_event);
        pipeline.setInputData(std::move(input));
        pipeline.execute();

        auto& dpm = pipeline.getDataProductManager();

        if (!dpm.hasProduct("NaluEvent") || !dpm.hasProduct("NaluTime")) {
            std::cerr << "Skipping event " << event_count << ": missing required products\n";
            dpm.clear();
            continue;
        }

        auto event_lock = dpm.checkoutRead("NaluEvent");
        auto time_lock  = dpm.checkoutRead("NaluTime");

        event_ptr = dynamic_cast<dataProducts::NaluEvent*>(event_lock.get()->getObject());
        time_ptr  = dynamic_cast<dataProducts::NaluTime*>(time_lock.get()->getObject());

        if (!event_ptr || !time_ptr) {
            std::cerr << "Skipping event " << event_count << ": failed to cast product types\n";
            dpm.clear();
            continue;
        }

        tree.Fill();

        if (event_count >= next_progress_event || event_count == total_events_to_process) {
            double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::steady_clock::now() - t_start
            ).count();

            double eps = (elapsed > 0) ? static_cast<double>(event_count) / elapsed : 0.0;
            double percent = 100.0 * event_count / total_events_to_process;
            double remaining_time = (eps > 0.0) ? (total_events_to_process - event_count) / eps : 0.0;

            std::cout << std::fixed
                    << "[Progress] "
                    << std::setw(6) << std::setprecision(1) << percent << "% "
                    << "(" << std::setw(7) << event_count << "/" << total_events_to_process << ")"
                    << " | Time: " << std::setw(7) << std::setprecision(2) << elapsed << " s"
                    << " | Rate: " << std::setw(8) << std::setprecision(2) << eps << " events/s"
                    << " | ETA: " << std::setw(7) << std::setprecision(2) << remaining_time << " s\n";

            next_progress_event += progress_step;
        }

        dpm.clear();
    }

    // ----------------------------------
    // Timing and output finalization
    // ----------------------------------
    const auto t_end = std::chrono::steady_clock::now();
    const double duration_sec = std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start).count();
    const double rate = (event_count > 0) ? static_cast<double>(event_count) / duration_sec : 0.0;

    output_file.cd();
    tree.Write();
    output_file.Close();
    delete reader;

    std::cout << "\n----------------------------------------\n";
    std::cout << "           Processing Summary\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left << std::setw(25) << "Events processed:"      << std::right << std::setw(10) << event_count     << "\n";
    std::cout << std::left << std::setw(25) << "Elapsed time (s):"     << std::right << std::setw(10) << std::fixed << std::setprecision(2) << duration_sec << "\n";
    std::cout << std::left << std::setw(25) << "Events per second:"    << std::right << std::setw(10) << std::fixed << std::setprecision(2) << rate         << "\n";
    std::cout << std::left << std::setw(25) << "Output written to:"    << std::right << " " << "output.root" << "\n";
    std::cout << "----------------------------------------\n";


    return EXIT_SUCCESS;
}
