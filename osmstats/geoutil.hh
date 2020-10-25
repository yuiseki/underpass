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

#ifndef __GEOUTIL_HH__
#define __GEOUTIL_HH__

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

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include "hotosm.hh"
#include "osmstats/osmstats.hh"
#include "ogrsf_frmts.h"

namespace geoutil {

typedef boost::geometry::model::d2::point_xy<double> point_t;
typedef boost::geometry::model::polygon<point_t> polygon_t;

class GeoCountry
{
public:
    GeoCountry(void);
    GeoCountry(std::string name, std::string iso_a2, std::string iso_a3,
               std::string region, std::string subregion,
               boost::geometry::model::polygon<point_t>);
    void addTag(const std::string name);
    void addBoundary(boost::geometry::model::polygon<point_t> boundary);
private:
    // Default name
    std::string name;
    // International names
    std::vector<std::string> names;
    // 2 letter ISO abbreviation
    std::string iso_a2;
    // 3 letter ISO abbreviation
    std::string iso_a3;
    std::string region;
    std::string subregion;
    // The boundary
    boost::geometry::model::polygon<point_t> boundary;
};

class GeoUtil
{
public:
    GeoUtil(void) {
        GDALAllRegister();
    };
    /// Read a file into internal storage so boost::geometry functions
    /// can be used to process simple geospatial calculations instead
    /// of using postgres. This data is is used to determine which
    /// country a change was made in, or filtering out part of the
    /// planet to reduce data size. Since this uses GDAL, any
    /// multi-polygon file of any supported format can be used.
    bool readFile(const std::string &filespec, bool multi);
    
    /// Connect to the geodata database
    bool connect(const std::string &dbserver, const std::string &database);

    /// Find the country the changeset was made in
    osmstats::RawCountry findCountry(double lat, double lon); 

    /// See if this changeset is in a focus area. We ignore changsets in
    /// areas like North America to reduce the amount of data needed
    /// for calulations. This boundary can always be modified to be larger.
    bool focusArea(double lat, double lon);

    /// Dump internal data storage for debugging purposes
    void dump(void);
private:
    std::string dbserver;
    std::string database;
    pqxx::connection *db;
    pqxx::work *worker;
    /// This is a polygon boundary to use to only apply changesets
    /// within it.
    boost::geometry::model::polygon<point_t> boundary;
    boost::geometry::model::multi_polygon<polygon_t> countries;
};
    
}       // EOF geoutil

#endif  // EOF __GEOUTIL_HH__
