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

/// \file queryraw.cc
/// \brief This file is used to work with the OSM Raw database
///
/// This manages the OSM Raw schema in a postgres database. This
/// includes querying existing data in the database, as well as
/// updating the database.

// TODO: add support for relations/multipolygon

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <iostream>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include "utils/log.hh"
#include "data/pq.hh"
#include "raw/queryraw.hh"
#include "osm/osmobjects.hh"
#include "osm/osmchange.hh"

#include <boost/timer/timer.hpp>

using namespace pq;
using namespace logger;
using namespace osmobjects;
using namespace osmchange;

/// \namespace queryraw
namespace queryraw {

QueryRaw::QueryRaw(void) {}

QueryRaw::QueryRaw(std::shared_ptr<Pq> db) {
    dbconn = db;
}

std::string
QueryRaw::applyChange(const OsmNode &node) const
{
    std::string query;
    if (node.action == osmobjects::create || node.action == osmobjects::modify) {
        query = "INSERT INTO nodes as r (osm_id, geom, tags, timestamp, version, \"user\", uid, changeset) VALUES(";
        std::string format = "%d, ST_GeomFromText(\'%s\', 4326), %s, \'%s\', %d, \'%s\', %d, %d \
        ) ON CONFLICT (osm_id) DO UPDATE SET  geom = ST_GeomFromText(\'%s\', \
        4326), tags = %s, timestamp = \'%s\', version = %d, \"user\" = \'%s\', uid = %d, changeset = %d WHERE r.version < %d;";
        boost::format fmt(format);

        // osm_id
        fmt % node.id;

        // geometry
        std::stringstream ss;
        ss << std::setprecision(12) << boost::geometry::wkt(node.point);
        std::string geometry = ss.str();
        fmt % geometry;

        // tags
        std::string tags = "";
        if (node.tags.size() > 0) {
            for (auto it = std::begin(node.tags); it != std::end(node.tags); ++it) {
                std::string tag_format = "\"%s\" : \"%s\",";
                boost::format tag_fmt(tag_format);
                tag_fmt % dbconn->escapedString(it->first);
                tag_fmt % dbconn->escapedString(it->second);
                tags += tag_fmt.str();
            }
            tags.erase(tags.size() - 1);
            tags = "'{" + tags + "}'";
        } else {
            tags = "null";
        }
        fmt % tags;
        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % node.version;
        // user
        fmt % dbconn->escapedString(node.user);
        // uid
        fmt % node.uid;
        // changeset
        fmt % node.changeset;

        // ON CONFLICT
        fmt % geometry;
        fmt % tags;
        fmt % timestamp;
        fmt % node.version;
        fmt % dbconn->escapedString(node.user);
        fmt % node.uid;
        fmt % node.changeset;
        fmt % node.version;

        query += fmt.str();

    } else if (node.action == osmobjects::remove) {
        query = "DELETE from nodes where osm_id = " + std::to_string(node.id) + ";";
    }

    return query;
}


const std::string QueryRaw::polyTable = "ways_poly";
const std::string QueryRaw::lineTable = "ways_line";

std::string
QueryRaw::applyChange(const OsmWay &way) const
{
    std::string query = "";
    const std::string* tableName;

    std::stringstream ss;
    if (way.refs.size() > 3 && (way.refs.front() == way.refs.back())) {
        tableName = &QueryRaw::polyTable;
        ss << std::setprecision(12) << boost::geometry::wkt(way.polygon);
    } else {
        tableName = &QueryRaw::lineTable;
        ss << std::setprecision(12) << boost::geometry::wkt(way.linestring);
    }
    std::string geostring = ss.str();
    
    if (way.refs.size() > 2
        && (way.action == osmobjects::create || way.action == osmobjects::modify)) {
        if (way.refs.size() == boost::geometry::num_points(way.linestring)) {

            query = "INSERT INTO " + *tableName + " as r (osm_id, tags, refs, geom, timestamp, version, \"user\", uid, changeset) VALUES(";
            std::string format = "%d, %s, %s, %s, \'%s\', %d, \'%s\', %d, %d) \
            ON CONFLICT (osm_id) DO UPDATE SET tags = %s, refs = %s, geom = %s, timestamp = \'%s\', version = %d, \"user\" = \'%s\', uid = %d, changeset = %d WHERE r.version <= %d;";
            boost::format fmt(format);

            // osm_id
            fmt % way.id;

            // tags
            std::string tags = "";
            if (way.tags.size() > 0) {
                for (auto it = std::begin(way.tags); it != std::end(way.tags); ++it) {
                    std::string tag_format = "\"%s\" : \"%s\",";
                    boost::format tag_fmt(tag_format);
                    tag_fmt % dbconn->escapedString(it->first);
                    tag_fmt % dbconn->escapedString(it->second);
                    tags += tag_fmt.str();
                }
                tags.erase(tags.size() - 1);
                tags = "'{" + tags + "}'";
            } else {
                tags = "null";
            }

            fmt % tags;

            // refs
            std::string refs = "";
            for (auto it = std::begin(way.refs); it != std::end(way.refs); ++it) {
                refs += std::to_string(*it) + ",";
            }
            refs.erase(refs.size() - 1);
            refs = "ARRAY[" + refs + "]";
            fmt % refs;

            // geometry
            std::string geometry;
            geometry = "ST_GeomFromText(\'" + geostring + "\', 4326)";
            fmt % geometry;

            // timestamp
            std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
            fmt % timestamp;
            // version
            fmt % way.version;
            // user
            fmt % dbconn->escapedString(way.user);
            // uid
            fmt % way.uid;
            // changeset
            fmt % way.changeset;

            // ON CONFLICT
            fmt % tags;
            fmt % refs;
            fmt % geometry;
            fmt % timestamp;
            fmt % way.version;
            fmt % dbconn->escapedString(way.user);
            fmt % way.uid;
            fmt % way.changeset;
            fmt % way.version;

            query += fmt.str();

            query += "DELETE FROM way_refs WHERE way_id=" + std::to_string(way.id) + ";";
            for (auto ref = way.refs.begin(); ref != way.refs.end(); ++ref) {
                query += "INSERT INTO way_refs (way_id, node_id) VALUES (" + std::to_string(way.id) + "," + std::to_string(*ref) + ");";
            }
        }
    } else if (way.action == osmobjects::remove) {
        query += "DELETE FROM way_refs WHERE way_id=" + std::to_string(way.id) + ";";
        query += "DELETE FROM " + QueryRaw::polyTable + " where osm_id = " + std::to_string(way.id) + ";";
        query += "DELETE FROM " + QueryRaw::lineTable + " where osm_id = " + std::to_string(way.id) + ";";
    }

    return query;
}

std::vector<long> arrayStrToVector(std::string &refs_str) {
    refs_str.erase(0, 1);
    refs_str.erase(refs_str.size() - 1);
    std::vector<long> refs;
    std::stringstream ss(refs_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        refs.push_back(std::stod(token));
    }
    return refs;
}

void QueryRaw::getNodeCache(std::shared_ptr<OsmChangeFile> osmchanges, const multipolygon_t &poly)
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getNodeCache(osmchanges): took %w seconds\n");
#endif
    std::string referencedNodeIds;
    std::string modifiedNodesIds;
    std::vector<long> removedWays;

    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto wit = std::begin(change->ways); wit != std::end(change->ways); ++wit) {
            OsmWay *way = wit->get();
            if (way->action != osmobjects::remove) {
                // Save referenced nodes for later use
                for (auto rit = std::begin(way->refs); rit != std::end(way->refs); ++rit) {
                    if (way->action != osmobjects::remove) {
                        if (!osmchanges->nodecache.count(*rit)) {
                            referencedNodeIds += std::to_string(*rit) + ",";
                        }
                    }
                }
            } else {
                removedWays.push_back(way->id);
            }
        }
        // Save modified nodes for later use
        for (auto nit = std::begin(change->nodes); nit != std::end(change->nodes); ++nit) {
            OsmNode *node = nit->get();
            if (node->action == osmobjects::modify) {
                // Get only modified nodes inside priority area
                if (boost::geometry::within(node->point, poly)) {
                    modifiedNodesIds += std::to_string(node->id) + ",";
                }
            }
        }
    }

    // Add indirectly modified ways to osmchanges
    if (modifiedNodesIds.size() > 1) {
        modifiedNodesIds.erase(modifiedNodesIds.size() - 1);
        auto modifiedWays = getWaysByNodesRefs(modifiedNodesIds);
        auto change = std::make_shared<OsmChange>(none);
        for (auto wit = modifiedWays.begin(); wit != modifiedWays.end(); ++wit) {
           auto way = std::make_shared<OsmWay>(*wit->get());
           // Save referenced nodes for later use
           for (auto rit = std::begin(way->refs); rit != std::end(way->refs); ++rit) {
               if (!osmchanges->nodecache.count(*rit)) {
                   referencedNodeIds += std::to_string(*rit) + ",";
               }
           }
           if (std::find(removedWays.begin(), removedWays.end(), way->id) == removedWays.end()) {
                way->action = osmobjects::modify;
                change->ways.push_back(way);
           }
        }
        osmchanges->changes.push_back(change);
    }

    // Fill nodecache with referenced nodes
    if (referencedNodeIds.size() > 1) {
        referencedNodeIds.erase(referencedNodeIds.size() - 1);
        // Get Nodes from DB
        std::string nodesQuery = "SELECT osm_id, st_x(geom) as lat, st_y(geom) as lon FROM nodes where osm_id in (" + referencedNodeIds + ");";
        auto result = dbconn->query(nodesQuery);
        // Fill nodecache
        for (auto node_it = result.begin(); node_it != result.end(); ++node_it) {
            auto node_id = (*node_it)[0].as<long>();
            auto node_lat = (*node_it)[2].as<double>();
            auto node_lon = (*node_it)[1].as<double>();
            OsmNode node(node_lat, node_lon);
            osmchanges->nodecache[node_id] = node.point;
        }
    }

    // Build ways geometries using nodecache
    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto wit = std::begin(change->ways); wit != std::end(change->ways); ++wit) {
            OsmWay *way = wit->get();
            way->linestring.clear();
            for (auto rit = way->refs.begin(); rit != way->refs.end(); ++rit) {
                if (osmchanges->nodecache.count(*rit)) {
                    boost::geometry::append(way->linestring, osmchanges->nodecache.at(*rit));
                }
            }
            if (way->isClosed()) {
                way->polygon = { {std::begin(way->linestring), std::end(way->linestring)} };
            }
        }
    }

}

void
QueryRaw::getNodeCacheFromWays(std::shared_ptr<std::vector<OsmWay>> ways, std::map<double, point_t> &nodecache) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getNodeCacheFromWays(ways, nodecache): took %w seconds\n");
#endif

    // Get all nodes ids referenced in ways
    std::string nodeIds;
    for (auto wit = ways->begin(); wit != ways->end(); ++wit) {
        for (auto rit = std::begin(wit->refs); rit != std::end(wit->refs); ++rit) {
            nodeIds += std::to_string(*rit) + ",";
        }
    }
    if (nodeIds.size() > 1) {

        nodeIds.erase(nodeIds.size() - 1);

        // Get Nodes from DB
        std::string nodesQuery = "SELECT osm_id, st_x(geom) as lat, st_y(geom) as lon FROM nodes where osm_id in (" + nodeIds + ") and st_x(geom) is not null and st_y(geom) is not null;";
        auto result = dbconn->query(nodesQuery);
        // Fill nodecache
        for (auto node_it = result.begin(); node_it != result.end(); ++node_it) {
            auto node_id = (*node_it)[0].as<long>();
            auto node_lat = (*node_it)[1].as<double>();
            auto node_lon = (*node_it)[2].as<double>();
            auto point = point_t(node_lat, node_lon);
            nodecache.insert(std::make_pair(node_id, point));
        }
    }
}

std::map<std::string, std::string> parseTagsString(const std::string& input) {
    std::map<std::string, std::string> result;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Find the position of the arrow
        size_t arrowPos = token.find(":");
        if (arrowPos != std::string::npos) {
            std::string key = token.substr(1, arrowPos - 1);
            std::string value = token.substr(arrowPos + 2);
            key.erase(std::remove( key.begin(), key.end(), '\"' ),key.end());
            value.erase(std::remove( value.begin(), value.end(), '\"' ),value.end());
            result[key] = value;
        }
    }
    return result;
}

std::list<std::shared_ptr<OsmWay>>
QueryRaw::getWaysByNodesRefs(std::string &nodeIds) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getWaysByNodesRefs(nodeIds): took %w seconds\n");
#endif
    // Get all ways that have references to nodes
    std::list<std::shared_ptr<osmobjects::OsmWay>> ways;

    std::string waysQuery = "SELECT distinct(osm_id), refs, version, tags from way_refs join ways_poly wp on wp.osm_id = way_id where node_id = any(ARRAY[" + nodeIds + "])";
    waysQuery += " UNION SELECT distinct(osm_id), refs, version, tags from way_refs join ways_line wl on wl.osm_id = way_id where node_id = any(ARRAY[" + nodeIds + "]);";
    auto ways_result = dbconn->query(waysQuery);

    // Fill vector of OsmWay objects
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        auto way = std::make_shared<OsmWay>();
        way->id = (*way_it)[0].as<long>();
        std::string refs_str = (*way_it)[1].as<std::string>();
        if (refs_str.size() > 1) {
            way->refs = arrayStrToVector(refs_str);
        }
        way->version = (*way_it)[2].as<long>();
        auto tags = (*way_it)[3];
        if (!tags.is_null()) {
            auto tags = parseTagsString((*way_it)[3].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                way->addTag(key, val);
            }
        }
        ways.push_back(way);
    }
    return ways;
}

int QueryRaw::getWaysCount(const std::string &tableName) {
    std::string query = "select count(osm_id) from " + tableName;
    auto result = dbconn->query(query);
    return result[0][0].as<int>();
}

std::shared_ptr<std::vector<OsmWay>>
QueryRaw::getWaysFromDB(int lastid, const std::string &tableName) {
    std::string waysQuery;
    if (tableName == QueryRaw::polyTable) {
        waysQuery = "SELECT osm_id, refs, ST_AsText(ST_ExteriorRing(geom), 4326)";
    } else {
        waysQuery = "SELECT osm_id, refs, ST_AsText(geom, 4326)";
    }
    waysQuery += ", version, tags FROM " + tableName + " where osm_id > " + std::to_string(lastid) + " order by osm_id asc limit 500;";

    auto ways_result = dbconn->query(waysQuery);
    // Fill vector of OsmWay objects
    auto ways = std::make_shared<std::vector<OsmWay>>();
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        OsmWay way;
        way.id = (*way_it)[0].as<long>();
        std::string refs_str = (*way_it)[1].as<std::string>();
        if (refs_str.size() > 1) {
            way.refs = arrayStrToVector(refs_str);

            std::string poly = (*way_it)[2].as<std::string>();
            boost::geometry::read_wkt(poly, way.linestring);

            if (tableName == QueryRaw::polyTable) {
                way.polygon = { {std::begin(way.linestring), std::end(way.linestring)} };
            }
            way.version = (*way_it)[3].as<long>();
            auto tags = (*way_it)[4];
            if (!tags.is_null()) {
                auto tags = parseTagsString((*way_it)[4].as<std::string>());
                for (auto const& [key, val] : tags)
                {
                    way.addTag(key, val);
                }
            }
            ways->push_back(way);
        }
    }

    return ways;
}

std::shared_ptr<std::vector<OsmWay>>
QueryRaw::getWaysFromDBWithoutRefs(int lastid, const std::string &tableName) {
    std::string waysQuery;
    if (tableName == QueryRaw::polyTable) {
        waysQuery = "SELECT osm_id, ST_AsText(ST_ExteriorRing(geom), 4326)";
    } else {
        waysQuery = "SELECT osm_id, ST_AsText(geom, 4326)";
    }
    waysQuery += ", version, tags FROM " + tableName + " where osm_id > " + std::to_string(lastid) + " order by osm_id asc limit 500;";
    
    auto ways_result = dbconn->query(waysQuery);
    // Fill vector of OsmWay objects
    auto ways = std::make_shared<std::vector<OsmWay>>();
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        OsmWay way;
        way.id = (*way_it)[0].as<long>();

        std::string poly = (*way_it)[1].as<std::string>();
        boost::geometry::read_wkt(poly, way.linestring);

        if (tableName == QueryRaw::polyTable) {
            way.polygon = { {std::begin(way.linestring), std::end(way.linestring)} };
        }
        way.version = (*way_it)[2].as<long>();
        auto tags = (*way_it)[3];
        if (!tags.is_null()) {
            auto tags = parseTagsString((*way_it)[3].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                way.addTag(key, val);
            }
        }
        ways->push_back(way);

    }

    return ways;
}

} // namespace queryraw

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
