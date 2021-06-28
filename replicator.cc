//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
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
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <pqxx/pqxx>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <cassert>
#include <deque>
#include <list>
#include <thread>
#include <tuple>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace boost;
namespace opts = boost::program_options;

// #include "hotosm.hh"
#include "osmstats/changeset.hh"
// #include "osmstats/osmstats.hh"
// #include "data/geoutil.hh"
// #include "osmstats/replication.hh"
#include "data/import.hh"
#include "data/threads.hh"
#include "data/underpass.hh"
#include "timer.hh"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

enum pathMatches{ROOT, DIRECTORY, SUBDIRECTORY, FILEPATH};

// Forward declarations
namespace changeset {
  class ChangeSet;
};

/// A helper function to simplify the main part.
template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    copy(v.begin(), v.end(), std::ostream_iterator<T>(os, " "));
    return os;
}

/// \class Replicator
/// \brief This class does all the actual work
///
/// This class identifies, downloads, and processes a replication file.
/// Replication files are available from the OSM planet server.
class Replicator : public replication::Replication
{
public:
    /// Create a new instance, and read in the geoboundaries file.
    Replicator(void) {
        auto hashes = std::make_shared<std::map<std::string, int>>();
#if 0
        auto geou = std::make_shared<geoutil::GeoUtil>();
        // FIXME: this shouldn't be hardcoded
        geou->readFile("../underpass.git/data/geoboundaries.osm", true);
        changes = std::make_shared<changeset::ChangeSetFile>();
        changes->setupBoundaries(geou);

        // FIXME: should return a real value
        return false;
        baseurl = "https://planet.openstreetmap.org/replication/";
#endif
    };
    
    /// Initialize the raw_user, raw_hashtags, and raw_changeset tables
    /// in the OSM stats database from a changeset file
    bool initializeRaw(std::vector<std::string> &rawfile, const std::string &database) {
        for (auto it = std::begin(rawfile); it != std::end(rawfile); ++it) {
            changes->importChanges(*it);
        }
        // FIXME: return a real value
        return false;
    };


#if 0
    std::string getLastPath(replication::frequency_t interval) {
        ptime last = ostats.getLastUpdate();
        underpass::Underpass under;
        auto state = under.getState(interval, last);
        return state->path;
    };
#endif
    // osmstats::RawCountry & findCountry() {
    //     geou.inCountry();


    enum pathMatches matchUrl(const std::string &url) {
        boost::regex test{"([0-9]{3})"};

        boost::sregex_token_iterator iter(url.begin(), url.end(), test, 0);
        boost::sregex_token_iterator end;

        std::vector<std::string> dirs;

        for( ; iter != end; ++iter ) {
            dirs.push_back(*iter);
        }

        pathMatches match;
        switch(dirs.size()) {
            default:
                match = pathMatches::ROOT;
                break;
            case 1:
                match = pathMatches::DIRECTORY;
                break;
            case 2:
                match = pathMatches::SUBDIRECTORY;
                break;
            case 3:
                match = pathMatches::FILEPATH;
                break;
        }

        return match;
    }

//    std::string baseurl;                                ///< base URL for the planet server
private:
    std::shared_ptr<changeset::ChangeSetFile> changes;  ///< All the changes in the file
    std::shared_ptr<std::map<std::string, int>> hashes; ///< Existing hashtags
};

int
main(int argc, char *argv[])
{
    // Store the file names for replication files
    std::string changeset;
    std::string osmchange;
    // The update frequency between downloads
    //bool encrypted = false;
    long sequence = 0;
    ptime starttime(not_a_date_time);
    ptime endtime(not_a_date_time);
    std::string url;
    // std::string pserver = "https://download.openstreetmap.fr";
    // std::string pserver = "https://planet.openstreetmap.org";
    std::string pserver = "https://planet.maps.mail.ru";
    std::string datadir = "replication/";
    std::string boundary = "priority.geojson";
    replication::frequency_t frequency = replication::minutely;

    opts::positional_options_description p;
    opts::variables_map vm;
     try {
        opts::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "display help")
            ("server,s", "database server (defaults to localhost)")
            ("planet,p", "replication server (defaults to planet.openstreetmap.org)")
            ("url,u", opts::value<std::string>(), "Starting URL (ex. 000/075/000)")
            ("monitor,m", "Starting monitor")
            ("frequency,f", opts::value<std::string>(), "Update frequency (hour, daily), default minute)")
            ("timestamp,t", opts::value<std::vector<std::string>>(), "Starting timestamp")
            ("import,i", opts::value<std::string>(), "Initialize OSM database with datafile")
            ("changefile,c", opts::value<std::string>(), "Import change file")
            ("boundary,b", opts::value<std::string>(), "Boundary polygon file name")
            ("datadir,d", opts::value<std::string>(), "Base directory for cached files")
//            ("verbose,v", "enable verbosity")
            ;
        
        opts::store(opts::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        opts::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: options_description [options]" << std::endl;
            std::cout << desc;
            return 0;
        }
     } catch(std::exception& e) {
         std::cout << e.what() << std::endl;
         return 1;
     }

     if (vm.count("boundary")) {
         boundary = vm["boundary"].as<std::string>();
     }

     auto geou = std::make_shared<geoutil::GeoUtil>();
     std::string priority = SRCDIR;
     priority += "/data/" + boundary;
     if (boost::filesystem::exists(priority)) {
         geou->readFile(datadir + boundary);
     } else {
         priority = PKGLIBDIR;
         priority += "/" + boundary;
         geou->readFile(priority);
     }

     Replicator replicator;
     // replicator.initializeData();
     std::vector<std::string> rawfile;
     std::shared_ptr<std::vector<unsigned char>> data;

     // A changeset has the hashtags and comments we need. Multiple URLS
     // or filename may be specified on the command line, common when
     // catching up on changes.
     if (vm.count("changefile")) {
         std::string file = vm["changefile"].as<std::string>();
         std::cout << "Importing change file " << file << std::endl;
         replicator.readChanges(file);
     }

#if 0
     if (vm.count("osmchange")) {
         std::vector<std::string> files = vm["osmchange"].as<std::vector<std::string>>();
         if (files[0].substr(0, 4) == "http") {
             data = replicator.downloadFiles(files, false);
         } else {
             for (auto it = std::begin(files); it != std::end(files); ++it) {
                 replicator.readChanges(*it);
             }
         }
     }
     if (vm.count("sequence")) {
         std::cout << "Sequence is " << vm["sequence"].as<int>() << std::endl;
     }
#endif
     // This is a full URL to server with replication files
     if (vm.count("url")) {
         url = vm["url"].as<std::string>();
     }
     // This is the default data directory on that server
     if (vm.count("datadir")) {
         datadir = vm["datadir"].as<std::string>();
     }
     const char *tmp = std::getenv("DATADIR");
     if (tmp != 0) {
         datadir = tmp;
     }

     std::string strfreq = "minute";
     if (vm.count("frequency")) {
         strfreq = vm["frequency"].as<std::string>();
         if (strfreq[0] == 'm') {
             frequency = replication::minutely;
         } else if (strfreq[0] == 'h') {
             frequency = replication::hourly;
         } else if (strfreq[0] == 'd') {
             frequency = replication::daily;
         }
     }

     std::string fullurl = pserver + "/" + datadir + strfreq + "/" + url;
     replication::RemoteURL remote(fullurl);
//     remote.dump();
     // Specify a timestamp used by other options
     if (vm.count("timestamp")) {
         auto timestamps = vm["timestamp"].as<std::vector<std::string>>();
         if (timestamps[0] == "now") {
             starttime = boost::posix_time::microsec_clock::local_time();
         } else {
             starttime = time_from_string(timestamps[0]);
             if (timestamps.size() > 1) {
                 endtime = time_from_string(timestamps[1]);
             }
         }
     }

     osmstats::QueryOSMStats ostats;
     ostats.connect();
     underpass::Underpass under;
     under.connect();
     replication::Planet planet(remote);
     std::string last;
     std::string clast;
     if (vm.count("monitor")) {
         if (starttime.is_not_a_date_time() && url.size() == 0) {
             std::cerr << "ERROR: You need to supply either a timestamp or URL!" << std::endl;
//         if (timestamp.is_not_a_date_time()) {
//             timestamp = ostats.getLastUpdate();
//         }
             exit(-1);
         }
         std::map<replication::frequency_t, std::string> frequency_tags;
         frequency_tags[replication::minutely] = "minute";
         frequency_tags[replication::hourly] = "hour";
         frequency_tags[replication::daily] = "day";
         frequency_tags[replication::changeset] = "changeset";
         std::string path = frequency_tags[frequency] + "/";
         std::thread mthread;
         std::thread cthread;
         if (!url.empty()) {
             last = url;
             // remote.dump();
             mthread = std::thread(threads::startMonitor, std::ref(remote));
             auto state = under.getState(frequency, url);
             state->dump();
             if (state->path.empty()) {
                 auto tmp = planet.findData(frequency, last);
                 if (tmp->path.empty()) {
                     std::cerr << "ERROR: No last path!" << std::endl;
                     exit(-1);
                 }
             }
             auto state2 = under.getState(replication::changeset, state->timestamp);
             if (state2->path.empty()) {
                 std::cerr << "WARNING: No changeset path in database!" << std::endl;
                 auto tmp = planet.findData(replication::changeset, state->timestamp);
                 if (tmp->path.empty()) {
                     std::cerr << "ERROR: No changeset path!" << std::endl;
                     exit(-1);
                 }
             }
             state2->dump();
             clast = pserver + "/" + datadir + "changesets/" + state2->path;
             remote.parse(clast);
             // remote.dump();
             cthread = std::thread(threads::startMonitor, std::ref(remote));
         } else if (!starttime.is_not_a_date_time()) {
             // No URL, use the timestamp
             auto state = under.getState(frequency, starttime);
             if (state->path.empty()) {
                 auto tmp = planet.findData(frequency, starttime);
                 if (tmp->path.empty()) {
                     std::cerr << "ERROR: No last path!" << std::endl;
                     exit(-1);
                 } else {
                     // last = replicator.baseurl + under.freq_to_string(frequency) + "/" + tmp->path;
                 }
             } else {
                 // last = replicator.baseurl + under.freq_to_string(frequency) + "/" + state->path;
             }
             std::cout << "Last minutely is " << last  << std::endl;
             // mthread = std::thread(threads::startMonitor, std::ref(remote));
         }

         std::cout << "Waiting..." << std::endl;
         if (cthread.joinable()) {
             cthread.join();
         }
         if (mthread.joinable()) {
             mthread.join();
         }
         exit(0);
     }

#if 0
    if (vm.count("url")) {
        std::string url = vm["url"].as<std::string>();
        int match = replicator.matchUrl(url);

        // Trailing slash check.
        if (url.back() != '/' && match != pathMatches::FILEPATH) {
          url += "/";
        }
        auto links = planet.scanDirectory(url);

        Timer timer;
        switch (match) {
            default: // Root
            {
                for (auto it = std::begin(*links); it != std::end(*links); ++it) {
                    if (it->empty()) {
                        continue;
                    }

                    std::cout << *it << std::endl;
                    std::string subUrl = url + *it;
                    auto sublinks = planet.scanDirectory(subUrl);
                    std::thread tstate (threads::startStateThreads, std::ref(subUrl),
                                        std::ref(*sublinks));
                    std::cout << "Waiting..." << std::endl;
                    tstate.join();
                    timer.endTimer();
               }
            }
            break;
            case pathMatches::DIRECTORY:
            {
                for (auto sit = std::begin(*links); sit != std::end(*links); ++sit) {
                    if (sit->empty()) {
                        continue;
                    }
                    timer.startTimer();
                    std::string subdir = url + *sit;
                    
                    std::cout << "Sub Directory: " << subdir << std::endl;
                    auto flinks = planet.scanDirectory(subdir);
                    std::thread tstate (threads::startStateThreads, std::ref(subdir),
                                         std::ref(*flinks));

                    std::cout << "Waiting..." << std::endl;
                    tstate.join();
                    timer.endTimer();
                    continue;
                }
            }
            break;

            case pathMatches::SUBDIRECTORY:
            {
                std::thread tstate (threads::startStateThreads, std::ref(url),
                    std::ref(*links));
                std::cout << "Waiting for startStateThreads()..." << std::endl;
                tstate.join();
                timer.endTimer();
            }
            break;

            case pathMatches::FILEPATH:
                threads::startStateThreads(std::ref(url), std::ref(*links));
                timer.endTimer();
            break;
        }
    }
#endif
     std::string statistics;
     if (vm.count("initialize")) {
         rawfile = vm["initialize"].as<std::vector<std::string>>();
         replicator.initializeRaw(rawfile, statistics);
     }
     std::string osmdb;
     if (vm.count("osm")) {
         osmdb = vm["osm"].as<std::vector<std::string>>()[0];
     }
     if (vm.count("import")) {
         std::string file = vm["import"].as<std::string>();
         import::ImportOSM osm(file, osmdb);
     }
     // FIXME: add logging
     if (vm.count("verbose")) {
         std::cout << "Verbosity enabled.  Level is " << vm["verbose"].as<int>() << std::endl;
     }

     if (sequence > 0 and !starttime.is_not_a_date_time()) {
        std::cout << "ERROR: Can only specify a timestamp or a sequence" << std::endl;
        exit(1);
    }

    // replication::Replication rep(starttime, sequence);
    
    // std::vector<std::string> files;
    // files.push_back("004/139/998.state.txt");
    // std::shared_ptr<std::vector<std::string>> links = rep.downloadFiles(files, true);
    // std::cout << "Got "<< links->size() << " directories" << std::endl;
    // // links = rep.downloadFiles(*links, true);
    // // std::cout << "Got "<< links->size() << " directories" << std::endl;

    // changeset::StateFile foo("/tmp/foo1.txt", false);
    // changeset::StateFile bar("/tmp/foo2.txt", false);
    // return 0;
}

