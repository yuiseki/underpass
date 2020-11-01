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

#ifndef __IMPORT_HH__
#define __IMPORT_HH__

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

#include <osmium/io/any_input.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/io/any_output.hpp>
#include <glibmm/convert.h>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;


namespace import {

class OSMHandler : public osmium::handler::Handler {
public:
    void way(const osmium::Way& way) {
        std::cout << "way " << way.id()
                  << ", Changeset: " << way.changeset()
                  << ", Version: " << way.version()
                  << ", UID: " << way.uid()
                  << ", User: " << way.user()
                  << ", Timestamp: " << way.timestamp() << std::endl;
        for (const osmium::Tag& t : way.tags()) {
            std::cout << "\t" << t.key() << "=" << t.value() << std::endl;
        }
    }

    void node(const osmium::Node& node) {
        std::cout << "node " << node.id()
                  << ", Changeset: " << node.changeset()
                  << ", Version: " << node.version()
                  << ", UID: " << node.uid()
                  << ", User: " << node.user()
                  << ", Timestamp: " << node.timestamp() << std::endl;
        for (const osmium::Tag& t : node.tags()) {
            std::cout << "\t" << t.key() << "=" << t.value() << std::endl;
        }
    }
    void relation(const osmium::Relation& rel) {
        std::cout << "rel " << rel.id() << std::endl;
        for (const osmium::Tag& t : rel.tags()) {
            std::cout << "\t" << t.key() << "=" << t.value() << std::endl;
        }
    }
};

class ImportOSM
{
public:
    /// Import a raw OSM file using libosmium
    ImportOSM(const std::string &file, const std::string &db) {
        osmium::io::Reader reader{file};
        database = db;
        osmium::apply(reader, handler);
    };

    // ~ImportOSM(void) { reader.close(); };

private:
    std::string database;
    OSMHandler handler;
};


} // EOF import namespace

#endif  // EOF __IMPORT_HH__
