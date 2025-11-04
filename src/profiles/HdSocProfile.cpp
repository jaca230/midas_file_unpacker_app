#include "midas_file_unpacker_app/profiles/HdSocProfile.h"

// NOTE: Separate translation unit for HDSoC profile to keep class-per-file structure.

#include <TTree.h>

#include "analysis_pipeline/core/data/pipeline_data_product_manager.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluEvent.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluTime.h"

namespace midas_file_unpacker_app {

HdSocProfile::HdSocProfile()
    : primary_key_("hdsoc"),
      display_name_("HDSoC"),
      config_relative_path_("config/unpacker_pipelines/HDSoC/default_unpacking_pipeline.json") {}

std::string_view HdSocProfile::primaryKey() const { return primary_key_; }
std::string_view HdSocProfile::displayName() const { return display_name_; }
std::filesystem::path HdSocProfile::configRelativePath() const { return config_relative_path_; }
PipelineMode HdSocProfile::mode() const { return PipelineMode::HdSoc; }

void HdSocProfile::setupTree(TTree& tree) {
    tree.Branch("nalu_event", &event_ptr_);
    tree.Branch("nalu_time", &time_ptr_);
}

bool HdSocProfile::extractEvent(PipelineDataProductManager& dpm) {
    resetEventState();

    if (!dpm.hasProduct("NaluEvent")) {
        return false;
    }

    event_lock_ = dpm.checkoutRead("NaluEvent");
    if (!event_lock_.get()) {
        return false;
    }

    event_ptr_ = dynamic_cast<dataProducts::NaluEvent*>(event_lock_.get()->getObject());
    if (!event_ptr_) {
        return false;
    }

    if (dpm.hasProduct("NaluTime")) {
        time_lock_ = dpm.checkoutRead("NaluTime");
        if (time_lock_.get()) {
            time_ptr_ = dynamic_cast<dataProducts::NaluTime*>(time_lock_.get()->getObject());
        }
    }

    return true;
}

void HdSocProfile::resetEventState() {
    event_lock_ = PipelineDataProductReadLock();
    time_lock_ = PipelineDataProductReadLock();
    event_ptr_ = nullptr;
    time_ptr_ = nullptr;
}

} // namespace midas_file_unpacker_app
