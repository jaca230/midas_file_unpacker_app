#ifndef MIDAS_FILE_UNPACKER_APP_PROFILES_PIPELINEPROFILE_H
#define MIDAS_FILE_UNPACKER_APP_PROFILES_PIPELINEPROFILE_H

#include <cstddef>
#include <filesystem>
#include <string_view>

class TTree;
class PipelineDataProductManager;

namespace midas_file_unpacker_app {

enum class PipelineMode {
    Sampic,
    HdSoc
};

/// Base interface for pipeline-specific behaviour (split out per class for clarity).
class PipelineProfile {
public:
    virtual ~PipelineProfile() = default;

    virtual std::string_view primaryKey() const = 0;
    virtual std::string_view displayName() const = 0;
    virtual std::filesystem::path configRelativePath() const = 0;
    virtual PipelineMode mode() const = 0;

    virtual void setupTree(TTree& tree) = 0;
    virtual bool extractEvent(PipelineDataProductManager& dpm) = 0;
    virtual void resetEventState() = 0;
};

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_PROFILES_PIPELINEPROFILE_H
