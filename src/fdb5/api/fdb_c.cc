/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */
#include <unordered_set>
#include <string.h>

#include "eckit/io/MemoryHandle.h"
#include "eckit/io/FileDescHandle.h"
#include "eckit/message/Message.h"
#include "eckit/runtime/Main.h"

#include "metkit/mars/MarsRequest.h"

#include "fdb5/fdb5_version.h"
#include "fdb5/api/FDB.h"
#include "fdb5/api/helpers/FDBToolRequest.h"
#include "fdb5/api/helpers/ListIterator.h"
#include "fdb5/database/Key.h"

#include "fdb5/api/fdb_c.h"

using namespace fdb5;
using namespace eckit;

extern "C" {

//----------------------------------------------------------------------------------------------------------------------


struct fdb_handle_t : public FDB {
    using FDB::FDB;
};

struct fdb_key_t : public Key {
    using Key::Key;
};

struct fdb_request_t {
public:
    fdb_request_t() : request_(metkit::mars::MarsRequest()) {}
    fdb_request_t(std::string str) {
        request_ = metkit::mars::MarsRequest(str);
    }
    void values(const char* name, const char* values[], int numValues) {
        std::string n(name);
        std::vector<std::string> vv;
        for (int i=0; i<numValues; i++) {
            vv.push_back(std::string(values[i]));
        }
        request_.values(n, vv);
    }
    static fdb_request_t* parse(std::string str) {
        fdb_request_t* req = new fdb_request_t();
        req->request_ = metkit::mars::MarsRequest::parse(str);
        return req;
    }
    const metkit::mars::MarsRequest request() const { return request_; }
private:
    metkit::mars::MarsRequest request_;
};


class ListIteratorHolder {
public:
    ListIteratorHolder(DedupListIterator& iter) : iter_(std::move(iter)) {}

    DedupListIterator& get() { return iter_; }

private:
    DedupListIterator iter_;

};


struct fdb_listiterator_t {
public:
    fdb_listiterator_t() : validEl_(false), iter_(nullptr) {}

    void set(DedupListIterator&& iter) {
        iter_.reset(new ListIteratorHolder(iter));
    }

    int next() {
        ASSERT(iter_);
        
        validEl_ = iter_->get().next(el_);

        return validEl_ ? FDB_SUCCESS : FDB_ITERATION_COMPLETE;
    }

    void attrs(char** uri, size_t* off, size_t* len) {
        ASSERT(validEl_);

        const FieldLocation& loc = el_.location();
        const std::string path = loc.uri().name();
        *uri = new char[path.length()+1];
        strcpy(*uri, path.c_str());
        *off = loc.offset();
        *len = loc.length();
    }

    void key(fdb_key_t *key) {
        ASSERT(validEl_);

        for (auto k : el_.combinedKey()) {
            key->set(k.first, k.second);
        }
    }

private:
    bool validEl_;
    ListElement el_;
    std::unique_ptr<ListIteratorHolder> iter_;
};

struct fdb_datareader_t {
public:
    long open() {
        ASSERT(dh_);
        return dh_->openForRead();
    }
    void close() {
        ASSERT(dh_);
        dh_->close();
    }
    long tell() {
        ASSERT(dh_);
        return dh_->position();
    }
    long seek(long pos) {
        ASSERT(dh_);
        return dh_->seek(pos);
    }
    void skip(long pos) {
        ASSERT(dh_);
        dh_->skip(pos);
    }
    long read(void* buf, long length) {
        ASSERT(dh_);
        return dh_->read(buf, length);
    }
    void set(DataHandle* dh) {
        if (dh_)
            delete dh_;
        dh_ = dh;
    }
    
private:
    DataHandle* dh_;
};

//----------------------------------------------------------------------------------------------------------------------

/* Error handling */

static std::string g_current_error_str;
static fdb_failure_handler_t g_failure_handler = nullptr;
static void* g_failure_handler_context = nullptr;

const char* fdb_error_string(int err) {
    switch (err) {
    case FDB_SUCCESS:
        return "Success";
    case FDB_ERROR_GENERAL_EXCEPTION:
    case FDB_ERROR_UNKNOWN_EXCEPTION:
        return g_current_error_str.c_str();
    case FDB_ITERATION_COMPLETE:
        return "Iteration complete";
    default:
        return "<unknown>";
    };
}

} // extern "C"

// Template can't have C linkage


namespace {

// Template magic to provide a consistent error-handling approach
int innerWrapFn(std::function<int()> f) {
    return f();
}

int innerWrapFn(std::function<void()> f) {
    f();
    return FDB_SUCCESS;
}

template <typename FN>
int wrapApiFunction(FN f) {

    try {
        return innerWrapFn(f);
    } catch (Exception& e) {
        Log::error() << "Caught exception on C-C++ API boundary: " << e.what() << std::endl;
        g_current_error_str = e.what();
        if (g_failure_handler) {
            g_failure_handler(g_failure_handler_context, FDB_ERROR_GENERAL_EXCEPTION);
        }
        return FDB_ERROR_GENERAL_EXCEPTION;
    } catch (std::exception& e) {
        Log::error() << "Caught exception on C-C++ API boundary: " << e.what() << std::endl;
        g_current_error_str = e.what();
        if (g_failure_handler) {
            g_failure_handler(g_failure_handler_context, FDB_ERROR_GENERAL_EXCEPTION);
        }
        return FDB_ERROR_GENERAL_EXCEPTION;
    } catch (...) {
        Log::error() << "Caught unknown on C-C++ API boundary" << std::endl;
        g_current_error_str = "Unrecognised and unknown exception";
        if (g_failure_handler) {
            g_failure_handler(g_failure_handler_context, FDB_ERROR_UNKNOWN_EXCEPTION);
        }
        return FDB_ERROR_UNKNOWN_EXCEPTION;
    }

    ASSERT(false);
}

}

extern "C" {

//----------------------------------------------------------------------------------------------------------------------

// TODO: In a sensible, templated, try catch all exceptions.
//       --> We ought to have a standardised error return process.


/*
 * Initialise API
 * @note This is only required if being used from a context where Main()
 *       is not otherwise initialised
*/
int fdb_initialise() {
    return wrapApiFunction([] {
        static bool initialised = false;

        if (initialised) {
            Log::warning() << "Initialising FDB library twice" << std::endl;
        }

        if (!initialised) {
            const char* argv[2] = {"fdb-api", 0};
            Main::initialise(1, const_cast<char**>(argv));
            initialised = true;
        }
    });
}


int fdb_set_failure_handler(fdb_failure_handler_t handler, void* context) {
    return wrapApiFunction([handler, context] {
        g_failure_handler = handler;
        g_failure_handler_context = context;
        eckit::Log::info() << "FDB setting failure handler fn." << std::endl;
    });
}

int fdb_version(const char** version) {
    return wrapApiFunction([version]{
        (*version) = fdb5_version_str();
    });
}

int fdb_vcs_version(const char** sha1) {
    return wrapApiFunction([sha1]{
        (*sha1) = fdb5_git_sha1();
    });
}


int fdb_new_handle(fdb_handle_t** fdb) {
    return wrapApiFunction([fdb] {
        *fdb = new fdb_handle_t();
    });
}

int fdb_archive(fdb_handle_t* fdb, fdb_key_t* key, const char* data, size_t length) {
    return wrapApiFunction([fdb, key, data, length] {
        ASSERT(fdb);
        ASSERT(key);
        ASSERT(data);

        fdb->archive(*key, data, length);
    });
}
int fdb_archive_multiple(fdb_handle_t* fdb, fdb_request_t* req, const char* data, size_t length) {
    return wrapApiFunction([fdb, req, data, length] {
        ASSERT(fdb);
        ASSERT(data);

        eckit::MemoryHandle handle(data, length);
        if (req) {
            fdb->archive(req->request(), handle);
        }
        else {
            fdb->archive(handle);
        }
    });
}

int fdb_list(fdb_handle_t* fdb, const fdb_request_t* req, bool duplicates, fdb_listiterator_t* it) {
    return wrapApiFunction([fdb, req, duplicates, it] {
        ASSERT(fdb);
        ASSERT(it);

        std::vector<std::string> minKeySet; // we consider an empty set
        const FDBToolRequest toolRequest(
            req ? req->request() : metkit::mars::MarsRequest(),
            req == nullptr, minKeySet);

        it->set(fdb->list(toolRequest, duplicates));
    });
}
int fdb_retrieve(fdb_handle_t* fdb, fdb_request_t* req, fdb_datareader_t* dr) {
    return wrapApiFunction([fdb, req, dr] {
        ASSERT(fdb);
        ASSERT(req);
        ASSERT(dr);
        dr->set(fdb->retrieve(req->request()));
    });
}
int fdb_flush(fdb_handle_t* fdb) {
    return wrapApiFunction([fdb] {
        ASSERT(fdb);

        fdb->flush();
    });
}

int fdb_delete_handle(fdb_handle_t* fdb) {
    return wrapApiFunction([fdb]{
        ASSERT(fdb);
        delete fdb;
    });
}

/** ancillary functions for creating/destroying FDB objects */

int fdb_new_key(fdb_key_t** key) {
    return wrapApiFunction([key] {
        *key = new fdb_key_t();
    });
}
int fdb_key_add(fdb_key_t* key, const char* param, const char* value) {
    return wrapApiFunction([key, param, value]{
        ASSERT(key);
        ASSERT(param);
        ASSERT(value);
        key->set(std::string(param), std::string(value));
    });
}

int fdb_key_dict(fdb_key_t* key, fdb_key_dict_t** dict, size_t* length) {
    return wrapApiFunction([key, dict, length]{
        ASSERT(key);
        ASSERT(dict);
        ASSERT(length);
        const eckit::StringDict& keyDict = key->keyDict();
        *length = keyDict.size();
        *dict = new fdb_key_dict_t[*length];
        int i=0;
        for (auto k: keyDict) {
            (*dict)[i].key = new char[k.first.length()+1];
            strcpy((*dict)[i].key, k.first.c_str());
            (*dict)[i].value = new char[k.second.length()+1];
            strcpy((*dict)[i].value, k.second.c_str());
            i++;
        }
    });
}
int fdb_delete_key_dict(fdb_key_dict_t* dict, size_t length) {
    return wrapApiFunction([dict, length]{
        ASSERT(dict);
        for (size_t i=0; i<length; i++) {
            delete dict[i].key;
            delete dict[i].value;
        }
        delete dict;
    });
}

int fdb_delete_uri(char* uri) {
    return wrapApiFunction([uri]{
        ASSERT(uri);
        delete uri;
    });
}

int fdb_delete_key(fdb_key_t* key) {
    return wrapApiFunction([key]{
        ASSERT(key);
        delete key;
    });
}

int fdb_new_request(fdb_request_t** req) {
    return wrapApiFunction([req] {
        *req = new fdb_request_t("retrieve");
    });
}
int fdb_request_add(fdb_request_t* req, const char* param, const char* values[], int numValues) {
    return wrapApiFunction([req, param, values, numValues] {
        ASSERT(req);
        ASSERT(param);
        ASSERT(values);
        req->values(param, values, numValues);
    });
}
int fdb_delete_request(fdb_request_t* req) {
    return wrapApiFunction([req]{
        ASSERT(req);
        delete req;
    });
}

int fdb_new_listiterator(fdb_listiterator_t** it) {
    return wrapApiFunction([it]{
        *it = new fdb_listiterator_t();
    });
}
int fdb_listiterator_next(fdb_listiterator_t* it) {
    return wrapApiFunction(std::function<int()> {[it] {
        ASSERT(it);
        return it->next();
    }});
}
int fdb_listiterator_attrs(fdb_listiterator_t* it, char** uri, size_t* off, size_t* len) {
    return wrapApiFunction([it, uri, off, len] {
        ASSERT(it);
        ASSERT(uri);
        ASSERT(off);
        ASSERT(len);
        it->attrs(uri, off, len);
    });
}
int fdb_listiterator_key(fdb_listiterator_t* it, fdb_key_t* key) {
    return wrapApiFunction([it, key] {
        ASSERT(it);
        ASSERT(key);
        it->key(key);
    });
}

int fdb_delete_listiterator(fdb_listiterator_t* it) {
    return wrapApiFunction([it]{
        ASSERT(it);
        delete it;
    });
}

int fdb_new_datareader(fdb_datareader_t** dr) {
    return wrapApiFunction([dr]{
        *dr = new fdb_datareader_t();
    });
}
int fdb_datareader_open(fdb_datareader_t* dr, long* size) {
    return wrapApiFunction([dr, size]{
        ASSERT(dr);
        long tmp;
        tmp = dr->open();
        if (size)
            *size = tmp;
    });
}
int fdb_datareader_close(fdb_datareader_t* dr) {
    return wrapApiFunction([dr]{
        ASSERT(dr);
        dr->close();
    });
}
int fdb_datareader_tell(fdb_datareader_t* dr, long* pos) {
    return wrapApiFunction([dr, pos]{
        ASSERT(dr);
        ASSERT(pos);
        *pos = dr->tell();
    });
}
int fdb_datareader_seek(fdb_datareader_t* dr, long pos) {
    return wrapApiFunction([dr, pos]{
        ASSERT(dr);
        dr->seek(pos);
    });
}
int fdb_datareader_skip(fdb_datareader_t* dr, long count) {
    return wrapApiFunction([dr, count]{
        ASSERT(dr);
        dr->skip(count);
    });
}
int fdb_datareader_read(fdb_datareader_t* dr, void *buf, long count, long* read) {
    return wrapApiFunction([dr, buf, count, read]{
        ASSERT(dr);
        ASSERT(buf);
        ASSERT(read);
        *read = dr->read(buf, count);
    });
}
int fdb_delete_datareader(fdb_datareader_t* dr) {
    return wrapApiFunction([dr]{
        ASSERT(dr);
        dr->set(nullptr);
        delete dr;
    });
}

//----------------------------------------------------------------------------------------------------------------------

} // extern "C"
