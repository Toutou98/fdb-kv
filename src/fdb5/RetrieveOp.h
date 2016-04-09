/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file   RetrieveOp.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   Mar 2016

#ifndef fdb5_RetrieveOp_H
#define fdb5_RetrieveOp_H


#include "fdb5/Op.h"
#include "fdb5/HandleGatherer.h"

namespace fdb5 {

class DB;

//----------------------------------------------------------------------------------------------------------------------

class RetrieveOp : public fdb5::Op {

public: // methods

    RetrieveOp(const DB& db, HandleGatherer& result);

    /// Destructor

    virtual ~RetrieveOp();

private: // methods

    virtual void enter(const std::string& param, const std::string& value);
    virtual void leave();

    virtual void execute(const MarsTask& task, Key& key, Op& tail);

    virtual void fail(const MarsTask& task, Key& key, Op &tail);

private:

    virtual void print( std::ostream& out ) const;

private: // members

    const DB& db_;

    HandleGatherer& result_;
};

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5

#endif
