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
#include <map>
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

    static const std::string polyTable;
    static const std::string lineTable;

    /// Build query for processed Node
    std::string applyChange(const OsmNode &node) const;
    /// Build query for processed Way
    std::string applyChange(const OsmWay &way) const;
    /// Build query for processed Relation
    std::string applyChange(const OsmRelation &relation) const;
    /// Get nodes for filling Node cache
    void getNodeCache(std::shared_ptr<OsmChangeFile> osmchanges, const multipolygon_t &poly);
    /// Get nodes for filling Node cache from ways refs
    void getNodeCacheFromWays(std::shared_ptr<std::vector<OsmWay>> ways, std::map<double, point_t> &nodecache) const;
    // Get ways by refs
    std::list<std::shared_ptr<OsmWay>> getWaysByNodesRefs(std::string &nodeIds) const;

    // DB connection
    std::shared_ptr<Pq> dbconn;
    // Get ways count
    int getWaysCount(const std::string &tableName);
    // Build tags query
    std::string buildTagsQuery(std::map<std::string, std::string> tags) const;
    // Get ways by page
    std::shared_ptr<std::vector<OsmWay>> getWaysFromDB(long lastid, int pageSize, const std::string &tableName);
    std::shared_ptr<std::vector<OsmWay>> getWaysFromDBWithoutRefs(long lastid, int pageSize, const std::string &tableName);

};

} // namespace queryraw

#endif // EOF __QUERYRAW_HH__

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
