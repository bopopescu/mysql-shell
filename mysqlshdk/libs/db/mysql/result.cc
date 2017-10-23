/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/mysql/result.h"

#include <cstdlib>
#include <string>
#include <utility>

#include "mysqlshdk/libs/db/mysql/row.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
Result::Result(std::shared_ptr<mysqlshdk::db::mysql::Session_impl> owner,
               uint64_t affected_rows_, unsigned int warning_count_,
               uint64_t last_insert_id, const char *info_)
    : _session(owner),
      _affected_rows(affected_rows_),
      _last_insert_id(last_insert_id),
      _warning_count(warning_count_),
      _fetched_row_count(0),
      _has_resultset(false) {
  if (info_)
    _info.assign(info_);
}

void Result::fetch_metadata() {
  int num_fields = 0;

  _metadata.clear();

  // res could be NULL on queries not returning data
  std::shared_ptr<MYSQL_RES> res = _result.lock();

  if (res) {
    num_fields = mysql_num_fields(res.get());
    MYSQL_FIELD *fields = mysql_fetch_fields(res.get());

    for (int index = 0; index < num_fields; index++) {
      _metadata.push_back(mysqlshdk::db::Column(
          fields[index].db, fields[index].org_table, fields[index].table,
          fields[index].org_name, fields[index].name, fields[index].length,
          fields[index].decimals,
          map_data_type(fields[index].type, fields[index].flags),
          fields[index].charsetnr, bool(fields[index].flags & UNSIGNED_FLAG),
          bool(fields[index].flags & ZEROFILL_FLAG),
          bool(fields[index].flags & BINARY_FLAG)));
    }
  }
}

Result::~Result() {
}

const IRow *Result::fetch_one() {
  _row.reset();
  if (has_resultset()) {
    // Loads the first row
    std::shared_ptr<MYSQL_RES> res = _result.lock();

    if (res) {
      MYSQL_ROW mysql_row = mysql_fetch_row(res.get());
      if (mysql_row) {
        unsigned long *lengths;
        lengths = mysql_fetch_lengths(res.get());

        _row.reset(new Row(shared_from_this(), mysql_row, lengths));

        // Each read row increases the count
        _fetched_row_count++;
      } else {
        if (auto session = _session.lock()) {
          int code = 0;
          const char *state;
          const char *err = session->get_last_error(&code, &state);
          if (code != 0)
            throw shcore::Exception::mysql_error_with_code_and_state(err, code,
                                                                    state);
        }
      }
    }
  }
  return _row.get();
}

bool Result::next_resultset() {
  bool ret_val = false;

  if (auto s = _session.lock())
    ret_val = s->next_resultset();

  _fetched_row_count = 0;

  return ret_val;
}

std::unique_ptr<Warning> Result::fetch_one_warning() {
  if (_warning_count && !_fetched_warnings) {
    _fetched_warnings = true;
    if (auto s = _session.lock()) {
      auto result = s->query("show warnings", true);
      while (auto row = result->fetch_one()) {
        std::unique_ptr<Warning> w(new Warning());
        std::string level = row->get_string(0);
        if (level == "Error") {
          w->level = Warning::Level::Error;
        } else if (level == "Warning") {
          w->level = Warning::Level::Warn;
        } else {
          assert(level == "Note");
          w->level = Warning::Level::Note;
        }
        w->code = row->get_int(1);
        w->msg = row->get_string(2);
        _warnings.push_back(std::move(w));
      }
    }
  }

  if (!_warnings.empty()) {
    auto tmp = std::move(_warnings.front());
    _warnings.pop_front();
    return tmp;
  }
  return {};
}

void Result::reset(std::shared_ptr<MYSQL_RES> res) {
  _has_resultset = false;

  if (res)
    _has_resultset = true;

  _result = res;
}

Type Result::map_data_type(int raw_type, int flags) {
  switch (raw_type) {
    case MYSQL_TYPE_NULL:
      return Type::Null;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      return Type::Decimal;
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_NEWDATE:
      return Type::Date;
    case MYSQL_TYPE_TIME2:
    case MYSQL_TYPE_TIME:
      return Type::Time;
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
      if (flags & ENUM_FLAG)
        return Type::Enum;
      else if (flags & SET_FLAG)
        return Type::Set;
      else
        return Type::String;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return Type::Bytes;
    case MYSQL_TYPE_GEOMETRY:
      return Type::Geometry;
    case MYSQL_TYPE_JSON:
      return Type::Json;
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_LONGLONG:
      return (flags & UNSIGNED_FLAG) ? Type::UInteger : Type::Integer;
    case MYSQL_TYPE_FLOAT:
      return Type::Float;
    case MYSQL_TYPE_DOUBLE:
      return Type::Double;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME2:
    case MYSQL_TYPE_TIMESTAMP2:
      // The difference between TIMESTAMP and DATETIME is entirely in terms
      // of internal representation at the server side. At the client side,
      // there is no difference.
      // TIMESTAMP is the number of seconds since epoch, so it cannot store
      // dates before 1970. DATETIME is an arbitrary date and time value,
      // so it does not have that limitation.
      return Type::DateTime;
    case MYSQL_TYPE_BIT:
      return Type::Bit;
    case MYSQL_TYPE_ENUM:
      return Type::Enum;
    case MYSQL_TYPE_SET:
      return Type::Set;
  }
  throw std::logic_error("Invalid type");
}
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk