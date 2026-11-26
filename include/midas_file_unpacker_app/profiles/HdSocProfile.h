#ifndef MIDAS_FILE_UNPACKER_APP_PROFILES_HDSOCPROFILE_H
#define MIDAS_FILE_UNPACKER_APP_PROFILES_HDSOCPROFILE_H

#include "midas_file_unpacker_app/profiles/PipelineProfile.h"

#include "analysis_pipeline/core/data/pipeline_data_product_read_lock.h"

#include <filesystem>
#include <memory>
#include <string>

class PipelineDataProductManager;

namespace dataProducts {
class NaluEvent;
class NaluTime;
class NaluBuilderEvent;
class NaluCollectionTiming;
class NaluSelectionSummary;
}

namespace midas_file_unpacker_app {

/// Profile implementation for HDSoC/Nalu data products (now isolated to its own file).
class HdSocProfile final : public PipelineProfile {
public:
    HdSocProfile();

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
    dataProducts::NaluEvent* event_ptr_ = nullptr;
    bool has_event_ = false;
    PipelineDataProductReadLock time_lock_;
    dataProducts::NaluTime* time_ptr_ = nullptr;
    bool has_time_ = false;
    PipelineDataProductReadLock builder_event_lock_;
    dataProducts::NaluBuilderEvent* builder_event_ptr_ = nullptr;
    bool has_builder_event_ = false;
    PipelineDataProductReadLock collection_timing_lock_;
    dataProducts::NaluCollectionTiming* collection_timing_ptr_ = nullptr;
    bool has_collection_timing_ = false;
    PipelineDataProductReadLock selection_summary_lock_;
    dataProducts::NaluSelectionSummary* selection_summary_ptr_ = nullptr;
    bool has_selection_summary_ = false;

};

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_PROFILES_HDSOCPROFILE_H
