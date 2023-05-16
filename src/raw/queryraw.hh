//
// Copyright (c) 2023 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Underpass is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Underpass is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __QUERYRAW_HH__
#define __QUERYRAW_HH__

/// \file queryraw.hh
/// \brief This build raw queries for the database
///
/// This manages the OSM Raw schema in a postgres database. This
/// includes building queries for existing data in the database, 
/// as well for updating the database.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <iostream>
#include "data/pq.hh"
#include "osm/osmobjects.hh"
#include "osm/osmchange.hh"

using namespace pq;
using namespace osmobjects;
using namespace osmchange;

/// \namespace queryraw
namespace queryraw {

/// \class QueryStats
/// \brief This handles all direct database access
///
/// This class handles all the queries to the OSM Stats database.
/// This includes querying the database for existing data, as
/// well as updating the data whenh applying a replication file.
class QueryRaw {
  public:
    QueryRaw(void);
    ~QueryRaw(void){};
    QueryRaw(std::shared_ptr<Pq> db);

    /// Build query for processed Node
    std::string applyChange(const OsmNode &node) const;
    /// Build query for processed Way
    std::string applyChange(const OsmWay &way) const;
    /// Build query for updating modified nodes in ways geometries
    std::string applyChange(const std::shared_ptr<std::map<long, std::pair<double, double>>> nodes) const;
    /// Get nodes for filling Node cache
    void getNodeCache(std::shared_ptr<OsmChangeFile> osmchanges);
    /// Get nodes for filling Node cache from ways refs
    void getNodeCacheFromWays(std::shared_ptr<std::vector<OsmWay>> ways, std::map<double, point_t> &nodecache) const;
    // Get ways by refs
    std::shared_ptr<std::vector<OsmWay>> getWaysByNodesRefs(const std::shared_ptr<std::map<long, std::pair<double, double>>> nodes) const;
    // Database connection
    std::shared_ptr<Pq> dbconn;
    // Get ways count
    int getWaysCount();
    // Get ways by page
    std::shared_ptr<std::vector<OsmWay>> getWaysFromDB(int page);
    // Get single way by id
    std::shared_ptr<OsmWay> getWayById(long id);

};

} // namespace queryraw

#endif // EOF __QUERYRAW_HH__

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End: