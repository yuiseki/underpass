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

#ifndef __OSMSTATS_HH__
#define __OSMSTATS_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <algorithm>
#include <pqxx/pqxx>

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "hotosm.hh"
#include "osmstats/changeset.hh"

using namespace apidb;

namespace osmstats {

class OsmStats
{
public:
    OsmStats(const pqxx::result &res);
    OsmStats(pqxx::const_result_iterator &res);
    OsmStats(const std::string filespec);
    void dump(void);
// protected:
    // These are from the OSM Stats 'raw_changesets' table
    long id;
    long road_km_added;
    long road_km_modified;
    long waterway_km_added;
    long waterway_km_modified;
    long roads_added;
    long roads_modified;
    long waterways_added;
    long waterways_modified;
    long buildings_added;
    long buildings_modified;
    long pois_added;
    long pois_modified;
    std::string editor;
    long user_id;
    ptime created_at;
    ptime closed_at;
    bool verified;
    std::vector<long> augmented_diffs;
    ptime updated_at;
};

/// Stores the data from the raw countries table
class RawCountry
{
  public:
    RawCountry(void) { };
    RawCountry(pqxx::const_result_iterator &res) {
        id = res[0].as(int(0));
        name = res[1].c_str();
        abbrev = res[2].c_str();
    }
    RawCountry(int cid, const std::string &country, const std::string &code) {
        id = cid;
        name = country;
        abbrev = code;
    }
    int id;
    std::string name;
    std::string abbrev;
};

/// Stores the data from the raw user table
class RawUser
{
  public:
    RawUser(void) { };
    RawUser(pqxx::const_result_iterator &res) {
        id = res[0].as(int(0));
        name = res[1].c_str();
    }
    RawUser(long uid, const std::string &tag) {
        id = uid;
        name = tag;
    }
    int id;
    std::string name;
};

/// Stores the data from the raw user table
class RawHashtag
{
  public:
    RawHashtag(void) { };
    RawHashtag(pqxx::const_result_iterator &res) {
        id = res[0].as(int(0));
        name = res[1].c_str();
    }
    RawHashtag(int hid, const std::string &tag) {
        id = hid;
        name = tag;
    }
    int id;
    std::string name;
};

class QueryOSMStats : public apidb::QueryStats
{
  public:
    QueryOSMStats(void);
    /// Connect to the database
    bool connect(const std::string &database);

    /// Populate internal storage of a few heavily used data, namely
    /// the indexes for each user, country, or hashtag.
    bool populate(void);

    /// Read changeset data from the osmstats database
    bool getRawChangeSet(std::vector<long> &changeset_id);

    /// Add a user to the internal data store
    int addUser(long id, const std::string &user) {
        RawUser ru(id, user);
        users.push_back(ru);
    };
    /// Add a country to the internal data store
    int addCountry(long id, const std::string &name, const std::string &code) {
        RawCountry rc(id, name, code);
        countries.push_back(rc);
    };
    /// Add a hashtag to the internal data store
    int addHashtag(int id, const std::string &tag) {
        RawHashtag rh(id, tag);
        hashtags[tag] = rh;
    };

    /// Add a comment and their ID to the database
    int addComment(long id, const std::string &user);

    /// Write the list of hashtags to the database
    int updatedRawUsers(void);

    /// Write the list of hashtags to the database
    int updateRawHashtags(void);

    /// Add a changeset ID and the country it is in
    int updateCountries(int id, int country_id);

    // Accessor classes to extract country data from the database

    /// Get the country ID. name, and ISO abbreviation from the
    // raw_countries table.
    RawCountry &getCountryData(long id) { return countries[id]; }
    RawUser &getUserData(long id) { return users[id]; }
    // RawHashtag &getHashtag(long id) { return hashtags.find(id); }
    long getHashtagID(const std::string name) { return hashtags[name].id; }

    /// Apply a change to the database
    bool applyChange(changeset::ChangeSet &change);

    // void commit(void) { worker->commit(); };

    /// Dump internal data, debugging usage only!
    void dump(void);
private:
    pqxx::connection *db;
    pqxx::work *worker;

    std::vector<OsmStats> ostats;
    std::vector<RawCountry> countries;
    std::vector<RawUser> users;
    std::map<std::string, RawHashtag> hashtags;

    // These are only used to get performance statistics used for
    // debugging.
    ptime start;                // Starting timestamop for operation
    ptime end;                  // Ending timestamop for operation
};

    
}       // EOF osmstatsdb

#endif  // EOF __OSMSTATS_HH__
