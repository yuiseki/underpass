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
// #include <pqxx/pqxx>
#include <fstream>
#include <cctype>
#include <cmath>
#include <sstream>
// #include <iterator>
#ifdef LIBXML
#  include <libxml++/libxml++.h>
#endif
#include <gumbo.h>

#include <osmium/io/any_input.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/io/any_output.hpp>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
//#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http/parser.hpp>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;   // from <boost/asio/ssl.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

#include "hotosm.hh"
#include "osmstats/replication.hh"
#include "osmstats/changeset.hh"

/// Control access to the database connection
std::mutex db_mutex;

namespace replication {

/// Parse the two state files for a replication file, from
/// disk or memory.

/// There are two types of state files with of course different
/// formats for the same basic data. The simplest one is for
/// a changeset file. which looks like this:
///
/// \-\-\-
/// last_run: 2020-10-08 22:30:01.737719000 +00:00
/// sequence: 4139992
///
/// The other format is used for minutely change files, and
/// has more fields. For now, only the timestamp and sequence
/// number is stored. It looks like this:
///
/// \#Fri Oct 09 10:03:04 UTC 2020
/// sequenceNumber=4230996
/// txnMaxQueried=3083073477
/// txnActiveList=
/// txnReadyList=
/// txnMax=3083073477
/// timestamp=2020-10-09T10\:03\:02Z
///
/// State files are used to know where to start downloading files
StateFile::StateFile(const std::string &file, bool memory)
{
    std::string line;
    std::ifstream state;
    std::stringstream ss;

    // It's a disk file, so read it in.
    if (!memory) {
        try {
            state.open(file, std::ifstream::in);
        }
        catch(std::exception& e) {
            std::cout << "ERROR opening " << file << std::endl;
            std::cout << e.what() << std::endl;
            // return false;
        }
        // For a disk file, none of the state files appears to be larger than
        // 70 bytes, so read the whole thing into memory without
        // any iostream buffering.
        std::filesystem::path path = file;
        int size = std::filesystem::file_size(path);
        char *buffer = new char[size];
        state.read(buffer, size);
        ss << buffer;
        // FIXME: We do it this way to save lots of extra buffering
        // ss.rdbuf()->pubsetbuf(&buffer[0], size);
    } else {
        // It's in memory
        ss << file;
    }

    // Get the first line
    std::getline(ss, line, '\n');

    // This is a changeset state.txt file
    if (line == "---") {
        // Second line is the last_run timestamp
        std::getline(ss, line, '\n');
        // The timestamp is the second field
        std::size_t pos = line.find(" ");
        // 2020-10-08 22:30:01.737719000 +00:00
        timestamp = time_from_string(line.substr(pos+1));

        // Third and last line is the sequence number
        std::getline(ss, line, '\n');
        pos = line.find(" ");
        // The sequence is the second field
        sequence = std::stol(line.substr(pos+1));
        // This is a change file state.txt file
    } else {
        for (std::string line; std::getline(ss, line, '\n'); ) {
            std::size_t pos = line.find("=");
            if (line.substr(0, pos) == "sequenceNumber") {
                sequence= std::stol(line.substr(pos+1));
            } else if (line.substr(0, pos) == "txnMaxQueried") {
            } else if (line.substr(0, pos) == "txnActiveList") {
            } else if (line.substr(0, pos) == "txnReadyList") {
            } else if (line.substr(0, pos) == "txnMax") {
            } else if (line.substr(0, pos) == "timestamp") {
                // The time stamp is in ISO format, ie... 2020-10-09T10\:03\:02
                // But we have to unescape the colon or boost chokes.
                std::string tmp = line.substr(pos+1);
                pos = tmp.find('\\', pos+1);
                std::string tstamp = tmp.substr(0, pos); // get the date and the hour
                tstamp += tmp.substr(pos+1, 3); // get minutes
                pos = tmp.find('\\', pos+1);
                tstamp += tmp.substr(pos+1, 3); // get seconds
                timestamp = from_iso_extended_string(tstamp);
            }
        }
    }

    state.close();
}

// Dump internal data to the terminal, used only for debugging
void
StateFile::dump(void)
{
    std::cout << "Dumping state.txt file" << std::endl;    
    std::cout << "\tTimestamp: " << timestamp << std::endl;
    std::cout << "\tSequence: " << sequence << std::endl;
    std::cout << "\tPath: " << path << std::endl;
}

// parse a replication file containing changesets
bool
Replication::readChanges(const std::string &file)
{
    return false;
}

// Add this replication data to the changeset database
bool
Replication::mergeToDB()
{
    return false;
}

std::shared_ptr<std::vector<std::string>> &
Planet::getLinks(GumboNode* node, std::shared_ptr<std::vector<std::string>> &links)
{
    // if (node->type == GUMBO_NODE_TEXT) {
    //     std::string val = std::string(node->v.text.text);
    //     std::cout << "FIXME: " << "GUMBO_NODE_TEXT " << val << std::endl;
    // }
    
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute* href;
        if (node->v.element.tag == GUMBO_TAG_A &&
            (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
            // All the directories are a 3 digit number, and all the files
            // start with a 3 digit number
            if (href->value[0] >= 48 && href->value[0] <= 57) {
                //if (std::isalnum(href->value[0]) && std::isalnum(href->value[1])) {
                // std::cout << "FIXME: " << href->value << std::endl;
                if (std::strlen(href->value) > 0) {
                    links->push_back(href->value);
                }
            }
        }
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            getLinks(static_cast<GumboNode*>(children->data[i]), links);
        }
    }

    return links;
}

// Download a file from planet
std::shared_ptr<std::vector<unsigned char>>
Planet::downloadFile(const std::string &file)
{
    boost::system::error_code ec;
    auto data = std::make_shared<std::vector<unsigned char>>();
    // std::cout << "Downloading: " << file << std::endl;
    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, file, version };

    req.keep_alive();
    req.set(http::field::host, server);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;

    // Receive the HTTP response
    http::response_parser<http::string_body> parser;
    // read_header(stream, buffer, parser);
    read(stream, buffer, parser);
    if (parser.get().result() == boost::beast::http::status::not_found) {
        // continue;
    }

    // Check the magic number of the file
    if (parser.get().body()[0] == 0x1f) {
        std::cout << "It's gzipped" << std::endl;
    } else {
        if (parser.get().body()[0] == '<') {
            std::cout << "It's XML" << std::endl;
        }
    }

    for (auto body = std::begin(parser.get().body()); body != std::end(parser.get().body()); ++body) {
        data->push_back((unsigned char)*body);
    }
    return data;
}

// Download a file from planet
std::shared_ptr<std::vector<unsigned char>>
Replication::downloadFiles(std::vector<std::string> files, bool disk)
{
    boost::system::error_code ec;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    ssl::stream<tcp::socket> stream{ioc, ctx};

    // Look up the domain name
    auto const results = resolver.resolve(server, std::to_string(port));

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    try {
        stream.handshake(ssl::stream_base::client, ec);
    } catch (const std::exception &e) {
        std::cerr << "Couldn't shutdown stream" << e.what() << std::endl;
    }
    //stream.expires_after (std::chrono::seconds(30));
    auto data = std::make_shared<std::vector<unsigned char>>();
    auto links =  std::make_shared<std::vector<std::string>>();
    for (auto it = std::begin(files); it != std::end(files); ++it) {
        std::cout << "Downloading " << *it << std::endl;
        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, *it, version };

        req.set(http::field::host, server);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persistant
        boost::beast::flat_buffer buffer;

        // Receive the HTTP response
        http::response_parser<http::string_body> parser;
        // read_header(stream, buffer, parser);
        read(stream, buffer, parser);
        if (parser.get().result() == boost::beast::http::status::not_found) {
            continue;
        }
        
        // read(stream, buffer, parser.base());
        // Parse the HTML content to extract the hyperlinks to
        // the directories and files
        // Check the magic number of the file
        if (parser.get().body()[0] == 0x1f) {
            std::cout << "It's gzipped" << std::endl;
            // it's gzipped
        } else {
            if (parser.get().body()[0] == '<') {
                // It's an OSM XML file
                std::cout << "It's XML" << std::endl;
                if (disk) {
                    // FIXME: write to disk is specified
                }
                // ::cout << parser.get().body() << std::endl;
            } else {
                // It's text, probably a state.txt file. State
                // files are never compressed.
                //StateFile fooby(parser.get().body(), true);
                // Copy the data into the shared point, as this memory will be
                // deleted.
                for (auto body = std::begin(parser.get().body()); body != std::end(parser.get().body()); ++body) {
                    data->push_back((unsigned char)*body);
                }
                
                //data->reserve(parser.get().body().size());
                // std::copy(parser.get().body().begin(), parser.get().body().end(),
                //           std::back_inserter(data));
                // states.push_back(fooby);
                // fooby.dump();
                if (disk) {
                    // FIXME: write to disk is specified
                }
            }
        }
        if (files.size() == 1) {
            path += files[0];
        }
    }

    // Gracefully close the socket
    stream.shutdown(ec);
    if (ec) {
        std::cerr << "ERROR: stream shutdown failed" << ": " << ec.message() << std::endl;
    }
    
    return data;
}

Planet::~Planet(void)
{
    boost::system::error_code ec;

    db->close();                // close the database connection
    ioc.reset();                // reset the I/O conhtext
    stream.shutdown();          // shutdown the socket used by the stream
}

Planet::Planet(void)
{
    frequency_tags[replication::minutely] = "minute";
    frequency_tags[replication::hourly] = "hour";
    frequency_tags[replication::daily] = "day";
    frequency_tags[replication::changeset] = "changeset";
    
    connectDB();
    connectServer();
};


// Dump internal data to the terminal, used only for debugging
void
Planet::dump(void)
{
    std::cout << "Dumping Planet data" << std::endl;
    for (auto it = std::begin(changeset); it != std::end(changeset); ++it) {
        std::cout << "Changeset at: " << it->first << ": " << it->second << std::endl;
    }
    for (auto it = std::begin(minute); it != std::end(minute); ++it) {
        std::cout << "Minutely at: " << it->first << ": " << it->second << std::endl;
    }
    for (auto it = std::begin(hour); it != std::end(hour); ++it) {
        std::cout << "Minutely at: " << it->first << ": " << it->second << std::endl;
    }
    for (auto it = std::begin(day); it != std::end(day); ++it) {
        std::cout << "Daily at: " << it->first << ": " << it->second << std::endl;
    }
}

bool
Planet::connectDB(const std::string &dbname)
{
    // Connect to the database
    std::string args;
    if (dbname.empty()) {
	args = "dbname = underpass";
    } else {
	args = "dbname = " + dbname;
    }
    try {
	db = new pqxx::connection(args);
	if (db->is_open()) {
	    return true;
	} else {
	    return false;
	}
    } catch (const std::exception &e) {
	std::cerr << e.what() << std::endl;
	return false;
    }
}

bool
Planet::connectServer(const std::string &planet)
{
    if (!planet.empty()) {
        server = planet;
    }

    // Gracefully close the socket
    boost::system::error_code ec;
    ioc.reset();
    ctx.set_verify_mode(ssl::verify_none);
    auto const results = resolver.resolve(server, std::to_string(port));
    boost::asio::connect(stream.next_layer(), results.begin(), results.end(), ec);
    if (ec) {
        std::cerr << "ERROR: stream connect failed" << ": " << ec.message() << std::endl;
        return false;
    }
    stream.handshake(ssl::stream_base::client, ec);
    if (ec) {
        std::cerr << "ERROR: stream handshake failed" << ": " << ec.message() << std::endl;
        return false;
    }

    return true;
}

/// Get the maximum timestamp for the state.txt data
std::shared_ptr<StateFile>
Planet::getLastState(frequency_t freq)
{
    worker = new pqxx::work(*db);
    std::string query = "SELECT timestamp,sequence,path,frequency FROM states ORDER BY timestamp DESC LIMIT 1;";
    // std::cout << "QUERY: " << query << std::endl;
    pqxx::result result = worker->exec(query);
    auto last = std::make_shared<StateFile>();
    last->timestamp = time_from_string(pqxx::to_string(result[0][0]));
    last->sequence = result[0][1].as(int(0));
    last->path = pqxx::to_string(result[0][2]);

    worker->commit();

    return last;
}

// Get the minimum timestamp for the state.txt data. As hashtags didn't
// appear until late 2014, we don't care as much about the older data.
std::shared_ptr<StateFile>
Planet::getFirstState(frequency_t freq)
{
    worker = new pqxx::work(*db);
    std::string query = "SELECT timestamp,sequence,path,frequency FROM states ORDER BY timestamp ASC LIMIT 1;";
    // std::cout << "QUERY: " << query << std::endl;
    pqxx::result result = worker->exec(query);
    auto first = std::make_shared<StateFile>();
    first->timestamp = time_from_string(pqxx::to_string(result[0][0]));
    first->sequence = result[0][1].as(int(0));
    first->path = pqxx::to_string(result[0][2]);

    worker->commit();

    return first;
}

/// Write the stored data on the directories and timestamps
/// on the planet server.
bool
Planet::writeState(StateFile &state)
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
    db_mutex.lock();
    worker = new pqxx::work(*db);
    pqxx::result result = worker->exec(query);
    worker->commit();
    db_mutex.unlock();
    // FIXME: return a real value
    return false;
}

std::shared_ptr<StateFile>
Planet::getState(const std::string &path)
{
    db_mutex.lock();
    std::string query = "SELECT timestamp,path,sequence,frequency FROM states WHERE path=\'";
    query += path + "\';";
    // std::cout << "QUERY: " << query << std::endl;
    worker = new pqxx::work(*db);
    pqxx::result result = worker->exec(query);
    worker->commit();
    auto state = std::make_shared<StateFile>();
    if (result.size() > 0) {
        state->timestamp = time_from_string(pqxx::to_string(result[0][0]));
        state->path = pqxx::to_string(result[0][1]);
        state->sequence = result[0][2].as(int(0));
        state->frequency =  pqxx::to_string(result[0][3]);
    }
    db_mutex.unlock();

    return state;
}

// Get the state.txt date by timestamp
std::shared_ptr<StateFile>
Planet::getState(frequency_t freq, ptime &tstamp)
{
    auto state = std::make_shared<StateFile>();

    std::string query = "SELECT * FROM states WHERE timestamp>=";
    query += "\'" + to_simple_string(tstamp) + "\' ";
    query += " AND frequency=\'" + frequency_tags[freq] + "\'";
    query += " ORDER BY timestamp ASC LIMIT 1;";
    std::cout << "QUERY: " << query << std::endl;
    worker = new pqxx::work(*db);
    pqxx::result result = worker->exec(query);
    worker->commit();
    if (result.size() > 0) {
        state->timestamp = time_from_string(pqxx::to_string(result[0][0]));
        state->path = pqxx::to_string(result[0][1]);
        state->sequence = result[0][2].as(int(0));
        state->frequency =  pqxx::to_string(result[0][3]);
    }
    return state;
}

// Scan remote directory from planet
std::shared_ptr<std::vector<std::string>>
Planet::scanDirectory(const std::string &dir)
{
    std::cout << "Scanning remote Directory: " << dir << std::endl;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    ssl::stream<tcp::socket> stream{ioc, ctx};

    // Look up the domain name
    auto const results = resolver.resolve(server, std::to_string(port));

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    auto links =  std::make_shared<std::vector<std::string>>();

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, dir, version };
    req.set(http::field::host, server);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;

    // Receive the HTTP response
    boost::beast::error_code ec;
    http::response_parser<http::string_body> parser;
    // read_header(stream, buffer, parser);
    read(stream, buffer, parser);
    if (parser.get().result() == boost::beast::http::status::not_found) {
        return links;
    }
    GumboOutput* output = gumbo_parse(parser.get().body().c_str());
    getLinks(output->root, links);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    stream.shutdown();
    return links;
}

// Find the path to download the right file
std::string
Planet::findData(frequency_t freq, ptime starttime)
{
    boost::posix_time::time_duration delta;
    // Look for the top level directory
    if (freq == minutely) {
        for (auto it = std::rbegin(minute); it != std::rend(minute); ++it) {
            delta = it->first - starttime;
            // delta = starttime - it->first;
            std::cout << "Delta: " << delta.hours() << ":"
                      <<  delta.minutes()
                      << " : " << it->first
                      << " = " << it->second
                      << std::endl;
            // There are a thousand minutely updates in each low level directory
            // if (delta.hours() < 16600) {
            if (delta.minutes() < 0) {
                std::cout << "Found minutely top path " << it->second << " for "
                          << starttime << std::endl;
                // delta = it->first + delta; 
                ptime diff = it->first - boost::posix_time::hours(delta.hours())
                                       - boost::posix_time::minutes(delta.minutes());
                std::cout << "Found minutely full path " << diff
                          << " for " << " start: "
                          << starttime << std::endl;
                return it->second;
            }
        }
    } else if (freq == hourly) {
        for (auto it = std::begin(hour); it != std::end(hour); ++it) {
            delta = starttime - it->first;
            if (delta.hours() <= 1) {
                std::cout << "Found hourly path " << it->second << " for "
                          << starttime << std::endl;
                return it->second;
            }
        }
    } else if (freq == daily) {
        for (auto it = std::begin(day); it != std::end(day); ++it) {
            delta =  starttime - it->first;
            if (delta.hours() <= 24) {
                std::cout << "Found daily path " << it->second << " for "
                          << starttime << std::endl;
                return it->second;
            }
        }
    }
}

} // EOF replication namespace

