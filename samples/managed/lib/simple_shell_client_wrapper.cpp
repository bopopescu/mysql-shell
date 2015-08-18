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

#include "msclr\marshal.h"
#include "msclr\marshal_cppstd.h"

#include <string>

#include <boost/shared_ptr.hpp>

#include "shellcore/ishell_core.h"
#include "simple_client_shell_wrapper.h"
#include "modules/base_resultset.h"

using namespace MySqlX::Shell;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;

[assembly:AssemblyDelaySign(false)];
[assembly:AssemblyKeyName("ConnectorNet")];
[assembly:AssemblyVersion("0.0.1")];
[assembly:AssemblyInformationalVersion("0.0.1")];


void Custom_shell::print(const char *text)
{
  _shell->Print(msclr::interop::marshal_as<String^>(text));
}

void Custom_shell::print_error(const char *text)
{
  _shell->PrintError(msclr::interop::marshal_as<String^>(text));
}

bool Custom_shell::input(const char *text, std::string &ret)
{
  String^ m_ret = gcnew String("");
  bool result = _shell->Input(msclr::interop::marshal_as<String^>(text), m_ret);
  ret = msclr::interop::marshal_as<std::string>(m_ret);
  delete m_ret;
  return result;
}

bool Custom_shell::password(const char *text, std::string &ret)
{
  String^ m_ret = gcnew String("");
  bool result = _shell->Password(msclr::interop::marshal_as<String^>(text), m_ret);
  ret = msclr::interop::marshal_as<std::string>(m_ret);
  delete m_ret;
  return result;
}

void Custom_shell::source(const char* module)
{
  _shell->Source(msclr::interop::marshal_as<String^>(module));
}

SimpleClientShell::SimpleClientShell()
{
  gcroot< SimpleClientShell^> _managed_obj(this);
  _obj = new Custom_shell(_managed_obj);
}

void SimpleClientShell::MakeConnection(String ^connstr)
{
  //__try {
    std::string n_connstr = msclr::interop::marshal_as<std::string>(connstr);
    _obj->make_connection(n_connstr);
  /*}
  __except
  {
    throw gcnew Exception(msclr::interop::marshal_as<String^>(e.what()));
  }*/
}

void SimpleClientShell::SwitchMode(Mode^ mode)
{
  Int32^ p_mode = Convert::ToInt32(mode);
  int p_mode2 = safe_cast<int>(p_mode);
  shcore::IShell_core::Mode n_mode = static_cast<shcore::IShell_core::Mode>(p_mode2);
  _obj->switch_mode(n_mode);
}

List<Dictionary<String^, Object^>^>^ SimpleClientShell::get_managed_doc_result(Document_result_set *doc)
{
  List<Dictionary<String^, Object^>^>^ result = gcnew List<Dictionary<String^, Object^>^>();
  boost::shared_ptr<std::vector<shcore::Value> > data = doc->get_data();
  std::vector<shcore::Value>::const_iterator myend = data->end();
  // a document is an array of objects of type mysh::Row
  int i = 0;
  for (std::vector<shcore::Value>::const_iterator it = data->begin(); it != myend; ++it, ++i)
  {
    Dictionary<String^, Object^>^ dic = gcnew Dictionary<String^, Object^>();
    if (it->type == shcore::String)
    {
      dic->Add((gcnew Int32(i))->ToString(), msclr::interop::marshal_as<String^>(it->as_string()));
      result->Add(dic);
      continue;
    }
    boost::shared_ptr<mysh::Row> row = it->as_object<mysh::Row>();
    for (size_t idx = 0; idx < row->value_iterators.size(); idx++)
    {
      Object^ o;
      shcore::Value& val = row->value_iterators[idx]->second;
      switch (val.type)
      {
      case shcore::Integer:
        o = gcnew Int32(val.as_int());
        break;
      case shcore::String:
        o = msclr::interop::marshal_as<String^>(val.as_string());
        break;
      case shcore::Bool:
        o = gcnew Boolean(val.as_bool());
        break;
      case shcore::Float:
        o = gcnew Double(val.as_double());
        break;
      default:
        o = msclr::interop::marshal_as<String^>(val.descr());
      }
      dic->Add(msclr::interop::marshal_as<String^>(row->value_iterators[idx]->first), o);
    }
    result->Add(dic);
  }
  return result;
}

List<array<Object^>^>^ SimpleClientShell::get_managed_table_result_set(Table_result_set* tbl)
{
  // a table result set is an array of shcore::Value's each of them in turn is an array (vector) of shcore::Value's (escalars)
  List<array<Object^>^>^ result = gcnew List<array<Object^>^>();
  std::vector<Result_set_metadata> *metadata = tbl->get_metadata().get();
  std::vector<shcore::Value> *dataset = tbl->get_data().get();

  for (int i = 0; i < dataset->size(); ++i)
  {
    shcore::Value& v_row = (*dataset)[i];
    boost::shared_ptr<mysh::Row> row = v_row.as_object<mysh::Row>();
    array<Object^>^ arr = gcnew array<Object^>(metadata->size());
    std::cout << std::endl;
    for (size_t i = 0; i < metadata->size(); i++)
    {
      shcore::Value& val = row->value_iterators[i]->second;
      Object^ o;
      switch (val.type)
      {
      case shcore::Integer:
        o = gcnew Int32(val.as_int());
        break;
      case shcore::String:
        o = msclr::interop::marshal_as<String^>(val.as_string());
        break;
      case shcore::Bool:
        o = gcnew Boolean(val.as_bool());
        break;
      case shcore::Float:
        o = gcnew Double(val.as_double());
        break;
      default:
        o = msclr::interop::marshal_as<String^>(val.descr());
      }
      arr->SetValue(o, (int)i);
    }
    result->Add(arr);
  }
  return result;
}

List<ResultSetMetadata^>^ SimpleClientShell::get_managed_metadata(Table_result_set* tbl)
{
  List<ResultSetMetadata^>^ result = gcnew List<ResultSetMetadata^>();
  std::vector<Result_set_metadata>* m = tbl->get_metadata().get();
  std::vector<Result_set_metadata>::const_iterator myend = m->end();
  for (std::vector<Result_set_metadata>::const_iterator it = m->begin(); it != myend; ++it)
  {
    ResultSetMetadata^ m_meta = gcnew ResultSetMetadata(
      msclr::interop::marshal_as<String ^>(it->get_catalog()),
      msclr::interop::marshal_as<String ^>(it->get_db()),
      msclr::interop::marshal_as<String ^>(it->get_table()),
      msclr::interop::marshal_as<String ^>(it->get_orig_table()),
      msclr::interop::marshal_as<String ^>(it->get_name()),
      msclr::interop::marshal_as<String ^>(it->get_orig_name()),
      gcnew Int32(it->get_charset()),
      gcnew Int32(it->get_length()),
      gcnew Int32(it->get_type()),
      gcnew Int32(it->get_flags()),
      gcnew Int32(it->get_decimal())
      );
    result->Add(m_meta);
  }

  return result;
}

ResultSet^ SimpleClientShell::Execute(String^ query)
{
  std::string n_query = msclr::interop::marshal_as<std::string>(query);
  boost::shared_ptr<Result_set> n_result = _obj->execute(n_query);

  Table_result_set* tbl = dynamic_cast<Table_result_set*>(n_result.get());
  Document_result_set* doc = dynamic_cast<Document_result_set*>(n_result.get());

  if (tbl != NULL)
  {
    List<array<Object^>^>^ data = get_managed_table_result_set(tbl);
    List<ResultSetMetadata^>^ meta = get_managed_metadata(tbl);

    TableResultSet^ table = gcnew TableResultSet(data, meta, gcnew Int64(tbl->get_affected_rows()), gcnew Int32(tbl->get_warning_count()), msclr::interop::marshal_as<String^>(tbl->get_execution_time()));
    return table;
  }
  else if (doc != NULL)
  {
    List<Dictionary<String^, Object^>^>^ data = get_managed_doc_result(doc);
    DocumentResultSet^ doc_rs = gcnew DocumentResultSet(data, gcnew Int64(-1), gcnew Int32(-1), "");
    return doc_rs;
  }
  else
  {
    ResultSet^ result = gcnew ResultSet(gcnew Int64(-1), gcnew Int32(-1), "");
    return result;
  }
}

SimpleClientShell::!SimpleClientShell()
{

}

SimpleClientShell::~SimpleClientShell()
{
  delete _obj;
  this->!SimpleClientShell();
}

void SimpleClientShell::Print(String^ text)
{

}

void SimpleClientShell::PrintError(String^ text)
{

}

bool SimpleClientShell::Input(String^ text, String^% ret)
{
  return false;
}

bool SimpleClientShell::Password(String^ text, String^% ret)
{
  return false;
}

void SimpleClientShell::Source(String^ module)
{

}