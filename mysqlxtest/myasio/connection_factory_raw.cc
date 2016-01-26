/*
 * Copyright (c) 2015, 2016 Oracle and/or its affiliates. All rights reserved.
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
 
#include "myasio/connection_factory_raw.h"

#include "myasio/connection_raw.h"

namespace ngs
{

  IConnection_unique_ptr Connection_raw_factory::create_connection(boost::asio::io_service &io_service)
  {
    return IConnection_unique_ptr(new Connection_raw<boost::asio::ip::tcp::socket>(io_service));
  }

  IOptions_context_ptr Connection_raw_factory::create_ssl_context_options()
  {
    return IOptions_context_ptr();
  }

} // namespace ngs

