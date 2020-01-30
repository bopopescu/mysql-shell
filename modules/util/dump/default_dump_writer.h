/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_DUMP_DEFAULT_DUMP_WRITER_H_
#define MODULES_UTIL_DUMP_DEFAULT_DUMP_WRITER_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/util/dump/dump_writer.h"

namespace mysqlsh {
namespace dump {

class Default_dump_writer : public Dump_writer {
 public:
  Default_dump_writer() = delete;
  explicit Default_dump_writer(std::unique_ptr<mysqlshdk::storage::IFile> out);

  Default_dump_writer(const Default_dump_writer &) = delete;
  Default_dump_writer(Default_dump_writer &&) = default;

  Default_dump_writer &operator=(const Default_dump_writer &) = delete;
  Default_dump_writer &operator=(Default_dump_writer &&) = default;

  ~Default_dump_writer() override = default;

 private:
  void store_preamble(
      const std::vector<mysqlshdk::db::Column> &metadata) override;

  void store_row(const mysqlshdk::db::IRow *row) override;

  void store_postamble() override;

  void read_metadata(const std::vector<mysqlshdk::db::Column> &metadata);

  void store_field(const mysqlshdk::db::IRow *row, uint32_t idx);

  void store_null();

  void finish_row();

  uint32_t m_num_fields;

  // not using vectors of bool here, as they are not very efficient on access
  std::vector<int> m_is_string_type;

  std::vector<int> m_is_number_type;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DEFAULT_DUMP_WRITER_H_
