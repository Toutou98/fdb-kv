/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/io/MultiHandle.h"
#include "eckit/types/Types.h"
#include "eckit/log/Plural.h"


#include "fdb5/HandleGatherer.h"

using namespace eckit;

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

HandleGatherer::HandleGatherer(bool sorted):
    sorted_(sorted) {
}

HandleGatherer::~HandleGatherer() {
    for (std::vector<DataHandle *>::iterator j = handles_.begin(); j != handles_.end(); ++j) {
        delete (*j);
    }
}

DataHandle *HandleGatherer::dataHandle() {
    for (std::vector<DataHandle *>::iterator j = handles_.begin(); j != handles_.end(); ++j) {
        (*j)->compress(sorted_);
    }

    DataHandle *h = new MultiHandle(handles_);
    handles_.clear();
    return h;
}

void HandleGatherer::add(DataHandle *h) {
    ASSERT(h);
    if (sorted_) {
        for (std::vector<DataHandle *>::iterator j = handles_.begin(); j != handles_.end(); ++j) {
            if ( (*j)->merge(h) ) {
                delete h;
                return;
            }
        }
    } else {
        if (handles_.size() > 0) {
            if ( handles_.back()->merge(h) ) {
                delete h;
                return;
            }
        }
    }
    handles_.push_back(h);
}

void HandleGatherer::print( std::ostream &out ) const {
    out << Plural(handles_.size(), "handle");
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5
