# cmake/CPMConfig.cmake

# ---------------------- CPM Package Registry ----------------------
# NOTE: Order matters! Dependencies should be declared in dependency order.
# NOTE: The name in package list matters! Dependencies use "Find{name}.cmake",
# so the name has to match the normal package export name for that package



set(CPM_PACKAGE_LIST
  spdlog
  nlohmann_json
  TBB
  AnalysisPipelineStagesCore
  AnalysisPipeline
  midas_event_unpacker_plugin
)


# ---------------------- spdlog ----------------------
set(spdlog_REPO   "gabime/spdlog")
set(spdlog_TAG    "v1.13.0")
set(spdlog_TARGET "")
set(spdlog_DOWNLOAD_ONLY YES)
set(spdlog_OPTIONS
  "SPDLOG_BUILD_TESTS OFF"
  "SPDLOG_BUILD_EXAMPLE OFF"
)

# ---------------------- nlohmann_json ----------------------
set(nlohmann_json_REPO   "nlohmann/json")
set(nlohmann_json_TAG    "v3.11.3")
set(nlohmann_json_TARGET "")
set(nlohmann_json_DOWNLOAD_ONLY YES)
set(nlohmann_json_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- oneTBB ----------------------
set(TBB_REPO   "oneapi-src/oneTBB")
set(TBB_TAG    "v2021.12.0")  # Or latest stable
set(TBB_TARGET "") # Public Dependecy of Analysis Pipeline, no need to link
set(TBB_OPTIONS
  "TBB_TEST OFF;TBB_STRICT OFF;TBB_EXAMPLES OFF;CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- analysis_pipeline_stages ----------------------
set(AnalysisPipelineStagesCore_REPO   "jaca230/analysis_pipeline_stages_core")
set(AnalysisPipelineStagesCore_TAG    "main")
set(AnalysisPipelineStagesCore_TARGET "AnalysisPipelineStagesCore::analysis_pipeline_stages_core")
set(AnalysisPipelineStagesCore_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

# ---------------------- analysis_pipeline ----------------------
set(AnalysisPipeline_REPO   "jaca230/analysis_pipeline")
set(AnalysisPipeline_TAG    "main")
set(AnalysisPipeline_TARGET "AnalysisPipeline::analysis_pipeline")
set(AnalysisPipeline_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
  "BUILD_EXAMPLE_PLUGIN OFF"
)

# ---------------------- midas_event_unpacker_plugin ----------------------
set(midas_event_unpacker_plugin_REPO   "jaca230/midas_event_unpacker_plugin")
set(midas_event_unpacker_plugin_TAG    "main")
set(midas_event_unpacker_plugin_TARGET "midas_event_unpacker_plugin::midas_event_unpacker_plugin")
set(midas_event_unpacker_plugin_OPTIONS
  "CMAKE_POSITION_INDEPENDENT_CODE ON"
  "USE_BUNDLED_MIDAS ON"
)