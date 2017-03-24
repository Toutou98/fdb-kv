/*
 * (C) Copyright 1996-2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Simon Smart
/// @date   Dec 2016


#ifndef fdb5_pmem_MemoryBufferStream_H
#define fdb5_pmem_MemoryBufferStream_H

#include "eckit/serialisation/Stream.h"
#include "eckit/io/Buffer.h"
#include "eckit/io/Length.h"
#include "eckit/io/Offset.h"

#include <string>


namespace fdb5 {
namespace pmem {

// -------------------------------------------------------------------------------------------------

// A little helper class that might end up somewhere else

class MemoryBufferStream : public eckit::Stream {

public: // methods

    MemoryBufferStream();
    ~MemoryBufferStream();

    virtual long read(void*,long);
    virtual long write(const void*,long);
    virtual void rewind();
    virtual std::string name() const;

    size_t position() const;
    const eckit::Buffer& buffer() const;

private: // members

    eckit::Length size_;
    eckit::Offset position_;

    eckit::Buffer buffer_;
};


// -------------------------------------------------------------------------------------------------

} // namespace pmem
} // namespace fdb5

#endif // fdb5_pmem_MemoryBufferStream_H