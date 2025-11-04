#include "midas_file_unpacker_app/profiles/ProfileRegistry.h"

// NOTE: Registry implementation moved here to register profile instances in one place.

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "midas_file_unpacker_app/profiles/HdSocProfile.h"
#include "midas_file_unpacker_app/profiles/SampicProfile.h"

namespace midas_file_unpacker_app {

namespace {

std::string toLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

} // namespace

ProfileRegistry::ProfileRegistry() {
    auto sampic_profile = std::make_shared<SampicProfile>();
    registerProfile(sampic_profile, {"sampic", "sampic-daq"});

    auto hdsoc_profile = std::make_shared<HdSocProfile>();
    registerProfile(hdsoc_profile, {"hdsoc", "nalu"});

    default_key_ = "sampic";
}

void ProfileRegistry::registerProfile(std::shared_ptr<PipelineProfile> profile,
                                      std::initializer_list<const char*> aliases) {
    Entry entry;
    entry.profile = std::move(profile);
    entry.aliases.reserve(aliases.size());

    for (const char* alias_cstr : aliases) {
        std::string alias(alias_cstr);
        std::string normalized = toLowerCopy(alias);
        lookup_[normalized] = entry.profile;
        entry.aliases.push_back(std::move(alias));
    }

    entries_.push_back(std::move(entry));
}

std::shared_ptr<PipelineProfile> ProfileRegistry::getProfile(const std::string& key) const {
    std::string normalized = normalizeKey(key);
    auto it = lookup_.find(normalized);
    if (it == lookup_.end()) {
        std::ostringstream oss;
        oss << "Unknown profile '" << key << "'. Available profiles: ";
        auto summaries = profileSummaries();
        for (std::size_t i = 0; i < summaries.size(); ++i) {
            if (i > 0) {
                oss << "; ";
            }
            oss << summaries[i];
        }
        throw std::runtime_error(oss.str());
    }
    return it->second;
}

bool ProfileRegistry::hasProfile(const std::string& key) const {
    return lookup_.find(normalizeKey(key)) != lookup_.end();
}

std::string ProfileRegistry::normalizeKey(std::string key) const {
    return toLowerCopy(std::move(key));
}

std::vector<std::string> ProfileRegistry::profileSummaries() const {
    std::vector<std::string> summaries;
    summaries.reserve(entries_.size());
    for (const auto& entry : entries_) {
        std::ostringstream oss;
        oss << entry.profile->displayName() << " (keys: ";
        for (std::size_t i = 0; i < entry.aliases.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << entry.aliases[i];
        }
        oss << ")";
        summaries.push_back(oss.str());
    }
    return summaries;
}

std::vector<std::string> ProfileRegistry::profileDisplayNames() const {
    std::unordered_set<std::string> names;
    for (const auto& entry : entries_) {
        names.insert(std::string(entry.profile->displayName()));
    }

    std::vector<std::string> result(names.begin(), names.end());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace midas_file_unpacker_app
