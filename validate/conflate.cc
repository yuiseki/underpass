//
// Copyright (c) 2020, 2021, 2022 Humanitarian OpenStreetMap Team
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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <boost/geometry.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <vector>
#include <deque>
#include <cmath>
#include "data/pq.hh"
#include "validate/conflate.hh"
#include "validate/validate.hh"
#include "data/osmobjects.hh"
#include "log.hh"
using namespace logger;

namespace conflate {

Conflate::Conflate(void)
{
    std::string dburl = "osm2pgsql";
    conf_db.connect(dburl);
}

Conflate::Conflate(const multipolygon_t &poly)
{

}

bool
Conflate::createView(const std::string &wkt)
{
    std::string view = "DROP VIEW IF EXISTS boundary;CREATE VIEW boundary AS SELECT osm_id,area,building,highway,amenity,ST_Transform(way, 4326) AS way FROM planet_osm_polygon WHERE ST_Within(ST_Transform(way, 4326), ST_MakeValid(ST_GeomFromEWKT(\'";
    view += "SRID=4326;" + wkt + "\')));";
    pqxx::result result = conf_db.query(view);
    if (result.size() > 0) {
	return true;
    } else {
	return false;
    }
}

bool
Conflate::createView(const multipolygon_t &poly)
{
    view = poly;
    std::ostringstream bbox;
    bbox << boost::geometry::wkt(poly);
    // std::string view = "DROP VIEW IF EXISTS boundary;CREATE VIEW boundary AS SELECT osm_id,area,building,highway,amenity,ST_Transform(way, 4326) AS way FROM planet_osm_polygon WHERE ST_Within(ST_Transform(way, 4326), ST_MakeValid(ST_GeomFromEWKT(\'";
    // view += "SRID=4326;" + bbox.str() + "\')));";
    // pqxx::result result = conf_db.query(view);

    return createView(bbox.str());
}

Conflate::Conflate(const std::string &dburl, const multipolygon_t &poly)
{
    conf_db.connect(dburl);
    createView(poly);
}

Conflate::Conflate(const std::string &dburl)
{
    conf_db.connect(dburl);
}

bool
Conflate::connect(const std::string &dburl)
{
    return conf_db.connect(dburl);
}

std::shared_ptr<std::vector<std::shared_ptr<ValidateStatus>>>
Conflate::newDuplicatePolygon(const osmobjects::OsmWay &way)
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("Conflate::newDuplicatePolygon: took %w seconds\n");
#endif
    auto ids = std::make_shared<std::vector<std::shared_ptr<ValidateStatus>>>();
    if (!conf_db.isOpen()) {
	log_error(_("Database is not connected!"));
        return ids;
    }

    // Raw OSM data is in SRID 3857, but boost::geometry (and gdal) use 4326 instead. Unfortunately that
    // gives us coordinates in degress of the globe, which isn't useful to do area calculations with. So
    // it gets converted to SRID 2167, which forces it to be in meters.
    boost::format fmt("SELECT ST_Area(ST_Transform(ST_INTERSECTION(ST_GeomFromEWKT(\'SRID=4326;%s\'),way), 2167)), ST_Area(ST_Transform(ST_GeomFromEWKT(\'SRID=4326;%s\'), 2167)),osm_id,ST_Area(ST_Transform(way, 2167)) FROM boundary WHERE ST_OVERLAPS(ST_GeomFromEWKT(\'SRID=4326;%s\'),way) AND building IS NOT NULL;");

    if (boost::geometry::num_points(way.polygon) == 0) {
	log_error(_("Way polygon is empty!"));
        return ids;
    }
    fmt % boost::geometry::wkt(way.polygon);
    fmt % boost::geometry::wkt(way.polygon);
    fmt % boost::geometry::wkt(way.polygon);
    
    // log_debug("QueryN: %1%, %2%", way.id, fmt.str());
    pqxx::result result = conf_db.query(fmt.str());
    float tolerance = 0.001;   // tolerance for floating point comparisons
    float threshold = 10.0;
    for (auto it = std::begin(result); it != std::end(result); ++it) {
	float intersection =  it[0].as(float(0));
        bool area = false;
        bool overlap = false;
        osmobjects::OsmWay way2;
        way2.id = it[2].as(long(0));
        auto status1 = std::make_shared<ValidateStatus>(way2);

        float intersect = (it[1].as(float(0))/it[3].as(float(0)));
        float diff = abs(it[1].as(float(0)) - it[3].as(float(0)));
	// std::cerr << "DiffN: " << std::fixed << std::setprecision(4)
	// 	  << intersect
	// 	  << " : " << diff
	// 	  << " : " << intersection
	// 	  << " : " << way.id << std::endl;
        if (intersect < 2.0) {
	    // std::cerr << "similar area!" << std::endl;
            area = true;
        }
        if (intersection > 30.0 && area) {
            log_debug(_("Duplicates: %1%, %2%, intersection: %3%, diff: %4%"), way.id, way2.id, intersection,diff);
	    status1->status.insert(duplicate);
        } else {
            overlap = true;
            log_debug(_("Overlapping: %1%, %2%"), way.id, way2.id);
	    status1->status.insert(overlaping);
	}
	ids->push_back(status1);
    }
    return ids;
}

std::shared_ptr<std::vector<std::shared_ptr<ValidateStatus>>>
Conflate::existingDuplicatePolygon(void)
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("Conflate::existingDuplicatePolygon: took %w seconds\n");
#endif
    auto ids = std::make_shared<std::vector<std::shared_ptr<ValidateStatus>>>();
    if (!conf_db.isOpen()) {
	log_error(_("Database is not connected!"));
        return ids;
    }

    std::string query = "SELECT ST_Area(ST_Transform(ST_INTERSECTION(g1.way, g2.way), 2167)),g1.osm_id,ST_Area(ST_Transform(g1.way, 2167)),g2.osm_id,ST_Area(ST_Transform(g2.way, 2167)) FROM boundary AS g1, boundary AS g2 WHERE ST_OVERLAPS(g1.way, g2.way);";

    pqxx::result result = conf_db.query(query);
    // log_debug("QueryE: %1%", query);
    float tolerance = 0.001;   // tolerance for floating point comparisons
    float threshold = 10.0;
    for (auto it = std::begin(result); it != std::end(result); ++it) {
	float intersection =  it[0].as(float(0));
        bool area = false;
        bool overlap = false;
        osmobjects::OsmWay way1;
        way1.id = it[1].as(long(0));
        osmobjects::OsmWay way2;
        way2.id = it[3].as(long(0));
        auto status1 = std::make_shared<ValidateStatus>(way1);
	status1->osm_id =it[1].as(long(0));
        auto status2 = std::make_shared<ValidateStatus>(way2);
	status1->osm_id = it[3].as(long(0));

        float intersect = it[0].as(float(0))/it[2].as(float(0));
        float diff = abs(it[2].as(float(0)) - it[4].as(float(0)));
        std::cerr << "DiffE: " << std::fixed << std::setprecision(12) << intersect << " : " << diff << " : " << std::endl;
        // similar size, so more likely to be a duplicate
        if (intersect < 2.0) {
	    // std::cerr << "similar area!" << std::endl;
            area = true;
        }
        if (intersection > 30.0 && area) {
            log_debug(_("Duplicates: %1%, %2%, intersection: %3%, diff: %4%"), way1.id, way2.id, intersection,diff);
	    status1->status.insert(duplicate);
	    status2->status.insert(duplicate);
        } else {
            overlap = true;
            log_debug(_("Overlapping: %1%, %2%"), way1.id, way2.id);
	    status1->status.insert(overlaping);
	    status2->status.insert(overlaping);
	}
        ++it;                   // skip the duplicate entries
        ids->push_back(status1);
        ids->push_back(status2);
    }
    return ids;
}

std::shared_ptr<std::vector<std::shared_ptr<ValidateStatus>>>
Conflate::newDuplicateLineString(const osmobjects::OsmWay &way)
{
    auto ids = std::make_shared<std::vector<std::shared_ptr<ValidateStatus>>>();
    return ids;
}

std::shared_ptr<std::vector<std::shared_ptr<ValidateStatus>>>
Conflate::existingDuplicateLineString(void)
{
    auto ids = std::make_shared<std::vector<std::shared_ptr<ValidateStatus>>>();
    return ids;
}

} // namespace conflate

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:












