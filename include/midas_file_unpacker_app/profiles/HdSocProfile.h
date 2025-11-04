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
    PipelineDataProductReadLock time_lock_;

    dataProducts::NaluEvent* event_ptr_ = nullptr;
    dataProducts::NaluTime* time_ptr_ = nullptr;
};

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_PROFILES_HDSOCPROFILE_H
