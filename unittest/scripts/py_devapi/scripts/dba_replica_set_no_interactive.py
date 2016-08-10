# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.get_farm({"enforce":True})
#@ Farm: validating members
farm = dba.create_farm('devFarm', 'testing')
farm.add_seed_instance({"host": __host})
rset = farm.getReplicaSet()

all_members = dir(rset)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Replica Set Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')

#@# Farm: add_instance errors
rset.add_instance()
rset.add_instance(5,6)
rset.add_instance(5)
rset.add_instance({"host": __host, "schema": 'abs'})
rset.add_instance({"host": __host, "user": 'abs'})
rset.add_instance({"host": __host, "password": 'abs'})
rset.add_instance({"host": __host, "authMethod": 'abs'})
rset.add_instance({"port": __port})
rset.add_instance('')

#@# Farm: add_instance
rset.add_instance(__host_port)
rset.add_instance({"host": __host, "port": __port})

# Cleanup
dba.drop_farm('devFarm', {"dropDefaultReplicaSet": True})
