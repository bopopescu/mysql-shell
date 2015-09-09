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
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include "mod_mysqlx_table_select.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

TableSelect::TableSelect(boost::shared_ptr<Table> owner)
: Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("select", boost::bind(&TableSelect::select, this, _1), "data");
  add_method("where", boost::bind(&TableSelect::where, this, _1), "data");
  add_method("groupBy", boost::bind(&TableSelect::group_by, this, _1), "data");
  add_method("having", boost::bind(&TableSelect::having, this, _1), "data");
  add_method("orderBy", boost::bind(&TableSelect::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableSelect::limit, this, _1), "data");
  add_method("offset", boost::bind(&TableSelect::offset, this, _1), "data");
  add_method("bind", boost::bind(&TableSelect::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("select", "");
  register_dynamic_function("where", "select");
  register_dynamic_function("groupBy", "select, where");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("orderBy", "select, where, groupBy, having");
  register_dynamic_function("limit", "select, where, groupBy, having, orderBy");
  register_dynamic_function("offset", "limit");
  register_dynamic_function("bind", "select, where, groupBy, having, orderBy, offset, limit");
  register_dynamic_function("execute", "select, where, groupBy, having, orderBy, offset, limit, bind");
  register_dynamic_function("__shell_hook__", "select, where, groupBy, having, orderBy, offset, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Initializes this record selection handler.
* \param searchExprStr: An optional list of string expressions identifying the columns to be retrieved, alias support is enabled on these fields.
* \return This TableSelect object.
*
* The TableSelect handler will only retrieve the columns that were included on the filter, if no filter was set then all the columns will be included.
*
* Calling this function is allowed only for the first time and it is done automatically when Table.select() is called.
*
* After this function invocation, the following functions can be invoked:
*
* - where(String searchCriteria)
* - groupBy(List searchExprStr)
* - orderBy(List sortExprStr);
* - limit(Integer numberOfRows);
* - execute(ExecuteOptions options), including usage examples.
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableSelect TableSelect::select(List searchExprStr){}
#endif
shcore::Value TableSelect::select(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "TableSelect.select");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      std::vector<std::string> fields;

      if (args.size())
      {
        parse_string_list(args, fields);

        if (fields.size() == 0)
          throw shcore::Exception::argument_error("Field selection criteria can not be empty");
      }

      _select_statement.reset(new ::mysqlx::SelectStatement(table->_table_impl->select(fields)));

      // Updates the exposed functions
      update_functions("select");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.select");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the search condition to filter the records to be retrieved from the owner Table.
* \param searchCondition: An optional expression to filter the records to be retrieved;
* if not specified all the records will be retrieved from the table unless a limit is set.
* \return This TableSelect object.
*
* This function can be invoked only once after:
*
* - select(List columns)
*
* After this function invocation, the following functions can be invoked:
*
* - groupBy(List searchExprStr)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/TableSelect TableSelect::where(String searchCondition){}
#endif
shcore::Value TableSelect::where(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.where");

  try
  {
    _select_statement->where(args.string_at(0));

    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.where");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a grouping criteria for the resultset.
* \param searchExprStr: A list of string expressions identifying the grouping criteria.
* \return This TableSelect object.
*
* If used, the TableSelect handler will group the records using the stablished criteria.
*
* This function can be invoked only once after:
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
*
* After this function invocation the following functions can be invoked:
*
* - having(String searchCondition)
* - orderBy(List sortExprStr);
* - limit(Integer numberOfRows);
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/TableSelect TableSelect::groupBy(List searchExprStr){}
#endif
shcore::Value TableSelect::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.groupBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Grouping criteria can not be empty");

    _select_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.groupBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a condition for records to be considered in agregate function operations.
* \param searchCondition: A condition on the agregate functions used on the grouping criteria.
* \return This TableSelect object.
*
* If used the TableSelect operation will only consider the records matching the stablished criteria.
*
* This function can be invoked only once after:
*
* - groupBy(List searchExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/

TableSelect TableSelect::having(String searchCondition){}
#endif
shcore::Value TableSelect::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.having");

  try
  {
    _select_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.having");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the sorting criteria to be used on the Resultset.
* \param sortExprStr: A list of expression strings defining the sort criteria for the returned records.
* \return This TableSelect object.
*
* If used the TableSelect handler will return the records sorted with the defined criteria.
*
* The elements of sortExprStr list are strings defining the column name on which the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* This function can be invoked only once after:
*
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableSelect TableSelect::orderBy(List sortExprStr){}
#endif
shcore::Value TableSelect::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _select_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the maximum number of records to be returned on the select operation.
* \param numberOfRows: The maximum number of records to be retrieved.
* \return This TableSelect object.
*
* If used, the TableSelect operation will return at most numberOfRows records.
*
* This function can be invoked only once after:
*
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
* - orderBy(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - offset(Integer limitOffset)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableSelect TableSelect::limit(Integer numberOfRows){}
#endif
shcore::Value TableSelect::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.limit");

  try
  {
    _select_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets number of records to skip on the resultset when a limit has been defined.
* \param limitOffset: The number of records to skip before start including them on the Resultset.
* \return This TableSelect object.
*
* This function can be invoked only once after:
*
* - limit(Integer numberOfRows)
*
* After this function invocation, the following functions can be invoked:
*
* - execute(ExecuteOptions options)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableSelect TableSelect::offset(Integer limitOffset){}
#endif
shcore::Value TableSelect::offset(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect.offset");

  try
  {
    _select_statement->offset(args.uint_at(0));

    update_functions("offset");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.offset");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
TableSelect TableSelect::bind({ var:val, var : val, ... })
{}
#endif
shcore::Value TableSelect::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableSelect.bind: not yet implemented.");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the Find operation with all the configured options and returns.
* \return Collection_resultset A Collection resultset object that can be used to retrieve the results of the find operation.
*
* This function can be invoked after any other function on this class.
*
* Examples:
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getSession("myuser@localhost", mypwd);
*
* // Creates a table named friends on the test schema
* mysession.sql('create table test.friends (name varchar(50), age integer, gender varchar(20));').execute();
*
* var table = mysession.test.friends;
*
* // create some initial data
* table.insert('name','last_name','age','gender')
*      .values('jack','black', 17, 'male')
* .    .values('adam', 'sandler', 15, 'male')
* .    .values('brian', 'adams', 14, 'male')
* .    .values('alma', 'lopez', 13, 'female').execute();

* table.insert(['name','last_name','age','gender'])
* .    .values('carol', 'shiffield', 14, 'female')
* .    .values('donna', 'summers', 16, 'female')
* .    .values('angel', 'down', 14, 'male').execute();

*
* // Retrieve all the records from the table
* var res_all = table.find().execute();
*
* // Retrieve the records for all males
* var res_males = table.select().where('gender="male"').execute();
*
* // Retrieve the name and last name only
* var res_partial = table.select(['name', 'last_name']).execute();
*
* // Retrieve the records sorted by age in descending order
* var res_sorted = table.select(['name', 'last_name', 'age']).orderBy(['age desc']).execute();
*
* // Retrieve the four younger friends
* var res_youngest = table.select().orderBy(['age']).limit(4).execute();
*
* // Retrieve the four younger friends after the youngest
* var res = table.select().orderBy(['age']).limit(4).offset(1).execute();
* \endcode
*/ResultSet TableSelect::execute(ExecuteOptions options){}
#endif
shcore::Value TableSelect::execute(const shcore::Argument_list &args)
{
  mysqlx::Resultset *result = NULL;

  try
  {
    args.ensure_count(0, "TableSelect.execute");

    result = new mysqlx::Resultset(boost::shared_ptr< ::mysqlx::Result>(_select_statement->execute()));
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}