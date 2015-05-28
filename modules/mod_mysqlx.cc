/*
 * Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx.h"

using namespace mysh;
using namespace shcore;

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <iostream>
#include <iomanip>

#include "uuid_gen.h"

#include "shellcore/object_factory.h"
#include "mod_crud_collection_add.h"
#include "mod_crud_collection_find.h"
#include "mod_crud_table_insert.h"
#include "mod_result.h"

REGISTER_ALIASED_OBJECT(mysqlx, Connection, X_connection);
REGISTER_OBJECT(mysqlx, CollectionAdd);
REGISTER_OBJECT(mysqlx, CollectionFind);
REGISTER_OBJECT(mysqlx, TableInsert);

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/
static void translate_exception()
{
  try
  {
    throw;
  }
  catch (boost::system::system_error &e)
  {
    throw shcore::Exception::runtime_error(e.what());
  }
  catch (std::runtime_error &e)
  {
    throw shcore::Exception::runtime_error(e.what());
  }
  catch (std::logic_error &e)
  {
    throw shcore::Exception::logic_error(e.what());
  }
  catch (...)
  {
    throw;
  }
}

#define CATCH_AND_TRANSLATE()   \
  catch (...)                   \
{ translate_exception(); }

X_connection::X_connection(const std::string &uri, const char *password)
: Base_connection(uri, password), _next_stmt_id(0)
{
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 33060;
  std::string sock;
  std::string db;
  int pwd_found;

  if (!parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (password)
    pass.assign(password);

  std::stringstream ss;
  ss << port;

  try
  {
    _protobuf = boost::shared_ptr<mysqlx::Mysqlx_test_connector>(new mysqlx::Mysqlx_test_connector(host, ss.str()));
  }
  CATCH_AND_TRANSLATE();

  auth(user.c_str(), pass.c_str());
}

void X_connection::auth(const char *user, const char *pass)
{
  Mysqlx::Session::AuthenticateStart m;

  m.set_mech_name("plain");
  m.set_auth_data(user);
  m.set_initial_response(pass);
  _protobuf->send_message(&m);

  int mid;
  try
  {
    Message *r = _protobuf->read_response(mid);

    if (r)
      delete r;
  }
  CATCH_AND_TRANSLATE();

  if (mid != Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK)
  {
    throw shcore::Exception::argument_error("Authentication failed...");
  }
}

boost::shared_ptr<shcore::Object_bridge> X_connection::create(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, "Mysqlx_connection()");
  return boost::shared_ptr<shcore::Object_bridge>(new X_connection(args.string_at(0),
    args.size() > 1 ? args.string_at(1).c_str() : NULL));
}

X_resultset *X_connection::_sql(const std::string &query, shcore::Value options)
{
  int mid;
  Message* msg;

  // Reads any remaining stuff from the previos result
  // To let the comm in a clean state
  if (_last_result && !_last_result->is_all_fetch_done())
    _last_result->flush_messages(true);

  _timer.start();
  ++_next_stmt_id;

  bool has_data = false;

  try
  {
    Mysqlx::Sql::StmtExecute stmt;
    stmt.set_stmt(query);
    mid = Mysqlx::ClientMessages::SQL_STMT_EXECUTE;

    std::set<int> responses;
    responses.insert(Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
    responses.insert(Mysqlx::ServerMessages::SQL_COLUMN_META_DATA);

    msg = _protobuf->send_receive_message(mid, &stmt, responses, "executing statement: " + query);

    has_data = mid == Mysqlx::ServerMessages::SQL_COLUMN_META_DATA;
  }
  CATCH_AND_TRANSLATE();

  X_resultset* result = new X_resultset(shared_from_this(), has_data, _next_stmt_id, 0, 0, 0, NULL, mid, msg, true,
                                        options && options.type == shcore::Map ? options.as_map() : boost::shared_ptr<shcore::Value::Map_type>());

  // If no data will be returned then loads the resultset metadata
  // And fills the timer data as well
  if (mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
  {
    _timer.end();

    result->set_result_metadata(msg);
    result->reset(_timer.raw_duration());
  }

  return result;
}

shcore::Value X_connection::sql_one(const std::string &query)
{
  shcore::Value ret_val = shcore::Value::Null();

  // Executes the normal query
  sql(query, shcore::Value());

  if (_last_result->has_resultset())
  {
    shcore::Argument_list no_args;
    ret_val = _last_result->next(no_args);

    // Reads whatever is remaining, we do not care about it
    _last_result->flush_messages(true);
  }

  return ret_val;
}

shcore::Value X_connection::sql(const std::string &query, shcore::Value options)
{
  // Creates the resultset
  _last_result.reset(_sql(query, options));

  // Calls next result so the metadata is loaded
  // if any result
  if (_last_result->has_resultset())
    next_result(_last_result.get(), true);

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(_last_result));
}

shcore::Value X_connection::close(const shcore::Argument_list &args)
{
  if (_last_result && !_last_result->is_all_fetch_done())
    _last_result->flush_messages(true);

  _protobuf->close();

  return shcore::Value();
}

X_connection::~X_connection()
{
  close(shcore::Argument_list());
}

shcore::Value X_connection::crud_execute(const std::string& id, shcore::Value::Map_type_ref data)
{
  shcore::Value ret_val;

  if (id == "TableInsert")
    ret_val = crud_table_insert(data);
  else if (id == "CollectionAdd")
    ret_val = crud_collection_add(data);
  else if (id == "CollectionFind")
    ret_val = crud_collection_find(data);

  return ret_val;
}

std::string get_new_uuid()
{
  uuid_type uuid;
  generate_uuid(uuid);

  std::stringstream str;
  str << std::hex << std::noshowbase << std::setfill('0') << std::setw(2);
  //*
  str << (int)uuid[0] << std::setw(2) << (int)uuid[1] << std::setw(2) << (int)uuid[2] << std::setw(2) << (int)uuid[3];
  str << std::setw(2) << (int)uuid[4] << std::setw(2) << (int)uuid[5];
  str << std::setw(2) << (int)uuid[6] << std::setw(2) << (int)uuid[7];
  str << std::setw(2) << (int)uuid[8] << std::setw(2) << (int)uuid[9];
  str << std::setw(2) << (int)uuid[10] << std::setw(2) << (int)uuid[11]
    << std::setw(2) << (int)uuid[12] << std::setw(2) << (int)uuid[13]
    << std::setw(2) << (int)uuid[14] << std::setw(2) << (int)uuid[15];

  return str.str();
}

shcore::Value X_connection::crud_collection_add(shcore::Value::Map_type_ref data)
{
  //----SAMPLE IMPLEMENTATION
  // This implementation allows performing a table insert operation as:
  //
  // table.insert(['col1', 'col2','col3']).values(['val1', val2', 'val3']).run()
  //
  // The definitive implementation should consider many different cases
  // and also many validations must be included
  uint64_t affected_rows = 0;
  uint64_t last_insert_id = 0;
  uuid_type uuid;

  try
  {
    Mysqlx::Crud::Insert insert;
    insert.mutable_collection()->set_schema((*data)["schema"].as_string());
    insert.mutable_collection()->set_name((*data)["collection"].as_string());
    insert.set_data_model(Mysqlx::Crud::DataModel((*data)["data_model"].as_int()));

    std::vector<std::string> json_docs;
    size_t count = 0;

    if (data->has_key("Documents"))
    {
      shcore::Value::Map_type_ref doc;
      shcore::Value::Array_type_ref doc_collection;

      switch ((*data)["Documents"].type)
      {
        case shcore::Map:
          doc = (*data)["Documents"].as_map();
          if (!doc->has_key("_id"))
          {
            (*doc)["_id"] = shcore::Value(get_new_uuid());
          }

          // Appends the document to the list to be inserted
          json_docs.push_back((*data)["Documents"].repr());
          break;
        case shcore::Array:
          doc_collection = (*data)["Documents"].as_array();
          count = doc_collection->size();
          for (size_t index = 0; index < count; index++)
          {
            if ((*doc_collection)[index].type == shcore::Map)
            {
              doc = (*doc_collection)[index].as_map();
              if (!doc->has_key("_id"))
                (*doc)["_id"] = shcore::Value(get_new_uuid());

              // Appends the document to the list to be inserted
              json_docs.push_back((*doc_collection)[index].repr());
            }
            else
              throw shcore::Exception::argument_error("Invalid document specified on list for add operation.");
          }
          break;
        default:
          throw shcore::Exception::argument_error("Invalid document specified on add operation.");
          break;
      }
    }

    // Here it gors the implementation in case no Documents are specified
    // i.e. uses an iterator instead
    else
      throw shcore::Exception::logic_error("No documents specified on add operation.");

    int mid = Mysqlx::ClientMessages::CRUD_INSERT;

    // Assumming there were docs, if not a failure should have been raised already
    // Creates the row list
    for (size_t index = 0; index < json_docs.size(); index++)
    {
      Mysqlx::Sql::Row* row = insert.mutable_row()->Add();
      X_row::add_field(row, shcore::Value(json_docs[index]));
    }

    Message* msg = _protobuf->send_receive_message(mid, &insert, Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK, "Inserting documents...");

    Mysqlx::Sql::StmtExecuteOk *insert_ok = dynamic_cast<Mysqlx::Sql::StmtExecuteOk *>(msg);

    if (insert_ok->has_rows_affected())
      affected_rows = insert_ok->rows_affected();
    if (insert_ok->has_last_insert_id())
      last_insert_id = insert_ok->last_insert_id();
  }
  CATCH_AND_TRANSLATE();

  // Creates a resultset with the received info
  X_resultset* result = new X_resultset(shared_from_this(), false, _next_stmt_id, affected_rows, 0, 0, NULL, 0, NULL, false);

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(result));
}

shcore::Value X_connection::crud_collection_find(shcore::Value::Map_type_ref data)
{
  // The definitive implementation should consider many different cases
  // and also many validations must be included
  uint64_t affected_rows = 0;
  uint64_t last_insert_id = 0;
  uuid_type uuid;
  int mid;
  Message* msg;

  // Reads any remaining stuff from the previos result
  // To let the comm in a clean state
  if (_last_result && !_last_result->is_all_fetch_done())
    _last_result->flush_messages(true);

  _timer.start();
  ++_next_stmt_id;

  bool has_data = false;

  try
  {
    Mysqlx::Crud::Find find;
    find.mutable_collection()->set_schema((*data)["schema"].as_string());
    find.mutable_collection()->set_name((*data)["collection"].as_string());
    find.set_data_model(Mysqlx::Crud::DataModel((*data)["data_model"].as_int()));

    std::vector<std::string> json_docs;
    size_t count = 0;

    if (data->has_key("find.SearchCondition"))
    {
      // TODO: Uncomment once the Projection parser is fully implemented
      //std::string filter = (*data)["find.SearchCondition"].as_string();
      //Expr_parser expression(filter);
      //find.set_allocated_criteria(expression.expr().release());
    }

    if (data->has_key("fields.SearchFields"))
    {
      // TODO: Set the fields when the Projection parser is implemented
    }

    if (data->has_key("groupby.SearchFields"))
    {
      // TODO: Set the ordering when the Projection parser is implemented
    }

    if (data->has_key("having.SearchCondition"))
    {
      // TODO: Sets the value when the Expression parser for collections
      //       is implemented and HAVING is supported
    }

    if (data->has_key("SortFields"))
    {
      // TODO: Set the ordering when the Srot parser is implemented
    }

    if (data->has_key("LimitOffset"))
      find.mutable_limit()->set_skip((*data)["LimitOffset"].as_int());

    if (data->has_key("NumberOfRows"))
      find.mutable_limit()->set_offset((*data)["NumberOfRows"].as_int());

    mid = Mysqlx::ClientMessages::CRUD_FIND;

    std::set<int> responses;
    responses.insert(Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
    responses.insert(Mysqlx::ServerMessages::SQL_COLUMN_META_DATA);

    msg = _protobuf->send_receive_message(mid, &find, responses, "Finding documents...");

    has_data = mid == Mysqlx::ServerMessages::SQL_COLUMN_META_DATA;
  }
  CATCH_AND_TRANSLATE();

  Collection_resultset* result = new Collection_resultset(shared_from_this(), has_data, _next_stmt_id, 0, 0, 0, NULL, mid, msg, false, boost::shared_ptr<shcore::Value::Map_type>());

  // If no data will be returned then loads the resultset metadata
  // And fills the timer data as well
  if (mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
  {
    _timer.end();

    result->set_result_metadata(msg);
    result->reset(_timer.raw_duration());
  }

  // Creates the resultset
  _last_result.reset(result);

  // Calls next result so the metadata is loaded
  // if any result
  if (_last_result->has_resultset())
    next_result(_last_result.get(), true);

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(_last_result));
}

shcore::Value X_connection::crud_table_insert(shcore::Value::Map_type_ref data)
{
  //----SAMPLE IMPLEMENTATION
  // This implementation allows performing a table insert operation as:
  //
  // table.insert(['col1', 'col2','col3']).values(['val1', val2', 'val3']).run()
  //
  // The definitive implementation should consider many different cases
  // and also many validations must be included
  uint64_t affected_rows = 0;
  uint64_t last_insert_id = 0;

  try
  {
    Mysqlx::Crud::Insert insert;
    insert.mutable_collection()->set_schema((*data)["schema"].as_string());
    insert.mutable_collection()->set_name((*data)["collection"].as_string());
    insert.set_data_model(Mysqlx::Crud::DataModel((*data)["data_model"].as_int()));

    if (data->has_key("Fields"))
    {
      if (data->has_key("Values"))
      {
        shcore::Value::Array_type_ref fields = (*data)["Fields"].as_array();
        shcore::Value::Array_type_ref values = (*data)["Values"].as_array();

        if (fields->size() == values->size())
        {
          Mysqlx::Crud::Column *column;
          size_t index;

          // Creates the projection
          for (index = 0; index < fields->size(); index++)
          {
            column = insert.mutable_projection()->Add();
            column->set_name(fields->at(index).as_string());
          }

          // Creates the row list
          Mysqlx::Sql::Row *row = insert.mutable_row()->Add();
          for (index = 0; index < fields->size(); index++)
            X_row::add_field(row, values->at(index));
        }
      }
    }
    else if (data->has_key("FieldsAndValues"))
    {
    }

    int mid = Mysqlx::ClientMessages::CRUD_INSERT;
    Message* msg = _protobuf->send_receive_message(mid, &insert, Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK, "Inserting record...");

    Mysqlx::Sql::StmtExecuteOk *insert_ok = dynamic_cast<Mysqlx::Sql::StmtExecuteOk *>(msg);

    if (insert_ok->has_rows_affected())
      affected_rows = insert_ok->rows_affected();
    if (insert_ok->has_last_insert_id())
      last_insert_id = insert_ok->last_insert_id();
  }
  CATCH_AND_TRANSLATE();

  // Creates the resultset
  X_resultset* result = new X_resultset(shared_from_this(), false, _next_stmt_id, affected_rows, 0, 0, NULL, 0, NULL, false);

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(result));
}

bool X_connection::next_result(Base_resultset *target, bool first_result)
{
  bool ret_val = false;
  X_resultset *real_target = dynamic_cast<X_resultset *> (target);

  // Just to ensure the right instance was received as parameter
  if (real_target && real_target->has_resultset())
  {
    // The first call to next result is done to fetch the metadata
    // Subsequent calls to next_result need to ensure all the messages on the
    // tx buffer are cleared out first
    if (!first_result && !real_target->is_current_fetch_done())
      real_target->flush_messages(false);

    if (!real_target->is_all_fetch_done())
    {
      ret_val = true;
      real_target->fetch_metadata();

      _timer.end();
      real_target->reset(_timer.raw_duration());
    }
  }

  return ret_val;
}

X_row::X_row(Mysqlx::Sql::Row *row, std::vector<Field>* metadata) : Base_row(metadata), _row(row)
{
}

shcore::Value X_row::get_value(int index)
{
  Mysqlx::Datatypes::Any field(_row->field(index));

  // TODO: Is there a way a field has nothing (like in mysql NULL)
  // TODO: What if field.has_object and field.has_array
  if (field.has_scalar())
  {
    if (field.scalar().has_v_bool())
      return shcore::Value(field.scalar().v_bool());
    else if (field.scalar().has_v_double())
      return shcore::Value(field.scalar().v_double());
    else if (field.scalar().has_v_float())
      return shcore::Value(field.scalar().v_float());
    //else if (field.scalar().has_v_null())
    //  return shcore::Value::Null();
    else if (field.scalar().has_v_string())
      return shcore::Value(field.scalar().v_string().value());
    else if (field.scalar().has_v_opaque())
      return shcore::Value(field.scalar().v_opaque());
    else if (field.scalar().has_v_signed_int())
      return shcore::Value(field.scalar().v_signed_int());
    else if (field.scalar().has_v_unsigned_int())
      return shcore::Value(int64_t(field.scalar().v_unsigned_int()));
  }

  return shcore::Value::Null();
}

std::string X_row::get_value_as_string(int index)
{
  shcore::Value value = get_value(index);
  return value.repr();
}

void X_row::add_field(Mysqlx::Sql::Row *row, shcore::Value value)
{
  Mysqlx::Datatypes::Any *field = row->add_field();

  field->set_type(Mysqlx::Datatypes::Any::SCALAR);

  switch (value.type)
  {
    case Null:
    case Undefined:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
      break;
    case Bool:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_BOOL);
      field->mutable_scalar()->set_v_bool(value.as_bool());
      break;
    case Float:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_DOUBLE);
      field->mutable_scalar()->set_v_double(value.as_double());
      break;
    case Integer:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_UINT);
      field->mutable_scalar()->set_v_unsigned_int(value.as_int());
      break;
    case String:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
      field->mutable_scalar()->mutable_v_string()->set_value(value.as_string());
      break;
    case Array:
      break;
    case Function:
      break;
    case Map:
      break;
    case MapRef:
      break;
    case Object:
      break;
  }
}

X_row::~X_row()
{
  if (_row)
    delete _row;
}