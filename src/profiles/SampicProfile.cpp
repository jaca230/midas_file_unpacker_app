#include "midas_file_unpacker_app/profiles/SampicProfile.h"

// NOTE: Implementation extracted from the previous monolithic Profiles.cpp
// to keep a single class per file for easier future maintenance.

#include <TTree.h>

#include "analysis_pipeline/core/data/pipeline_data_product_manager.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicCollectorTiming.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEvent.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEventTiming.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicTriggerMetadata.h"

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
    tree.Branch("has_sampic_event", &has_event_flag_,
                "has_sampic_event/O");
    tree.Branch("sampic_event_timing", &event_timing_ptr_);
    tree.Branch("has_sampic_event_timing", &has_event_timing_flag_,
                "has_sampic_event_timing/O");
    tree.Branch("sampic_collector_timing", &collector_timing_ptr_);
    tree.Branch("has_sampic_collector_timing", &has_collector_flag_, "has_sampic_collector_timing/O");
    tree.Branch("sampic_trigger_metadata", &trigger_metadata_ptr_);
    tree.Branch("has_sampic_trigger_metadata", &has_trigger_metadata_flag_,
                "has_sampic_trigger_metadata/O");
}

bool SampicProfile::extractEvent(PipelineDataProductManager& dpm) {
    resetEventState();

    if (dpm.hasProduct("SampicEvent")) {
        event_lock_ = dpm.checkoutRead("SampicEvent");
        if (event_lock_.get()) {
            event_ptr_ = dynamic_cast<dataProducts::SampicEvent*>(
                event_lock_.get()->getObject());
        }
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

    if (dpm.hasProduct("SampicTriggerMetadata")) {
        trigger_metadata_lock_ = dpm.checkoutRead("SampicTriggerMetadata");
        if (trigger_metadata_lock_.get()) {
            trigger_metadata_ptr_ =
                dynamic_cast<dataProducts::SampicTriggerMetadata*>(
                    trigger_metadata_lock_.get()->getObject());
        }
    }

    has_event_flag_ = (event_ptr_ != nullptr);
    has_event_timing_flag_ = (event_timing_ptr_ != nullptr);
    has_collector_flag_ = (collector_timing_ptr_ != nullptr);
    has_trigger_metadata_flag_ = (trigger_metadata_ptr_ != nullptr);
    return has_event_flag_ || has_event_timing_flag_ ||
           has_collector_flag_ || has_trigger_metadata_flag_;
}

void SampicProfile::resetEventState() {
    event_lock_ = PipelineDataProductReadLock();
    event_timing_lock_ = PipelineDataProductReadLock();
    collector_lock_ = PipelineDataProductReadLock();
    trigger_metadata_lock_ = PipelineDataProductReadLock();

    event_ptr_ = nullptr;
    event_timing_ptr_ = nullptr;
    collector_timing_ptr_ = nullptr;
    trigger_metadata_ptr_ = nullptr;
    has_event_flag_ = false;
    has_event_timing_flag_ = false;
    has_collector_flag_ = false;
    has_trigger_metadata_flag_ = false;
}

} // namespace midas_file_unpacker_app
