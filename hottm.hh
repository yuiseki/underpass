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

/// \file hottm.hh
/// \brief Access data from a Tasking Manager database

#ifndef __HOTTM_HH__
#define __HOTTM_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <array>
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "hottm/tmdefs.hh"
#include "hottm/tmprojects.hh"
#include "hottm/tmteams.hh"
#include "hottm/tmusers.hh"

namespace tmdb {

/// \class Tasking Manager
/// \brief The TaskingManager class connects to the TM DB.
class TaskingManager
{
  public:
    /// connect to the DB specified with \a dburl.
    /// \param dburl DB server connection: HOST or
    ///        USER:PASSSWORD@HOST/DATABASENAME
    /// \return TRUE on success.
    bool connect(const std::string &dburl);

    /**
     * \brief Retrieve the users from TM DB.
     * \param userId, optional user id, default value of 0 synchronizes all users.
     * \return a vector of TMUser objects.
     */
    std::vector<TMUser> getUsers(TaskingManagerIdType userId = 0);

    std::vector<TMTeam> getTeams(void) { return getTeams(0); };

    std::vector<TMTeam> getTeams(long teamid);

    std::vector<TaskingManagerIdType> getTeamMembers(long teamid, bool active);

    std::vector<TMProject> getProjects(void) { return getProjects(0); };

    std::vector<TaskingManagerIdType> getProjectTeams(long projectid);

    std::vector<TMProject> getProjects(TaskingManagerIdType projectid);

    /**
     * \brief getWorker is a factory method that creates and returns a
     *        (possibly NULL) worker for the TM DB connection.
     * \note Ownership is transfered to the caller.
     * \return the (possibly NULL) unique pointer to the worker.
     */
    std::unique_ptr<pqxx::work> getWorker() const;

  private:
    std::unique_ptr<pqxx::connection> db;
};

} // namespace tmdb

#endif // EOF __HOTTM_HH__

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
