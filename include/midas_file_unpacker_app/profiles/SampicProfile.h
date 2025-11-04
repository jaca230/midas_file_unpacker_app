#ifndef MIDAS_FILE_UNPACKER_APP_PROFILES_SAMPICPROFILE_H
#define MIDAS_FILE_UNPACKER_APP_PROFILES_SAMPICPROFILE_H

#include "midas_file_unpacker_app/profiles/PipelineProfile.h"

#include "analysis_pipeline/core/data/pipeline_data_product_read_lock.h"

#include <filesystem>
#include <memory>
#include <string>

class PipelineDataProductManager;

namespace dataProducts {
class SampicCollectorTiming;
class SampicEvent;
class SampicEventTiming;
}

namespace midas_file_unpacker_app {

/// Profile implementation for SAMPIC data products (refactored into its own file).
class SampicProfile final : public PipelineProfile {
public:
    SampicProfile();

    std::string_view primaryKey() const override;
    std::string_view displayName() const override;
    std::filesystem::path configRelativePath() const override;
    PipelineMode mode() const override;

    void setupTree(TTree& tree) override;
    bool extractEvent(PipelineDataProductManager& dpm) override;
    void resetEventState() override;

private:
    std::string primary_key_;
    std::string display_name_;
    std::filesystem::path config_relative_path_;

    PipelineDataProductReadLock event_lock_;
    PipelineDataProductReadLock event_timing_lock_;
    PipelineDataProductReadLock collector_lock_;

    dataProducts::SampicEvent* event_ptr_ = nullptr;
    dataProducts::SampicEventTiming* event_timing_ptr_ = nullptr;
    dataProducts::SampicCollectorTiming* collector_timing_ptr_ = nullptr;
    bool has_collector_flag_ = false;
};

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_PROFILES_SAMPICPROFILE_H
