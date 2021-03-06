// Validate that the Shell option "credentialStore.helper" already has a value set depending on the OS that is in use

//@ Verify that the shell.options["credentialStore.helper"] option is set to a valid value for the OS that is in use.
shell.options["credentialStore.helper"];

//@ [SR6: always]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it (set to always).
shell.options["credentialStore.savePasswords"] = "always";

//@ Create a Session to a server using valid credentials without the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
shell.connect(__cred.x.uri);

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
EXPECT_ARRAY_CONTAINS(__cred.x.uri, shell.listCredentials());
