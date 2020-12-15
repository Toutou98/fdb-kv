/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   Nov 2016

#include <algorithm>

#include "fdb5/LibFdb5.h"

#include "eckit/config/Resource.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/log/Log.h"

#include "fdb5/config/Config.h"
#include "fdb5/fdb5_version.h"

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

REGISTER_LIBRARY(LibFdb5);

LibFdb5::LibFdb5() : Library("fdb") {}

LibFdb5& LibFdb5::instance()
{
    static LibFdb5 libfdb;
    return libfdb;
}

Config LibFdb5::defaultConfig() {
    Config config;
    return config.expandConfig();
}

const void* LibFdb5::addr() const { return this; }

std::string LibFdb5::version() const { return fdb5_version_str(); }

unsigned int LibFdb5::latestSerialisationVersion() const {
    return 3;
}

static unsigned int getEnvFDB5SerialisationVersion() {
    if (::getenv("FDB5_SERIALISATION_VERSION")) {
        const char* versionstr = ::getenv("FDB5_SERIALISATION_VERSION");
        eckit::Log::debug() << "FDB5_SERIALISATION_VERSION overidde to version: " << versionstr << std::endl;
        unsigned int version = ::atoi(versionstr);
        return version;
    }
    return 0; // no version override
}

unsigned int LibFdb5::useSerialisationVersion() const {
    static unsigned int envVersion = getEnvFDB5SerialisationVersion();
    return (envVersion ? envVersion : latestSerialisationVersion());
}

std::vector<unsigned int> LibFdb5::supportedSerialisationVersions() const {
    std::vector<unsigned int> versions = {3, 2, 1};
    return versions;
}

std::string LibFdb5::gitsha1(unsigned int count) const {
    std::string sha1(fdb5_git_sha1());
    if(sha1.empty()) {
        return "not available";
    }

    return sha1.substr(0,std::min(count,40u));
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5

