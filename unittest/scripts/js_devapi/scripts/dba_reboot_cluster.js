// Assumptions: smart deployment rountines available

//@<> Skip tests in 8.0.4 to not trigger GR plugin deadlock {VER(==8.0.4)}
testutil.skip("Reboot tests freeze in 8.0.4 because of bug in GR");

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

// Update __have_ssl and other with the real instance SSL support.
// NOTE: Workaround BUG#25503817 to display the right ssl info for status()
update_have_ssl(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode:'DISABLED'});

session.close();
// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({scheme:'mysql', host: "localhost", port: __mysql_sandbox_port2, user: 'root', password: 'root'})

cluster.status();

//@ Add instance 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@ Add instance 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3);

// Waiting for the third added instance to become online
wait_slave_state(cluster, uri3, "ONLINE");

//@ Dba.rebootClusterFromCompleteOutage errors
dba.rebootClusterFromCompleteOutage("");
dba.rebootClusterFromCompleteOutage("dev", {invalidOpt: "foobar"});
dba.rebootClusterFromCompleteOutage("dev2");
dba.rebootClusterFromCompleteOutage("dev");

// Kill all the instances

// Kill instance 2
testutil.killSandbox(__mysql_sandbox_port2);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
wait_slave_state(cluster, uri2, "(MISSING)");

// Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
wait_slave_state(cluster, uri3, "UNREACHABLE");

// Kill instance 1
testutil.killSandbox(__mysql_sandbox_port1);

// Re-start all the instances except instance 3

// Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);
//the timeout for GR plugin to install a new view is 60s, so it should be at
// least that value the parameter for the timeout for the waitForDelayedGRStart
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, 'root', 100);

// Start instance 1
testutil.startSandbox(__mysql_sandbox_port1);
//the timeout for GR plugin to install a new view is 60s, so it should be at
// least that value the parameter for the timeout for the waitForDelayedGRStart
testutil.waitForDelayedGRStart(__mysql_sandbox_port1, 'root', 100);

session.close();

cluster.disconnect();

// Re-establish the connection to instance 1
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

var instance2 = localhost + ':' + __mysql_sandbox_port2;
var instance3 = localhost + ':' + __mysql_sandbox_port3;

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance3]});

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance2]});

// Test both rejoinInstances and removeInstances on a single call
//@ Dba.rebootClusterFromCompleteOutage success
cluster = dba.rebootClusterFromCompleteOutage("dev", {rejoinInstances: [instance2], removeInstances: [instance3], clearReadOnly: true});

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@<OUT> cluster status after reboot
cluster.status();
session.close();

//@ Finalization
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
