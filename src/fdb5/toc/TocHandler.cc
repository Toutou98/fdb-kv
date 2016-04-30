/*
 * (C) Copyright 1996-2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <fcntl.h>

#include "eckit/config/Resource.h"
#include "eckit/io/FileHandle.h"
#include "eckit/serialisation/MemoryStream.h"


#include "fdb5/Index.h"
#include "fdb5/MasterConfig.h"
#include "fdb5/toc/TocHandler.h"
#include "fdb5/toc/TocIndex.h"


namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

class TocHandlerCloser {
    TocHandler &handler_;
  public:
    TocHandlerCloser(TocHandler &handler): handler_(handler) {}
    ~TocHandlerCloser() {
        handler_.close();
    }
};

//----------------------------------------------------------------------------------------------------------------------

TocHandler::TocHandler(const eckit::PathName &directory) :
    directory_(directory),
    filePath_(directory / "toc"),
    fd_(-1) {
}

TocHandler::~TocHandler() {
    close();
}

bool TocHandler::exists() const {
    return filePath_.exists();
}

void TocHandler::openForAppend() {

    ASSERT(fd_ == -1);

    // eckit::Log::info() << "Opening for append TOC " << filePath_ << std::endl;

    int iomode = O_WRONLY | O_APPEND;
    //#ifdef __linux__
    //  iomode |= O_NOATIME;
    //#endif
    SYSCALL2((fd_ = ::open( filePath_.asString().c_str(), iomode, (mode_t)0777 )), filePath_);
}

void TocHandler::openForRead() {

    ASSERT(fd_ == -1);

    // eckit::Log::info() << "Opening for read TOC " << filePath_ << std::endl;

    int iomode = O_RDONLY;
    //#ifdef __linux__
    //  iomode |= O_NOATIME;
    //#endif
    SYSCALL2((fd_ = ::open( filePath_.asString().c_str(), iomode )), filePath_ );
}

static size_t round(size_t a, size_t b) {
    return ((a + b - 1) / b) * b;
}

void TocHandler::append(TocRecord &r, size_t payloadSize ) {

    ASSERT(fd_ != -1);

    static size_t fdbRoundTocRecords = eckit::Resource<size_t>("fdbRoundTocRecords", 1024);

    r.header_.size_ = round(sizeof(TocRecord::Header) + payloadSize, fdbRoundTocRecords);

    size_t len;
    SYSCALL2( len = ::write(fd_, &r, r.header_.size_), filePath_ );
    ASSERT( len == r.header_.size_ );

}

bool TocHandler::readNext( TocRecord &r ) {

    int len;

    SYSCALL2( len = ::read(fd_, &r, sizeof(TocRecord::Header)), filePath_ );
    if (len == 0) {
        return false;
    }

    ASSERT(len == sizeof(TocRecord::Header));

    SYSCALL2( len = ::read(fd_, &r.payload_, r.header_.size_ - sizeof(TocRecord::Header)), filePath_ );
    ASSERT(len == r.header_.size_ - sizeof(TocRecord::Header));

    if ( TocRecord::currentVersion() != r.header_.version_ ) {
        std::ostringstream oss;
        oss << "Record version mistach, expected " << TocRecord::currentVersion()
            << ", got " << r.header_.version_;
        throw eckit::SeriousBug(oss.str());
    }

    return true;
}

void TocHandler::close() {
    if ( fd_ >= 0 ) {
        // eckit::Log::info() << "Closing TOC " << filePath_ << std::endl;
        SYSCALL2( ::close(fd_), filePath_ );
        fd_ = -1;
    }
}

void TocHandler::writeInitRecord(const Key &key) {

    if ( !directory_.exists() ) {
        directory_.mkdir();
    }

    int iomode = O_CREAT | O_RDWR;
    SYSCALL2(fd_ = ::open( filePath_.asString().c_str(), iomode, mode_t(0777) ), filePath_);

    TocHandlerCloser closer(*this);

    TocRecord r;

    size_t len = readNext(r);
    if (len == 0) {

        /* Copy rules first */

        eckit::PathName schemaPath(directory_ / "schema");

        eckit::Log::info() << "Copy schema from "
                           << MasterConfig::instance().schemaPath()
                           << " to "
                           << schemaPath
                           << std::endl;

        eckit::PathName tmp = eckit::PathName::unique(schemaPath);

        eckit::FileHandle in(MasterConfig::instance().schemaPath());
        eckit::FileHandle out(tmp);
        in.saveInto(out);

        eckit::PathName::rename(tmp, schemaPath);

        TocRecord r( TocRecord::TOC_INIT );
        eckit::MemoryStream s(&r.payload_[0], r.maxPayloadSize);
        s << key;
        append(r, s.position());

    } else {
        ASSERT(r.header_.tag_ == TocRecord::TOC_INIT);
    }
}

void TocHandler::writeClearRecord(const Index &index) {
    openForAppend();
    TocHandlerCloser closer(*this);

    TocRecord r( TocRecord::TOC_CLEAR );
    eckit::MemoryStream s(&r.payload_[0], r.maxPayloadSize);
    s << index.path().baseName();
    s << index.offset();
    append(r, s.position());

    eckit::Log::info() << "TOC_CLEAR " << index.path().baseName() << " - " << index.offset() << std::endl;

}

void TocHandler::writeWipeRecord() {
    openForAppend();
    TocHandlerCloser closer(*this);

    TocRecord r( TocRecord::TOC_WIPE );
    append(r, 0);
}

void TocHandler::writeIndexRecord(const Index &index) {
    openForAppend();
    TocHandlerCloser closer(*this);

    TocRecord r( TocRecord::TOC_INDEX );
    eckit::MemoryStream s(&r.payload_[0], r.maxPayloadSize);
    s << index.path().baseName();
    s << index.offset();
    index.encode(s);
    append(r, s.position());

    eckit::Log::info() << "TOC_INDEX " << index.path().baseName() << " - " << index.offset() << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------

class HasPath {
    eckit::PathName path_;
    off_t offset_;
  public:
    HasPath(const eckit::PathName &path, off_t offset): path_(path), offset_(offset) {}
    bool operator()(const Index *index) const {
        return (index->path() == path_) && (index->offset() == offset_);
    }
};

Key TocHandler::databaseKey() {
    openForRead();
    TocHandlerCloser close(*this);

    TocRecord r;

    while ( readNext(r) ) {
        if (r.header_.tag_ == TocRecord::TOC_INIT) {
            eckit::MemoryStream s(&r.payload_[0], r.maxPayloadSize);
            return Key(s);
        }
    }

    throw eckit::SeriousBug("Cannot find a TOC_INIT record");
}

std::vector<Index *> TocHandler::loadIndexes() {

    std::vector<Index *> indexes;

    if (!filePath_.exists()) {
        return indexes;
    }

    openForRead();
    TocHandlerCloser close(*this);

    TocRecord r;


    while ( readNext(r) ) {

        eckit::MemoryStream s(&r.payload_[0], r.maxPayloadSize);
        std::string path;
        off_t offset;
        std::vector<Index *>::iterator j;


        switch (r.header_.tag_) {

        case TocRecord::TOC_INIT:
            {
                Key key(s);
                eckit::Log::info() << "TOC_INIT key is " << key << std::endl;

//                if(dbKey.empty()) { dbKey = key; }
//                if(dbKey != key) {
//                    std::ostringstream oss;
//                    oss << "Key in TOC " << key << " doesn't match key from opened DB " << dbKey;
//                    throw eckit::SeriousBug(oss.str());
//                }
            }
            break;

        case TocRecord::TOC_INDEX:
            s >> path;
            s >> offset;
            eckit::Log::info() << "TOC_INDEX " << path << " - " << offset << std::endl;
            indexes.push_back( new TocIndex(s, directory_, directory_ / path, offset) );
            break;

        case TocRecord::TOC_CLEAR:
            s >> path;
            s >> offset;
            eckit::Log::info() << "TOC_CLEAR " << path << " - " << offset << std::endl;
            j = std::find_if (indexes.begin(), indexes.end(), HasPath(directory_ / path, offset));
            if (j != indexes.end()) {
                delete (*j);
                indexes.erase(j);
            }
            break;

        case TocRecord::TOC_WIPE:
            eckit::Log::info() << "TOC_WIPE" << std::endl;
            freeIndexes(indexes);
            break;

        default:
            throw eckit::SeriousBug("Unknown tag in TocRecord", Here());
            break;

        }

    }

    std::reverse(indexes.begin(), indexes.end()); // the entries of the last index takes precedence

    eckit::Log::info() << "TOC indexes " << indexes.size() << std::endl;

    return indexes;

}

void TocHandler::freeIndexes(std::vector<Index *> &indexes) {
    for (std::vector<Index *>::iterator j = indexes.begin(); j != indexes.end(); ++j) {
        delete (*j);
    }
    indexes.clear();
}


//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5