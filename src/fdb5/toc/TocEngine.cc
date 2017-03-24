/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <ostream>

#include "eckit/log/Log.h"
#include "eckit/utils/Regex.h"

#include "fdb5/LibFdb.h"
#include "fdb5/config/MasterConfig.h"
#include "fdb5/toc/TocEngine.h"
#include "fdb5/toc/RootManager.h"
#include "fdb5/toc/TocHandler.h"

using eckit::Regex;
using eckit::Log;

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

std::string TocEngine::name() const {
    return TocEngine::typeName();
}

std::string TocEngine::dbType() const {
    return TocEngine::typeName();
}

eckit::PathName TocEngine::location(const Key& key) const
{
    return RootManager::directory(key);
}

bool TocEngine::canHandle(const eckit::PathName& path) const
{
    eckit::PathName toc = path / "toc";
    return path.isDir() && toc.exists();
}

static void matchKeyToDB(const Key& key, std::set<Key>& keys)
{
    const Schema& schema = MasterConfig::instance().schema();
    schema.matchFirstLevel(key, keys);
}

std::vector<eckit::PathName> TocEngine::databases(const Key &key, const std::vector<eckit::PathName>& dirs) {

    std::set<Key> keys;

    matchKeyToDB(key,keys);

    Log::debug<LibFdb>() << "Matched DB keys " << keys << std::endl;

    std::vector<eckit::PathName> result;
    std::set<eckit::PathName> seen;

    for (std::vector<eckit::PathName>::const_iterator j = dirs.begin(); j != dirs.end(); ++j) {

        std::vector<eckit::PathName> subdirs;
        eckit::PathName::match((*j) / "*:*", subdirs, false);

        Log::debug<LibFdb>() << "Subdirs " << subdirs << std::endl;

        for (std::set<Key>::const_iterator i = keys.begin(); i != keys.end(); ++i) {

            Regex re("^" + (*i).valuesToString() + "$");

            for (std::vector<eckit::PathName>::const_iterator k = subdirs.begin(); k != subdirs.end(); ++k) {

                if(seen.find(*k) != seen.end()) {
                    continue;
                }

                if (re.match((*k).baseName())) {
                    try {
                        TocHandler toc(*k);
                        if (toc.databaseKey().match(key)) {
                            result.push_back(*k);
                        }
                    } catch (eckit::Exception& e) {
                        eckit::Log::error() <<  "Error loading FDB database from " << *k << std::endl;
                        eckit::Log::error() << e.what() << std::endl;
                    }
                    seen.insert(*k);;
                }

            }
        }
    }

    return result;
}


std::vector<eckit::PathName> TocEngine::allLocations(const Key& key) const
{
    return databases(key, RootManager::allRoots(key));
}

std::vector<eckit::PathName> TocEngine::visitableLocations(const Key& key) const
{
    return databases(key, RootManager::visitableRoots(key));
}

std::vector<eckit::PathName> TocEngine::writableLocations(const Key& key) const
{
    return databases(key, RootManager::writableRoots(key));
}

void TocEngine::print(std::ostream& out) const
{
    out << "TocEngine()";
}

static EngineBuilder<TocEngine> toc_builder;

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5