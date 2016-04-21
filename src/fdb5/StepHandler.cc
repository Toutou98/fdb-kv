/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/exception/Exceptions.h"
#include "eckit/utils/Translator.h"

#include "marslib/MarsTask.h"
#include "marslib/StepRange.h"

#include "fdb5/KeywordType.h"
#include "fdb5/StepHandler.h"
#include "fdb5/DB.h"

using namespace eckit;

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

StepHandler::StepHandler(const std::string &name) :
    KeywordHandler(name) {
}

StepHandler::~StepHandler() {
}


void StepHandler::toKey(std::ostream& out,
                       const std::string& keyword,
                       const std::string& value) const {
  out << StepRange(value);
}

void StepHandler::getValues(const MarsRequest &request,
                            const std::string &keyword,
                            StringList &values,
                            const MarsTask &task,
                            const DB *db) const {
    std::vector<std::string> steps;

    request.getValues(keyword, steps);

    Translator<StepRange, std::string> t;

    values.reserve(steps.size());

    if(db) {
        StringSet axis;
        db->axis(keyword, axis);
        for (std::vector<std::string>::const_iterator i = steps.begin(); i != steps.end(); ++i) {
            std::string s(t(StepRange(*i)));
            if(axis.find(s) == axis.end()) {
                std::string z = "0-" + s;
                if(axis.find(z) != axis.end()) {
                    values.push_back(z);
                }
                else {
                    values.push_back(s);
                }
            }
            else {
                values.push_back(s);
            }
        }
    }
    else {
        for (std::vector<std::string>::const_iterator i = steps.begin(); i != steps.end(); ++i) {
            values.push_back(t(StepRange(*i)));
        }
    }
}

void StepHandler::print(std::ostream &out) const {
    out << "StepHandler(" << name_ << ")";
}

static KeywordHandlerBuilder<StepHandler> handler("Step");

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5
