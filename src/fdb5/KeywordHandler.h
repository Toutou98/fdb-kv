/*
 * (C) Copyright 1996-2016 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file   KeywordHandler.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   April 2016

#ifndef fdb5_KeywordHandler_H
#define fdb5_KeywordHandler_H

#include <string>

#include "eckit/memory/NonCopyable.h"

class MarsTask;

namespace fdb5 {

class Op;

//----------------------------------------------------------------------------------------------------------------------

class KeywordHandler : private eckit::NonCopyable {

public: // methods

    KeywordHandler(const std::string& name);
    
    virtual ~KeywordHandler();

    virtual Op* makeOp(const MarsTask& task, Op& parent) const = 0;

    friend std::ostream& operator<<(std::ostream& s,const KeywordHandler& x);

public: // class methods

    static const KeywordHandler& lookup(const std::string& keyword);

private: // methods

    virtual void print( std::ostream& out ) const = 0;

protected: // members

    std::string name_;

};

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5

#endif