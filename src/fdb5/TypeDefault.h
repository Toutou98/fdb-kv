/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file   TypeDefault.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   April 2016

#ifndef fdb5_TypeDefault_H
#define fdb5_TypeDefault_H

#include "fdb5/Type.h"

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

class TypeDefault : public Type {

public: // methods

    TypeDefault(const std::string& name, const std::string& type);

    virtual ~TypeDefault();

private: // methods

    virtual void print( std::ostream& out ) const;

};

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5

#endif