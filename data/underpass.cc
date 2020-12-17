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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
#endif

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <algorithm>
#include <utility>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
//#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include "osmstats/replication.hh"
#include "osmstats/changeset.hh"
#include "osmstats/osmchange.hh"
#include "data/underpass.hh"

namespace underpass {

Underpass::Underpass(const std::string &dbname) 
{
    frequency_tags[replication::minutely] = "minute";
    frequency_tags[replication::hourly] = "hour";
    frequency_tags[replication::daily] = "day";
    frequency_tags[replication::changeset] = "changeset";
    database = dbname;
    connect(dbname);
}

Underpass::Underpass(void)
{
    frequency_tags[replication::minutely] = "minute";
    frequency_tags[replication::hourly] = "hour";
    frequency_tags[replication::daily] = "day";
    frequency_tags[replication::changeset] = "changeset";
    connect(database);
};

Underpass::~Underpass(void)
{
    // db->disconnect();        // close the database connection
    db->close();            // close the database connection
}

// Dump internal data to the terminal, used only for debugging
void
Underpass::dump(void)
{
    std::cout << "Database: " << database << std::endl;
    std::cout << "DB Server: " << dbserver << std::endl;
    std::cout << "DB Port: " << port << std::endl;
}

bool
Underpass::connect(const std::string &dbname)
{
    // Connect to the database
    std::string args;
    if (dbname.empty()) {
	args = "dbname = " + database;
    } else {
	args = "dbname = " + dbname;
    }
    try {
        db = new pqxx::connection(args);
	sdb = std::make_shared<pqxx::connection>(args);
	if (db->is_open()) {
            // pqxx::work worker(db);
            // pqxx::nontransaction(db);
	    return true;
	} else {
	    return false;
	}
    } catch (const std::exception &e) {
	std::cerr << "ERROR: Couldn't open database connection to " << dbname << std::endl;
	std::cerr << e.what() << std::endl;
	return false;
    }
}

/// Update the creator table to track editor statistics
bool
Underpass::updateCreator(long user_id, long change_id, const std::string &editor,
                   const std::string &hashtags)
{

}

std::shared_ptr<replication::StateFile>
Underpass::getState(const std::string &path)
{
    //db_mutex.lock();
    std::string query = "SELECT timestamp,path,sequence,frequency FROM states WHERE path=\'";
    query += path + "\';";
    // std::cout << "QUERY: " << query << std::endl;
    pqxx::work worker(*db);
    pqxx::result result = worker.exec(query);
    worker.commit();
    auto state = std::make_shared<replication::StateFile>();
    if (result.size() > 0) {
        state->timestamp = time_from_string(pqxx::to_string(result[0][0]));
        state->path = pqxx::to_string(result[0][1]);
        state->sequence = result[0][2].as(int(0));
        state->frequency =  pqxx::to_string(result[0][3]);
    }
    //db_mutex.unlock();
}

// Get the state.txt date by timestamp
std::shared_ptr<replication::StateFile>
Underpass::getState(replication::frequency_t freq, ptime &tstamp)
{
    auto state = std::make_shared<replication::StateFile>();

    std::string query = "SELECT * FROM states WHERE timestamp>=";
    query += "\'" + to_simple_string(tstamp) + "\' ";
    query += " AND frequency=\'" + frequency_tags[freq] + "\'";
    query += " ORDER BY timestamp ASC LIMIT 1;";
    std::cout << "QUERY: " << query << std::endl;
    pqxx::work worker(*db);
    pqxx::result result = worker.exec(query);
    worker.commit();
    if (result.size() > 0) {
        state->timestamp = time_from_string(pqxx::to_string(result[0][0]));
        state->path = pqxx::to_string(result[0][1]);
        state->sequence = result[0][2].as(int(0));
        state->frequency =  pqxx::to_string(result[0][3]);
    }
    return state;
}

/// Write the stored data on the directories and timestamps
/// on the planet server.
bool
Underpass::writeState(replication::StateFile &state)
{
    std::string query = "INSERT INTO states(timestamp, sequence, path, frequency) VALUES(";
    query += "\'" + to_simple_string(state.timestamp) + "\',";
    query += std::to_string(state.sequence);
    query += ",\'" + state.path + "\'";
    if (state.path.find("changeset") != std::string::npos) {
        query += ", \'changeset\'";
    } else if (state.path.find("minute") != std::string::npos) {
        query += ", \'minute\'";
    } else if (state.path.find("hour") != std::string::npos) {
        query += ", \'hour\'";
    } else if (state.path.find("day") != std::string::npos) {
        query += ", \'day\'";
    }

    query += ") ON CONFLICT DO NOTHING;";
    // std::cout << "QUERY: " << query << std::endl;
    //db_mutex.lock();
    pqxx::work worker(*db);
    pqxx::result result = worker.exec(query);
    worker.commit();
    // db_mutex.unlock();
    // FIXME: return a real value
    return false;
}

/// Get the maximum timestamp for the state.txt data
std::shared_ptr<replication::StateFile>
Underpass::getLastState(replication::frequency_t freq)
{
    pqxx::work worker(*db);
    std::string query = "SELECT timestamp,sequence,path,frequency FROM states ORDER BY timestamp DESC LIMIT 1;";
    // std::cout << "QUERY: " << query << std::endl;
    pqxx::result result = worker.exec(query);
    auto last = std::make_shared<replication::StateFile>();
    last->timestamp = time_from_string(pqxx::to_string(result[0][0]));
    last->sequence = result[0][1].as(int(0));
    last->path = pqxx::to_string(result[0][2]);

    worker.commit();

    return last;
}

// Get the minimum timestamp for the state.txt data. As hashtags didn't
// appear until late 2014, we don't care as much about the older data.
std::shared_ptr<replication::StateFile>
Underpass::getFirstState(replication::frequency_t freq)
{
    pqxx::work worker(*db);
    std::string query = "SELECT timestamp,sequence,path,frequency FROM states ORDER BY timestamp ASC LIMIT 1;";
    // std::cout << "QUERY: " << query << std::endl;
    pqxx::result result = worker.exec(query);
    auto first = std::make_shared<replication::StateFile>();
    first->timestamp = time_from_string(pqxx::to_string(result[0][0]));
    first->sequence = result[0][1].as(int(0));
    first->path = pqxx::to_string(result[0][2]);

    worker.commit();

    return first;
}

} // EOF replication namespace

