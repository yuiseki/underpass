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

/// \file validate.hh
/// \brief This class tries to validate the OSM objects
///
/// This class analyzes an OSM object to look for errors. This may
/// include lack of tags on a POI node or a way. This is not an
/// exhaustive test, mostly just a fast sanity-check.

#ifndef __VALIDATE_HH__
#define __VALIDATE_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <unordered_set>

#include <boost/config.hpp>
#include <boost/geometry.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/timer/timer.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "data/osmobjects.hh"
#include "data/yaml.hh"
#include "log.hh"

using namespace logger;

// JOSM validator
//   Crossing ways
//   Duplicate Ways
//   Duplicate nodes
//   Duplicate relations
//   Duplicated way nodes
//   Orphan nodes
//   No square building corners

// OSMInspector
//   Empty tag key
//   Unknown highway type
//   Single node way
//   Interescting ways

// OSMose
//   Overlapping buildings
//   orphan nodes
//   Duplicate geomtry
//   Highway not connected
//   Missing tags
//   Duplicate object
//
//


/// \enum osmtype_t
/// The data validation values for the status column in the database.
typedef enum {
    notags,
    complete,
    incomplete,
    badvalue,
    correct,
    badgeom,
    orphan,
    overlaping,
    duplicate
} valerror_t;

/// \class ValidateStatus
/// \brief This class stores data from the validation process
class ValidateStatus {
  public:
    ValidateStatus(void){};
    ValidateStatus(const osmobjects::OsmNode &node) {
        osm_id = node.id;
        user_id = node.uid;
        change_id = node.change_id;
        objtype = osmobjects::node;
        timestamp = node.timestamp;
    }
    ValidateStatus(const osmobjects::OsmWay &way) {
        osm_id = way.id;
        user_id = way.uid;
        change_id = way.change_id;
        objtype = osmobjects::way;
        timestamp = way.timestamp;
    }
    //valerror_t operator[](int index){ return status[index]; };
    /// Does this change have a particular status value
    bool hasStatus(const valerror_t &val) const {
        auto match = std::find(status.begin(), status.end(), val);
        if (match != status.end()) {
            return true;
        }
        return false;
    }
    /// Dump internal data for debugging
    void dump(void) const {
        std::cerr << "Dumping Validation Statistics" << std::endl;
        std::cerr << "\tOSM ID: " << osm_id << std::endl;
        std::cerr << "\tUser ID: " << user_id << std::endl;
        std::cerr << "\tChange ID: " << change_id << std::endl;
        std::cerr << "\tAngle: " << angle << std::endl;

        std::map<valerror_t, std::string> results;
        results[notags] = "No tags";
        results[complete] = "Tags are complete";
        results[incomplete] = "Tags are incomplete";
        results[badvalue] = "Bad tag value";
        results[correct] = "Correct tag value";
        results[badgeom] = "Bad geometry";
        results[orphan] = "Orphan";
        results[overlaping] = "Overlap";
        results[duplicate] = "Duplicate";
        for (const auto &stat: std::as_const(status)) {
            std::cerr << "\tResult: " << results[stat] << std::endl;
	}
	if (values.size() > 0) {
	    std::cerr << "\tValues: ";
	    for (auto it = std::begin(values); it != std::end(values); ++it ) {
		std::cerr << *it << ", " << std::endl;
	    }
	}
    }
    std::unordered_set<valerror_t> status;
    osmobjects::osmtype_t objtype;
    long osm_id = 0;		///< The OSM ID of the feature
    long user_id = 0;		///< The user ID of the mapper creating/modifying this feature
    long change_id = 0;		///< The changeset ID
    ptime timestamp;		///< The timestamp when this validation was performed
    point_t center;		///< The centroid of the building polygon
    double angle = 0;		///< The calculated angle of a corner
    std::unordered_set<std::string> values; ///< The found bad tag values
};


/// \class Validate
/// \brief This class contains shared methods for validating OSM map data
class BOOST_SYMBOL_VISIBLE Validate {
  public:
    Validate(void) {
        std::string dir = SRCDIR;
        if (!boost::filesystem::exists(dir)) {
            dir = PKGLIBDIR;
            if (!boost::filesystem::exists(dir)) {
                log_error(_("No validation config files in %1%!"), dir);
            }
        }
        for (auto &file: std::filesystem::recursive_directory_iterator(dir)) {
            std::filesystem::path config = file.path();
            if (config.extension() == ".yaml") {
                log_debug("Loading: %s", config.stem());
                yaml::Yaml yaml;
                yaml.read(config.string());
                if (!config.stem().empty()) {
                    yamls[config.stem()] = yaml;
                }
            }
        }
#if 0
	Validate(const std::string &filespec) {
	    yaml::Yaml yaml;
	    yaml.read(filespec);
	    if (!config.stem().empty()) {
		yamls[config.stem()] = yaml;
	    }
	}
#endif
    };
    virtual ~Validate(void){};
    // Validate(std::vector<std::shared_ptr<osmchange::OsmChange>> &changes) {};

    /// Check a POI for tags. A node that is part of a way shouldn't have any
    /// tags, this is to check actual POIs, like a school.
    virtual std::shared_ptr<ValidateStatus> checkPOI(const osmobjects::OsmNode &node, const std::string &type) = 0;

    /// This checks a way. A way should always have some tags. Often a polygon
    /// is a building
    virtual std::shared_ptr<ValidateStatus> checkWay(const osmobjects::OsmWay &way, const std::string &type) = 0;
    std::shared_ptr<ValidateStatus> checkTags(std::map<std::string, std::string> tags) {
	auto status = std::make_shared<ValidateStatus>();
        for (auto it = std::begin(tags); it != std::end(tags); ++it) {
            // FIXME: temporarily disabled
            // result = checkTag(it->first, it->second);
        }
	return status;
    };
    void dump(void) {
        for (auto it = std::begin(yamls); it != std::end(yamls); ++it) {
            it->second.dump();
        }
    }
    virtual std::shared_ptr<ValidateStatus> checkTag(const std::string &key, const std::string &value) = 0;
    yaml::Yaml &operator[](const std::string &key) { return yamls[key]; };

    bool overlaps(const std::list<std::shared_ptr<osmobjects::OsmWay>> &allways, osmobjects::OsmWay &way) {
	// This test only applies to buildings, as highways often overlap.
	yaml::Yaml tests = yamls["building"];
	if (tests.getConfig("overlaps") == "yes") {
#ifdef TIMING_DEBUG_X
	    boost::timer::auto_cpu_timer timer("validate::overlaps: took %w seconds\n");
#endif
	    if (way.numPoints() <= 1) {
		return false;
	    }
	    // It's probably a cicle
	    if (way.numPoints() > 5 && cornerAngle(way.linestring) < 30) {
		log_debug(_("Building %1% is round"), way.id);
		return false;
	    }
	    for (auto nit = std::begin(allways); nit != std::end(allways); ++nit) {
		osmobjects::OsmWay *oldway = nit->get();
		if (boost::geometry::overlaps(oldway->polygon, way.polygon)) {
		    if (way.getTagValue("layer") == oldway->getTagValue("layer") && way.id != oldway->id) {
			log_error(_("Building %1% overlaps with %2%"), way.id, oldway->id);
			return true;
		    }
		}
	    }
	}
	return false;
    }
    double cornerAngle(const linestring_t &way) {
	if (boost::geometry::num_points(way) < 2) {
	    log_error(_("way has no line segments!"));
	    return -1;
	}
        // first segment
        double x1 = boost::geometry::get<0>(way[0]);
        double y1 = boost::geometry::get<1>(way[0]);
        double x2 = boost::geometry::get<0>(way[1]);
        double y2 = boost::geometry::get<1>(way[1]);

        // Next segment that intersects
        double x3 = boost::geometry::get<0>(way[2]);
        double y3 = boost::geometry::get<1>(way[2]);

        double s1 = (y2 - y1) / (x2 - x1);
        double s2 = (y3 - y2) / (x3 - x2);
        double angle = std::atan((s2 - s1) / (1 + (s2 * s1))) * 180 / M_PI;
        return angle;
    };

  protected:
    std::map<std::string, yaml::Yaml> yamls;
};

#endif // EOF __VALIDATE_HH__

// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
