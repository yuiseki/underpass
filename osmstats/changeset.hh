//
// Copyright (c) 2020, Humanitarian OpenStreetMap Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __CHANGESET_HH__
#define __CHANGESET_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <map>
#include <list>
#include <iostream>
#include <pqxx/pqxx>
#include <libxml++/libxml++.h>
#include <deque>
#include <osmium/io/any_input.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/io/any_output.hpp>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "hotosm.hh"
//#include "osmstats/osmstats.hh"

namespace changeset {

// enum objtype {building, waterway, highway, poi);

/// This class reads a change file
class ChangeSet
{
public:
    ChangeSet(void);
    ChangeSet(const std::deque<xmlpp::SaxParser::Attribute> attrs);

    void dump(void);

    void addHashtags(const std::string &text) {
        hashtags.push_back(text);
    };
    void addComment(const std::string &text) { comment = text; };
    void addEditor(const std::string &text) { editor = text; };
    
    // protected so testcases can access private data
// protected:
    // These fields come from the changeset replication file
    long id = 0;
    ptime created_at;
    ptime closed_at;
    bool open = false;
    std::string user;
    long uid = 0;
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;
    int num_changes = 0;
    int comments_count = 0;
    std::vector<std::string> hashtags;
    std::string comment;
    std::string editor;
//    objtype type;
};

class StateFile
{
public:
    StateFile(void) {
        timestamp = boost::posix_time::second_clock::local_time();
        sequence = 0;
    };
    /// Initialize with a state file from disk or memory
    StateFile(const std::string &file, bool memory);

    // protected so testcases can access private data
protected:
    ptime timestamp;
    long sequence;
};

class ChangeSetFile  : public xmlpp::SaxParser
{
public:
    /// Read a changeset file from disk or memory into internal storage
    bool readChanges(const std::string &file, bool memory);

    /// Import a changeset file from disk and initialize the database
    bool importChanges(const std::string &file);

    // Parse the XML data
    bool readXML(const std::string xml);
    
    // Used by libxml++
    void on_start_element(const Glib::ustring& name,
                          const AttributeList& properties) override;

    /// Get one set of change data from the parsed XML data
    ChangeSet &getChange(long id) { return changes[id]; };

    void dump(void);

protected:
    apidb::QueryStats osmdb;

    bool store;
    std::string filename;
    // Index by changeset ID
    // std::map<long, ChangeSet> changes;
    // No index
    std::vector<ChangeSet> changes;
};
}       // EOF changeset

#endif  // EOF __CHANGESET_HH__