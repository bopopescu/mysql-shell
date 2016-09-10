/*
* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include "shell_script_tester.h"
#include "utils/utils_general.h"

namespace shcore {
class Shell_py_dba_tests : public Shell_py_script_tester {
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_py_script_tester::SetUp();

    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("py_devapi");
    set_setup_script("setup.py");
  }

  virtual void set_defaults() {
    Shell_py_script_tester::set_defaults();

    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

    if (_port.empty())
      _port = "33060";

    std::string code = "__user = '" + user + "';";
    exec_and_out_equals(code);
    code = "__pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "__host = '" + host + "';";
    exec_and_out_equals(code);
    code = "__port = " + _port + ";";
    exec_and_out_equals(code);
    code = "__schema = 'mysql';";
    exec_and_out_equals(code);
    code = "__uri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__xhost_port = '" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    if (_mysql_port.empty()) {
      code = "__host_port = '" + host + ":3306';";
      exec_and_out_equals(code);
      code = "__mysql_port = 3306;";
      exec_and_out_equals(code);
    } else {
      code = "__host_port = '" + host + ":" + _mysql_port + "';";
      exec_and_out_equals(code);
      code = "__mysql_port = " + _mysql_port + ";";
      exec_and_out_equals(code);
      code = "__mysql_sandbox_port1 = " + _mysql_sandbox_port1 + ";";
      exec_and_out_equals(code);
      code = "__mysql_sandbox_port2 = " + _mysql_sandbox_port2 + ";";
      exec_and_out_equals(code);
      code = "__mysql_sandbox_port3 = " + _mysql_sandbox_port3 + ";";
      exec_and_out_equals(code);
    }

    code = "__uripwd = '" + user + ":" + password + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__displayuri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
    exec_and_out_equals(code);
    code = "import os";
  }
};

TEST_F(Shell_py_dba_tests, no_interactive_deploy_instances) {
  _options->wizards = false;
  reset_shell();

  if (_sandbox_dir.empty()) {
    execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    execute("dba.deploy_local_instance(" + _mysql_sandbox_port2 + ", {'password': 'root', 'verbose': True});");
  } else {
    execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', verbose: True});");
    execute("dba.deploy_local_instance(" + _mysql_sandbox_port2 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', verbose: True});");
  }
}

TEST_F(Shell_py_dba_tests, no_interactive_classic_global_dba) {
  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.py");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, no_interactive_classic_global_cluster) {
  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.py");

  // Drops the cluster the instances remain intact
  execute("dba.drop_cluster('devCluster', {'dropDefaultReplicaSet':True})");
  execute("c = dba.get_cluster({'masterKey':'testing'})");
  MY_EXPECT_STDERR_CONTAINS("Dba.get_cluster: No default cluster is configured.");

  execute("session.close()");
}

TEST_F(Shell_py_dba_tests, no_interactive_classic_custom_dba) {
  _options->wizards = false;
  reset_shell();

  execute("import mysql");
  execute("mySession = mysql.get_classic_session('root:root@localhost:" + _mysql_sandbox_port1 + "')");
  execute("dba.reset_session(mySession)");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.py");

  execute("mySession.close()");
}

TEST_F(Shell_py_dba_tests, no_interactive_classic_custom_cluster) {
  _options->wizards = false;
  reset_shell();

  execute("import mysql");
  execute("mySession = mysql.get_classic_session('root:root@localhost:" + _mysql_sandbox_port1 + "')");
  execute("dba.reset_session(mySession)");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.py");

  // Drops the cluster the instances remain intact
  execute("dba.drop_cluster('devCluster', {'dropDefaultReplicaSet':True})");
  execute("c = dba.get_cluster({'masterKey':'testing'})");
  MY_EXPECT_STDERR_CONTAINS("Dba.get_cluster: No default cluster is configured.");

  execute("mySession.close()");
}

TEST_F(Shell_py_dba_tests, interactive_classic_global_dba) {
  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  //@<OUT> Dba: createCluster with interaction
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: getCluster with interaction
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: getCluster with interaction (default)
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: dropCluster interaction no options, cancel
  output_handler.prompts.push_back("n");

  //@<OUT> Dba: dropCluster interaction missing option, ok error
  output_handler.prompts.push_back("y");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.py");

  // TODO: These three lines are temporary cleanup
  // They should be deleted when the interactive tests for the cluster are in place
  execute("var c = dba.getCluster({ 'masterKey':'testing' })");
  execute("c.removeInstance('localhost':" + _mysql_sandbox_port1 + ")");
  execute("dba.dropCluster('devCluster', { 'dropDefaultReplicaSet':True });");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, no_interactive_delete_instances) {
  _options->wizards = false;
  reset_shell();

  if (_sandbox_dir.empty()) {
    execute("dba.stop_local_instance(" + _mysql_sandbox_port1 + ");");
    execute("dba.stop_local_instance(" + _mysql_sandbox_port2 + ");");
    execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ");");
    execute("dba.delete_local_instance(" + _mysql_sandbox_port2 + ");");
  } else {
    execute("dba.stop_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    execute("dba.stop_local_instance(" + _mysql_sandbox_port2 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    execute("dba.stop_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    execute("dba.stop_local_instance(" + _mysql_sandbox_port2 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
  }
}
}
