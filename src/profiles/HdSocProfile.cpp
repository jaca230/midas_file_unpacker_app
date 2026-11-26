#include "midas_file_unpacker_app/profiles/HdSocProfile.h"

// NOTE: Separate translation unit for HDSoC profile to keep class-per-file structure.

#include <TTree.h>
#include "analysis_pipeline/unpacker_nalu/data_products/NaluBuilderEvent.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluCollectionTiming.h"
#include "analysis_pipeline/unpacker_nalu/data_products/NaluSelectionSummary.h"


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
    tree.Branch("has_nalu_event", &has_event_, "has_nalu_event/O");
    tree.Branch("nalu_time", &time_ptr_);
    tree.Branch("has_nalu_time", &has_time_, "has_nalu_time/O");
    tree.Branch("nalu_builder_event", &builder_event_ptr_);
    tree.Branch("has_nalu_builder_event", &has_builder_event_, "has_nalu_builder_event/O");
    tree.Branch("nalu_collection_timing", &collection_timing_ptr_);
    tree.Branch("has_nalu_collection_timing", &has_collection_timing_, "has_nalu_collection_timing/O");
    tree.Branch("nalu_selection_summary", &selection_summary_ptr_);
    tree.Branch("has_nalu_selection_summary", &has_selection_summary_, "has_nalu_selection_summary/O");
}

bool HdSocProfile::extractEvent(PipelineDataProductManager& dpm) {
    resetEventState();
    if (dpm.hasProduct("NaluEvent")) {
        event_lock_ = dpm.checkoutRead("NaluEvent");
        if (event_lock_.get())
            event_ptr_ = dynamic_cast<dataProducts::NaluEvent*>(event_lock_.get()->getObject());
    }
    has_event_ = (event_ptr_ != nullptr);
    if (dpm.hasProduct("NaluTime")) {
        time_lock_ = dpm.checkoutRead("NaluTime");
        if (time_lock_.get())
            time_ptr_ = dynamic_cast<dataProducts::NaluTime*>(time_lock_.get()->getObject());
    }
    has_time_ = (time_ptr_ != nullptr);
    if (dpm.hasProduct("NaluBuilderEvent")) {
        builder_event_lock_ = dpm.checkoutRead("NaluBuilderEvent");
        if (builder_event_lock_.get())
            builder_event_ptr_ = dynamic_cast<dataProducts::NaluBuilderEvent*>(builder_event_lock_.get()->getObject());
    }
    has_builder_event_ = (builder_event_ptr_ != nullptr);
    if (dpm.hasProduct("NaluCollectionTiming")) {
        collection_timing_lock_ = dpm.checkoutRead("NaluCollectionTiming");
        if (collection_timing_lock_.get())
            collection_timing_ptr_ = dynamic_cast<dataProducts::NaluCollectionTiming*>(collection_timing_lock_.get()->getObject());
    }
    has_collection_timing_ = (collection_timing_ptr_ != nullptr);
    if (dpm.hasProduct("NaluSelectionSummary")) {
        selection_summary_lock_ = dpm.checkoutRead("NaluSelectionSummary");
        if (selection_summary_lock_.get())
            selection_summary_ptr_ = dynamic_cast<dataProducts::NaluSelectionSummary*>(selection_summary_lock_.get()->getObject());
    }
    has_selection_summary_ = (selection_summary_ptr_ != nullptr);
    return has_event_ || has_time_ || has_builder_event_ || has_collection_timing_ || has_selection_summary_;
}

void HdSocProfile::resetEventState() {
    event_lock_ = PipelineDataProductReadLock();
    event_ptr_ = nullptr;
    has_event_ = false;
    time_lock_ = PipelineDataProductReadLock();
    time_ptr_ = nullptr;
    has_time_ = false;
    builder_event_lock_ = PipelineDataProductReadLock();
    builder_event_ptr_ = nullptr;
    has_builder_event_ = false;
    collection_timing_lock_ = PipelineDataProductReadLock();
    collection_timing_ptr_ = nullptr;
    has_collection_timing_ = false;
    selection_summary_lock_ = PipelineDataProductReadLock();
    selection_summary_ptr_ = nullptr;
    has_selection_summary_ = false;
}

} // namespace midas_file_unpacker_app
