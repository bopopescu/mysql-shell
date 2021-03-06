/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
#define MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/metadata_management_mysql.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlsh {
namespace dba {

class Instance;
class MetadataStorage;

struct NewInstanceInfo {
  std::string member_id;
  std::string host;
  int port;
  std::string version;
};

struct MissingInstanceInfo {
  std::string id;
  std::string label;
  std::string endpoint;
};

// TODO(rennox): This should be renamed to simply InstanceType as it is
// no longer exclusive to GR.
namespace GRInstanceType {
enum Type {
  Standalone = 1 << 0,
  GroupReplication = 1 << 1,
  InnoDBCluster = 1 << 2,
  StandaloneWithMetadata = 1 << 3,
  StandaloneInMetadata = 1 << 4,
  AsyncReplicaSet = 1 << 5,
  Unknown = 1 << 6
};
}

namespace ManagedInstance {
enum State {
  OnlineRW = 1 << 0,
  OnlineRO = 1 << 1,
  Recovering = 1 << 2,
  Unreachable = 1 << 3,
  Offline = 1 << 4,
  Error = 1 << 5,
  Missing = 1 << 6,
  Any =
      OnlineRO | OnlineRW | Recovering | Unreachable | Offline | Error | Missing
};

std::string describe(State state);
}  // namespace ManagedInstance

namespace ReplicationQuorum {
enum class States {
  All_online = 1 << 0,
  Normal = 1 << 1,
  Quorumless = 1 << 2,
  Dead = 1 << 3,
};

using State = mysqlshdk::utils::Enum_set<States, States::Dead>;

}  // namespace ReplicationQuorum

struct Cluster_check_info {
  // Server version from the instance from which the data was consulted
  mysqlshdk::utils::Version source_version;

  // The state of the cluster from the quorum point of view
  // Supports multiple states i.e. Normal | All_online
  ReplicationQuorum::State quorum;

  // The configuration type of the instance from which the data was consulted
  GRInstanceType::Type source_type;

  // The state of the instance from which the data was consulted
  ManagedInstance::State source_state;
};

// The AdminAPI maximum supported MySQL Server version
const mysqlshdk::utils::Version k_max_adminapi_server_version =
    mysqlshdk::utils::Version("8.1");

// The AdminAPI minimum supported MySQL Server version
const mysqlshdk::utils::Version k_min_adminapi_server_version =
    mysqlshdk::utils::Version("5.7");

// Specific minimum versions for GR and AR functions
const mysqlshdk::utils::Version k_min_gr_version =
    mysqlshdk::utils::Version("5.7");
const mysqlshdk::utils::Version k_min_ar_version =
    mysqlshdk::utils::Version("8.0");

enum class MDS_actions { NONE, NOTE, WARN, RAISE_ERROR };

struct Metadata_validations {
  metadata::States state;
  MDS_actions action;
};

// Note that this structure may be initialized using initializer
// lists, so the order of the fields is very important
struct FunctionAvailability {
  mysqlshdk::utils::Version min_version;
  int instance_config_state;
  ReplicationQuorum::State cluster_status;
  int instance_status;
  std::vector<Metadata_validations> metadata_validations;
};

void validate_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

void check_preconditions(const std::string &function_name,
                         const Cluster_check_info &info,
                         FunctionAvailability *custom_func_avail = nullptr);

Cluster_check_info get_cluster_check_info(const MetadataStorage &group_server);

// NOTE: BUG#30628479 is applicable to all the API functions and the root cause
// is that several instances of the Metadata class are created during a function
// execution.
// To solve this, all the API functions should create a single instance of the
// Metadata class and pass it down through the chain call. In other words, the
// following function should be deprecated in favor of the version below.
Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<Instance> &group_server,
    FunctionAvailability *custom_func_avail = nullptr);

// All fun
Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<MetadataStorage> &metadata,
    FunctionAvailability *custom_func_avail = nullptr);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
