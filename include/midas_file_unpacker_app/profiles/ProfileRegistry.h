#ifndef MIDAS_FILE_UNPACKER_APP_PROFILES_PROFILEREGISTRY_H
#define MIDAS_FILE_UNPACKER_APP_PROFILES_PROFILEREGISTRY_H

#include "midas_file_unpacker_app/profiles/PipelineProfile.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace midas_file_unpacker_app {

/// Keeps track of available profiles and their aliases.
class ProfileRegistry {
public:
    ProfileRegistry();

    std::shared_ptr<PipelineProfile> getProfile(const std::string& key) const;
    bool hasProfile(const std::string& key) const;
    std::string normalizeKey(std::string key) const;

    std::vector<std::string> profileSummaries() const;
    std::vector<std::string> profileDisplayNames() const;
    std::string defaultProfileKey() const { return default_key_; }

private:
    struct Entry {
        std::shared_ptr<PipelineProfile> profile;
        std::vector<std::string> aliases;
    };

    void registerProfile(std::shared_ptr<PipelineProfile> profile,
                         std::initializer_list<const char*> aliases);

    std::string default_key_;
    std::vector<Entry> entries_;
    std::unordered_map<std::string, std::shared_ptr<PipelineProfile>> lookup_;
};

} // namespace midas_file_unpacker_app

#endif // MIDAS_FILE_UNPACKER_APP_PROFILES_PROFILEREGISTRY_H
