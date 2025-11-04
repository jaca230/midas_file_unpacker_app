#include "midas_file_unpacker_app/profiles/SampicProfile.h"

// NOTE: Implementation extracted from the previous monolithic Profiles.cpp
// to keep a single class per file for easier future maintenance.

#include <TTree.h>

#include "analysis_pipeline/core/data/pipeline_data_product_manager.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicCollectorTiming.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEvent.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEventTiming.h"

namespace midas_file_unpacker_app {

SampicProfile::SampicProfile()
    : primary_key_("sampic"),
      display_name_("SAMPIC"),
      config_relative_path_("config/unpacker_pipelines/SAMPIC/default_unpacking_pipeline.json") {}

std::string_view SampicProfile::primaryKey() const { return primary_key_; }
std::string_view SampicProfile::displayName() const { return display_name_; }
std::filesystem::path SampicProfile::configRelativePath() const { return config_relative_path_; }
PipelineMode SampicProfile::mode() const { return PipelineMode::Sampic; }

void SampicProfile::setupTree(TTree& tree) {
    tree.Branch("sampic_event", &event_ptr_);
    tree.Branch("sampic_event_timing", &event_timing_ptr_);
    tree.Branch("sampic_collector_timing", &collector_timing_ptr_);
    tree.Branch("has_sampic_collector_timing", &has_collector_flag_, "has_sampic_collector_timing/O");
}

bool SampicProfile::extractEvent(PipelineDataProductManager& dpm) {
    resetEventState();

    if (!dpm.hasProduct("SampicEvent")) {
        return false;
    }

    event_lock_ = dpm.checkoutRead("SampicEvent");
    if (!event_lock_.get()) {
        return false;
    }

    event_ptr_ = dynamic_cast<dataProducts::SampicEvent*>(event_lock_.get()->getObject());
    if (!event_ptr_) {
        return false;
    }

    if (dpm.hasProduct("SampicEventTiming")) {
        event_timing_lock_ = dpm.checkoutRead("SampicEventTiming");
        if (event_timing_lock_.get()) {
            event_timing_ptr_ = dynamic_cast<dataProducts::SampicEventTiming*>(event_timing_lock_.get()->getObject());
        }
    }

    if (dpm.hasProduct("SampicCollectorTiming")) {
        collector_lock_ = dpm.checkoutRead("SampicCollectorTiming");
        if (collector_lock_.get()) {
            collector_timing_ptr_ = dynamic_cast<dataProducts::SampicCollectorTiming*>(collector_lock_.get()->getObject());
        }
    }

    has_collector_flag_ = (collector_timing_ptr_ != nullptr);
    return true;
}

void SampicProfile::resetEventState() {
    event_lock_ = PipelineDataProductReadLock();
    event_timing_lock_ = PipelineDataProductReadLock();
    collector_lock_ = PipelineDataProductReadLock();

    event_ptr_ = nullptr;
    event_timing_ptr_ = nullptr;
    collector_timing_ptr_ = nullptr;
    has_collector_flag_ = false;
}

} // namespace midas_file_unpacker_app
