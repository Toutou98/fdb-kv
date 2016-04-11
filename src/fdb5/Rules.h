/*
 * (C) Copyright 1996-2016 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file   Rules.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   April 2016

#ifndef fdb5_Rules_H
#define fdb5_Rules_H

#include <iosfwd>

#include "eckit/memory/NonCopyable.h"

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

class Rules : public eckit::NonCopyable {

public: // methods

	Rules();
    
    ~Rules();

    friend std::ostream& operator<<(std::ostream& s,const Rules& x);

private: // methods

    void print( std::ostream& out ) const;

};

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5

#endif
