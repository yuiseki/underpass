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
#include <iostream>
#include <mutex>
#include <range/v3/all.hpp>
#include<algorithm>
#include<iterator>

// #include <boost/range/sub_range.hpp>
// #include <boost/range.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;   // from <boost/asio/ssl.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

#include "data/threads.hh"
#include "osmstats/osmstats.hh"
#include "osmstats/osmchange.hh"
#include "osmstats/changeset.hh"
#include "osmstats/replication.hh"

std::mutex stream_mutex;

namespace threads {


ThreadManager::ThreadManager(void)
{
    // Launch the pool with four threads.
    // boost::asio::thread_pool pool(numThreads());

    // Wait for all tasks in the pool to complete.
    // pool.join();
};

void
ThreadManager::startStateThreads(const std::string &base, std::vector<std::string> &files)
{
    // std::map<std::string, std::thread> thread_pool;
    auto planet = std::make_shared<replication::Planet>();
    //replication::Planet> planet;

    boost::system::error_code ec;
    // This lambda gets creates inline code for each thread in the pool
    auto state = [planet](const std::string &path)->bool {
        std::shared_ptr<replication::StateFile> state = threadStateFile(planet->stream,
                                                                        path + ".state.txt");
        if (!state->path.empty()) {
            planet->writeState(*state);
            state->dump();
            return true;
        } else {
            std::cerr << "ERROR: No StateFile returned: " << path << std::endl;
            // planet.reset(new replication::Planet);
            // planet.reset(new replication::Planet());

            // planet.connectServer();
            // std::this_thread::sleep_for(std::chrono::seconds{1});
            // state = threadStateFile(planet.stream, path + ".state.txt");
            // if (!state->path.empty()) {
            //     planet.writeState(*state);
            //     state->dump();
            //     return true;
            // }
            return false;
        }
    };

    // boost::asio::thread_pool pool(20);
    boost::asio::thread_pool pool(numThreads());

    // Note this uses ranges, which only got added in C++20, so
    // for now use the ranges-v3 library, which is the implementation.
    // The planet server drops the network connection after 111
    // GET requests, so break the 1000 strings into smaller chunks
    // 144, 160, 176, 192, 208, 224
    auto rng  = files | ranges::views::chunk(200);

    Timer timer;
    timer.startTimer();
    int counter = 0;
    std::vector<std::string> foo;
    for (auto cit = std::begin(rng); cit != std::end(rng); ++cit) {
        //std::copy(std::begin(*cit), std::end(*cit), std::back_inserter(foo));
        planet->startTimer();
        // std::cout << "Chunk data: " << *cit << std::endl;
        for (auto it = std::begin(*cit); it != std::end(*cit); ++it) {
            if (boost::filesystem::extension(*it) != ".txt") {
                continue;
            }
            std::string subpath = base + it->substr(0, it->size() - 10);
            std::shared_ptr<replication::StateFile> exists = planet->getState(subpath);
            if (!exists->path.empty()) {
                std::cout << "Already stored: " << subpath << std::endl;
                continue;
            }
            // Add a thread to the pool for this file
            if (!it->size() <= 1) {
#ifdef USE_MULTI_LOADER
                boost::asio::post(pool, [subpath, state]{state(subpath);});
#else
                std::shared_ptr<replication::StateFile> state = threadStateFile(planet->stream,
                                                                base + *it);
                if (!state->path.empty()) {
                    planet->writeState(*state);
                    state->dump();
                    continue;
                }
#endif
            }
        }

        // Wait for all the threads to finish before shutting down the socket
        // pool.join();
        // try {
        //     planet->db->close();
        //     planet->stream.shutdown();
        //     planet->ioc.reset();
        // } catch (const std::exception &e) {
        //     std::cerr << "Couldn't shutdown stream" << e.what() << std::endl;
        // }
        // Don't hit the server too hard while testing, it's not polite
        planet->endTimer("chunk ");
        std::this_thread::sleep_for(std::chrono::seconds{1});
        planet.reset(new replication::Planet);
    }
    pool.join();
    timer.endTimer("directory ");
}

void
threadOsmChange(const std::string &database, ptime &timestamp)
{
    ptime now = boost::posix_time::second_clock::local_time();
    osmstats::QueryOSMStats ostats;
    replication::Replication repl;
    replication::Planet planet;
#if 0
    auto data = std::make_shared<std::vector<unsigned char>>();
    std::string dir = planet.findData(replication::changeset, timestamp);
    data = replicator.downloadFiles("https://planet.openstreetmap.org/" + dir + ".state.txt", true);
    if (data->empty()) {
        std::cout << "OsmChange File not found" << std::endl;
        exit(-1);
    }
    std::string tmp(reinterpret_cast<const char *>(data->data()));
    changeset::ChangeSetFile change;
    change.readXML();
    state.dump();
    if (state.timestamp >= timestamp) {
        data = replicator.downloadFiles("https://planet.openstreetmap.org/" + dir + ".state.txt", true);
    }
#endif
}

// This updates several fields in the raw_changesets table, which are part of
// the changeset file, and don't need to be calculated.
void
threadChangeSet(const std::string &database, ptime &timestamp)
{
    osmstats::QueryOSMStats ostats;
    replication::Replication repl;
}

// This updates the calculated fields in the raw_changesets table, based on
// the data in the OSM stats database.
void
threadStatistics(const std::string &database, ptime &timestamp)
{
    osmstats::QueryOSMStats ostats;
    replication::Replication repl;
}

/// Updates the states table in the Underpass database
std::shared_ptr<replication::StateFile>
threadStateFile(ssl::stream<tcp::socket> &stream, const std::string &file)
{
    std::string server = "planet.openstreetmap.org";
    // See if the data exists in the databse already
    // std::shared_ptr<replication::StateFile> exists = planet.getState(subpath);    

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;
    boost::beast::error_code ec;

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, file, 11};

    req.keep_alive();
    req.set(http::field::host, server);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    
    // std::cout << "(" << std::this_thread::get_id() << ")Downloading " << file << std::endl;

    // Stays locked till the function exits
    const std::lock_guard<std::mutex> lock(stream_mutex);

    // Send the HTTP request to the remote host
    // std::lock_guard<std::mutex> guard(stream_mutex);
    boost::beast::http::response_parser<http::string_body> parser;
    std::cout << "Downloading: " << file << std::endl;

    http::write(stream, req);
    boost::beast::http::read(stream, buffer, parser, ec);
    if (ec == http::error::partial_message) {
        std::cerr << "ERROR: partial read" << ": " << ec.message() << std::endl;
        // Give the network a chance to recover
        std::this_thread::yield();
        // std::this_thread::sleep_for(std::chrono::seconds{1});
        //return std::make_shared<replication::StateFile>();
    }
    if (ec == http::error::end_of_stream) {
        std::cerr << "ERROR: end of stream read failed" << ": " << ec.message() << std::endl;
        // Give the network a chance to recover
        // stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return std::make_shared<replication::StateFile>();
    } else if (ec) {
        std::cerr << "ERROR: stream read failed" << ": " << ec.message() << std::endl;
        return std::make_shared<replication::StateFile>();
    }
    if (parser.get().result() == boost::beast::http::status::not_found) {
        // continue;
    }

    // File never downloaded, return empty
    if (parser.get().body().size() < 10) {
        std::cerr << "ERROR: failed to download:  " << ": " << file << std::endl;
        return std::make_shared<replication::StateFile>();
    }

    //const std::lock_guard<std::mutex> unlock(stream_mutex);
    auto data = std::make_shared<std::vector<unsigned char>>();
    for (auto body = std::begin(parser.get().body()); body != std::end(parser.get().body()); ++body) {
        data->push_back((unsigned char)*body);
    }
    if (data->size() == 0) {
        std::cout << "StateFile not found: " << file << std::endl;
        return std::make_shared<replication::StateFile>();
    } else {
        std::string tmp(reinterpret_cast<const char *>(data->data()));
        auto state = std::make_shared<replication::StateFile>(tmp, true);
        state->path = file.substr(0, file.size() - 10);
        return state;
    }
}

}       // EOF namespace threads

