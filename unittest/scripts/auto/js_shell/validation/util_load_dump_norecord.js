//@<ERR> Bad Options (should fail)
Util.loadDump: Argument #1 is expected to be a string (ArgumentError)
Util.loadDump: Argument #1 is expected to be a string (ArgumentError)
Util.loadDump: Argument #1 is expected to be a string (ArgumentError)
Util.loadDump: Cannot open file '[[*]]<<<filename_for_output("ldtest/dump/@.sql/@.json")>>>': No[[*]] directory (RuntimeError)
Util.loadDump: Cannot open file '[[*]]<<<filename_for_output("ldtest/@.json")>>>': No such file or directory (RuntimeError)
Util.loadDump: Cannot open file '[[*]]<<<filename_for_output("ldtest/@.json")>>>': No such file or directory (RuntimeError)
Util.loadDump: Argument #2 is expected to be a map (ArgumentError)
Util.loadDump: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
Util.loadDump: The option 'osNamespace' cannot be used when the value of 'osBucketName' option is not set. (ArgumentError)
Util.loadDump: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set. (ArgumentError)
Util.loadDump: Cannot open file: /badpath. (RuntimeError)
Util.loadDump: Invalid options: bogus (ArgumentError)
Util.loadDump: Option 'includeSchemas' is expected to be of type Array, but is String (TypeError)
Util.loadDump: At least one of loadData, loadDdl or loadUsers options must be enabled (RuntimeError)
Util.loadDump: At least one of loadData, loadDdl or loadUsers options must be enabled (RuntimeError)
Util.loadDump: Option 'waitDumpTimeout' UInteger expected, but value is String (TypeError)
Util.loadDump: Option 'analyzeTables' is expected to be of type String, but is Bool (TypeError)
Util.loadDump: Invalid value '' for analyzeTables option, allowed values: 'off', 'on', and 'histogram'. (RuntimeError)
Util.loadDump: Invalid value 'xxx' for analyzeTables option, allowed values: 'off', 'on', and 'histogram'. (RuntimeError)
Util.loadDump: Invalid value 'xxx' for deferTableIndexes option, allowed values: 'all', 'fulltext', and 'off'. (RuntimeError)
Util.loadDump: Invalid value '' for deferTableIndexes option, allowed values: 'all', 'fulltext', and 'off'. (RuntimeError)
Util.loadDump: Option 'deferTableIndexes' is expected to be of type String, but is Bool (TypeError)

//@<ERR> Bad Bucket Name Option {!hasOciEnvironment('OS')}
Util.loadDump: Cannot open file: [[*]]<<<filename_for_output("/.oci/config")>>>. (RuntimeError)

//@<ERR> Bad Bucket Name Option {hasOciEnvironment('OS')}
Util.loadDump: Failed opening object '@.json' in READ mode: Not Found (404) (RuntimeError)

//@# Plain load of plain dump (compressed and chunked)
|Loading DDL and Data from '[[*]]ldtest/dump' using 4 threads.|
|Executing common preamble SQL|
|Executing DDL script for schema|
|Executing DDL script for `xtest`.`t_tinyint`|
|Executing triggers SQL for `sakila`.`payment`|
|Executing common postamble SQL|
|0 warnings were reported during the load.|
