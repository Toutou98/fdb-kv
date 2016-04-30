/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/types/Metadata.h"


#include "fdb5/legacy/LegacyArchiver.h"
#include "fdb5/database/ArchiveVisitor.h"

namespace fdb5 {
namespace legacy {

//----------------------------------------------------------------------------------------------------------------------

LegacyArchiver::LegacyArchiver() :
    Archiver(),
    translator_(),
    legacy_() {
}

void LegacyArchiver::archive(const eckit::DataBlobPtr blob) {
    const eckit::Metadata &metadata = blob->metadata();

    Key key;

    eckit::StringList keywords = metadata.keywords();

    std::string value;
    for (eckit::StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i) {
        metadata.get(*i, value);
        key.set(*i, value);
    }

    std::cout << "Metadata keys " << key << std::endl;
    std::cout << "Legacy keys " << legacy_ << std::endl;

    // compare legacy and metadata

    eckit::StringSet missing;
    eckit::StringSet mismatch;

    for (Key::const_iterator j = key.begin(); j != key.end(); ++j) {

        Key::const_iterator itr = legacy_.find((*j).first);
        if (itr == legacy_.end()) {
            missing.insert((*j).first);
        } else {
            if (j->second != itr->second) {
                std::ostringstream oss;
                oss << j->first << "=" << j->second << " and " << itr->second;
                mismatch.insert(oss.str());
            }
        }
    }

    if (missing.size() || mismatch.size()) {

        std::ostringstream oss;
        oss << "FDB LegacyArchiver missing keys: " << missing;
        oss << " mismatch keys: " << mismatch;

        throw eckit::SeriousBug(oss.str());
    }

    // archive

    ArchiveVisitor visitor(*this, key, blob->buffer(), blob->length());

    this->Archiver::archive(key, visitor);
}

void LegacyArchiver::legacy(const std::string &keyword, const std::string &value) {
    translator_.set(legacy_, keyword, value);
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace legacy
} // namespace fdb5
