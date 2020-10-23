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
#include <array>
#include <memory>
#include <iostream>
#include <pqxx/pqxx>
#include <libxml++/libxml++.h>

// The Dump handler
#include <osmium/handler/dump.hpp>

// Allow any format of input files (XML, PBF, ...)
#include <osmium/io/any_input.hpp>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "hotosm.hh"
#include "osmstats/changefile.hh"

namespace changefile {

// ChangeFile::ChangeFile(void)
// {
//     std::ios_base::sync_with_stdio(false);
// }

// ChangeFile::~ChangeFile(void)
// {

// }

// bool
// ChangeFile::connect(std::string &database)
// {

// }
bool
ChangeFile::readFile(const std::string &osc)
{
    try {
        // Default is all entity types: nodes, ways, relations, and changesets
        osmium::osm_entity_bits::type read_types = osmium::osm_entity_bits::all;
        
        // Get entity types from command line if there is a 2nd argument.
        read_types = osmium::osm_entity_bits::nothing;
        std::string types = "fixme!!!!";
        if (types.find('n') != std::string::npos) {
            read_types |= osmium::osm_entity_bits::node;
        }
        if (types.find('w') != std::string::npos) {
            read_types |= osmium::osm_entity_bits::way;
        }
        if (types.find('r') != std::string::npos) {
            read_types |= osmium::osm_entity_bits::relation;
        }
        if (types.find('c') != std::string::npos) {
            read_types |= osmium::osm_entity_bits::changeset;
        }

        // Initialize Reader with file name and the types of entities we want to
        // read.
        osmium::io::Reader reader{"fixme!", read_types};
        
        // The file header can contain metadata such as the program that
        // generaed
            // the file and the bounding box of the data.
            osmium::io::Header header = reader.header();
        std::cout << "HEADER:\n  generator=" << header.get("generator") << "\n";
        
        for (const auto& bbox : header.boxes()) {
            std::cout << "  bbox=" << bbox << "\n";
        }

        // Initialize Dump handler.
        osmium::handler::Dump dump{std::cout};

        // Read from input and send everything to Dump handler.
       osmium::apply(reader, dump);

        // You do not have to close the Reader explicitly, but because the
        // destructor can't throw, you will not see any errors otherwise.
        reader.close();
    } catch (const std::exception& e) {
        // All exceptions used by the Osmium library derive from std::exception.
        std::cerr << e.what() << '\n';
        std::exit(1);
    }
}

}       // EOF changeset

