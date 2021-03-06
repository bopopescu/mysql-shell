/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "utils/utils_string.h"
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <random>

#ifdef _WIN32
#include <windows.h>
// Starting with Windows 8: MultiByteToWideChar is declared in Stringapiset.h.
// Before Windows 8, it was declared in Winnls.h.
#include <Stringapiset.h>
#else
#include <cwchar>
#endif

namespace shcore {

constexpr const char k_idchars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890";

std::string str_strip(const std::string &s, const std::string &chars) {
  size_t begin = s.find_first_not_of(chars);
  size_t end = s.find_last_not_of(chars);
  if (begin == std::string::npos) return std::string();
  return s.substr(begin, end - begin + 1);
}

std::string str_lstrip(const std::string &s, const std::string &chars) {
  size_t begin = s.find_first_not_of(chars);
  if (begin == std::string::npos) return std::string();
  return s.substr(begin);
}

std::string str_rstrip(const std::string &s, const std::string &chars) {
  size_t end = s.find_last_not_of(chars);
  if (end == std::string::npos) return std::string();
  return s.substr(0, end + 1);
}

std::string str_format(const char *formats, ...) {
  static const int kBufferSize = 256;
  std::string buffer;
  buffer.resize(kBufferSize);
  int len;
  va_list args;

#ifdef WIN32
  va_start(args, formats);
  len = _vscprintf(formats, args);
  va_end(args);
  if (len < 0) throw std::invalid_argument("Could not format string");
  buffer.resize(len + 1);
  va_start(args, formats);
  len = vsnprintf(&buffer[0], buffer.size(), formats, args);
  va_end(args);
  if (len < 0) throw std::invalid_argument("Could not format string");
  buffer.resize(len);
#else
  va_start(args, formats);
  len = vsnprintf(&buffer[0], buffer.size(), formats, args);
  va_end(args);
  if (len < 0) throw std::invalid_argument("Could not format string");
  if (len + 1 >= kBufferSize) {
    buffer.resize(len + 1);
    va_start(args, formats);
    len = vsnprintf(&buffer[0], buffer.size(), formats, args);
    va_end(args);
    if (len < 0) throw std::invalid_argument("Could not format string");
  }
  buffer.resize(len);
#endif

  return buffer;
}

std::string str_replace(const std::string &s, const std::string &from,
                        const std::string &to) {
  std::string str;
  int offs = from.length();
  str.reserve(s.length());

  if (from.empty()) {
    str.append(to);
    for (char c : s) {
      str.push_back(c);
      str.append(to);
    }
  } else {
    std::string::size_type start = 0, p = s.find(from);
    while (p != std::string::npos) {
      if (p > start) str.append(s, start, p - start);
      str.append(to);
      start = p + offs;
      p = s.find(from, start);
    }
    if (start < s.length()) str.append(s, start, s.length() - start);
  }
  return str;
}

static const char *k_bits[256] = {
    "00000000", "00000001", "00000010", "00000011", "00000100", "00000101",
    "00000110", "00000111", "00001000", "00001001", "00001010", "00001011",
    "00001100", "00001101", "00001110", "00001111", "00010000", "00010001",
    "00010010", "00010011", "00010100", "00010101", "00010110", "00010111",
    "00011000", "00011001", "00011010", "00011011", "00011100", "00011101",
    "00011110", "00011111", "00100000", "00100001", "00100010", "00100011",
    "00100100", "00100101", "00100110", "00100111", "00101000", "00101001",
    "00101010", "00101011", "00101100", "00101101", "00101110", "00101111",
    "00110000", "00110001", "00110010", "00110011", "00110100", "00110101",
    "00110110", "00110111", "00111000", "00111001", "00111010", "00111011",
    "00111100", "00111101", "00111110", "00111111", "01000000", "01000001",
    "01000010", "01000011", "01000100", "01000101", "01000110", "01000111",
    "01001000", "01001001", "01001010", "01001011", "01001100", "01001101",
    "01001110", "01001111", "01010000", "01010001", "01010010", "01010011",
    "01010100", "01010101", "01010110", "01010111", "01011000", "01011001",
    "01011010", "01011011", "01011100", "01011101", "01011110", "01011111",
    "01100000", "01100001", "01100010", "01100011", "01100100", "01100101",
    "01100110", "01100111", "01101000", "01101001", "01101010", "01101011",
    "01101100", "01101101", "01101110", "01101111", "01110000", "01110001",
    "01110010", "01110011", "01110100", "01110101", "01110110", "01110111",
    "01111000", "01111001", "01111010", "01111011", "01111100", "01111101",
    "01111110", "01111111", "10000000", "10000001", "10000010", "10000011",
    "10000100", "10000101", "10000110", "10000111", "10001000", "10001001",
    "10001010", "10001011", "10001100", "10001101", "10001110", "10001111",
    "10010000", "10010001", "10010010", "10010011", "10010100", "10010101",
    "10010110", "10010111", "10011000", "10011001", "10011010", "10011011",
    "10011100", "10011101", "10011110", "10011111", "10100000", "10100001",
    "10100010", "10100011", "10100100", "10100101", "10100110", "10100111",
    "10101000", "10101001", "10101010", "10101011", "10101100", "10101101",
    "10101110", "10101111", "10110000", "10110001", "10110010", "10110011",
    "10110100", "10110101", "10110110", "10110111", "10111000", "10111001",
    "10111010", "10111011", "10111100", "10111101", "10111110", "10111111",
    "11000000", "11000001", "11000010", "11000011", "11000100", "11000101",
    "11000110", "11000111", "11001000", "11001001", "11001010", "11001011",
    "11001100", "11001101", "11001110", "11001111", "11010000", "11010001",
    "11010010", "11010011", "11010100", "11010101", "11010110", "11010111",
    "11011000", "11011001", "11011010", "11011011", "11011100", "11011101",
    "11011110", "11011111", "11100000", "11100001", "11100010", "11100011",
    "11100100", "11100101", "11100110", "11100111", "11101000", "11101001",
    "11101010", "11101011", "11101100", "11101101", "11101110", "11101111",
    "11110000", "11110001", "11110010", "11110011", "11110100", "11110101",
    "11110110", "11110111", "11111000", "11111001", "11111010", "11111011",
    "11111100", "11111101", "11111110", "11111111"};

std::string bits_to_string(uint64_t bits, int nbits) {
  std::string r;
  std::div_t length = std::div(nbits, 8);

  switch (length.quot) {
    case 8:
      r.append(k_bits[(bits >> 56) & 0xff]);
      // fallthrough
    case 7:
      r.append(k_bits[(bits >> 48) & 0xff]);
      // fallthrough
    case 6:
      r.append(k_bits[(bits >> 40) & 0xff]);
      // fallthrough
    case 5:
      r.append(k_bits[(bits >> 32) & 0xff]);
      // fallthrough
    case 4:
      r.append(k_bits[(bits >> 24) & 0xff]);
      // fallthrough
    case 3:
      r.append(k_bits[(bits >> 16) & 0xff]);
      // fallthrough
    case 2:
      r.append(k_bits[(bits >> 16) & 0xff]);
      // fallthrough
    case 1:
      r.append(k_bits[(bits >> 8) & 0xff]);
      // fallthrough
    case 0:
      r.append(k_bits[bits & 0xff]);
      break;
  }
  r = r.substr(8 - length.rem);
  return r;
}

std::pair<uint64_t, int> string_to_bits(const std::string &s) {
  int nbits = s.length();
  if (nbits > 64)
    throw std::invalid_argument("bit string length must be <= 64");
  std::bitset<64> bits(s);
  return {bits.to_ullong(), nbits};
}

std::string quote_string(const std::string &s, char quote) {
  const std::string q{quote};
  const std::string backslash = str_replace(s, "\\", "\\\\");
  const std::string esc = str_replace(backslash, q, "\\" + q);
  return std::string(q + esc + q);
}

std::string unquote_string(const std::string &s, char quote) {
  const std::string q{quote};
  auto result = s;

  if (result.length() >= 2 && result[0] == quote &&
      result[result.length() - 1] == quote) {
    result = result.substr(1, result.length() - 2);
  }

  result = shcore::str_replace(result, "\\" + q, q);
  result = shcore::str_replace(result, "\\\\", "\\");

  return result;
}

std::vector<std::string> str_break_into_lines(const std::string &line,
                                              std::size_t line_width) {
  std::vector<std::string> result;
  std::string rem(line);
  std::string::size_type nl_pos = std::string::npos;
  std::string::size_type split_point = std::string::npos;

  while (rem.length() > line_width ||
         (nl_pos = rem.find('\n')) != std::string::npos) {
    if (nl_pos != std::string::npos) {
      split_point = nl_pos;
    } else {
      split_point = line_width - 1;
      while (!std::isspace(rem[split_point]) && split_point > 0) --split_point;

      if (split_point == 0 && !std::isspace(rem[0])) {
        for (split_point = line_width; split_point < rem.length();
             split_point++)
          if (std::isspace(rem[split_point])) break;
        if (split_point == rem.length()) break;
      } else {
        for (int i = split_point - 1; i >= 0; --i)
          if (rem[i] == '\n') split_point = i;
      }
    }

    if (split_point == 0)
      result.push_back("");
    else
      result.push_back(rem.substr(0, split_point));
    rem = split_point + 1 >= rem.length() ? std::string()
                                          : rem.substr(split_point + 1);
  }
  if (!rem.empty()) result.push_back(rem);
  return result;
}

std::pair<std::string::size_type, std::string::size_type> get_quote_span(
    const char quote_char, const std::string &str) {
  bool escaped = false;

  // if string has less than 2 chars  we assume matching quotes were not found.
  if (str.size() < 2)
    return std::make_pair(std::string::npos, std::string::npos);

  std::string::size_type open_quote_pos = 0;

  // find opening quote char
  for (std::string::size_type i = 0, end = str.size();
       (str[i] != quote_char || escaped) && i < end; ++i) {
    open_quote_pos++;
    escaped = (str[i]) == '\\' && !escaped;
  }
  // if no quotes were found
  if (open_quote_pos == str.size())
    return std::make_pair(std::string::npos, std::string::npos);

  // find closing quote char
  std::string::size_type close_quote_pos = open_quote_pos + 1;
  for (std::string::size_type i = close_quote_pos, end = str.size();
       (str[i] != quote_char || escaped) && i < end; ++i) {
    close_quote_pos++;
    escaped = (str[i]) == '\\' && !escaped;
  }

  // closing quote was not found
  if (close_quote_pos == str.size()) close_quote_pos = std::string::npos;
  return std::make_pair(open_quote_pos, close_quote_pos);
}

std::string str_subvars(
    const std::string &s,
    const std::function<std::string(const std::string &)> &subvar,
    const std::string &var_begin, const std::string &var_end) {
  assert(var_begin.size() > 0);
  std::string out_s;

  std::string::size_type p0 = 0;
  while (p0 != std::string::npos) {
    auto pos = s.find(var_begin, p0);
    if (pos == std::string::npos) {
      out_s.append(s.substr(p0));
      break;
    } else {
      out_s.append(s.substr(p0, pos - p0));
    }
    pos += var_begin.size();
    std::string::size_type p1;
    if (var_end.empty()) {
      p1 = s.find_first_not_of(k_idchars, pos);
    } else {
      p1 = s.find(var_end, pos);
      if (p1 == std::string::npos) return out_s.append(s.substr(pos));
    }
    if (p1 == std::string::npos) {
      out_s.append(subvar(s.substr(pos)));
    } else {
      out_s.append(subvar(s.substr(pos, p1 - pos)));
      p1 += var_end.size();
    }
    p0 = p1;
  }
  return out_s;
}

std::wstring utf8_to_wide(const std::string &utf8) {
  return utf8_to_wide(&utf8[0], utf8.size());
}

std::wstring utf8_to_wide(const char *utf8) {
  return utf8_to_wide(utf8, strlen(utf8));
}

std::string wide_to_utf8(const std::wstring &wide) {
  return wide_to_utf8(&wide[0], wide.size());
}

std::string wide_to_utf8(const wchar_t *wide) {
  return wide_to_utf8(wide, wcslen(wide));
}

#ifdef _WIN32

std::wstring utf8_to_wide(const char *utf8, const size_t utf8_length) {
  auto buffer_size_needed =
      MultiByteToWideChar(CP_UTF8, 0, utf8, utf8_length, nullptr, 0);
  std::wstring wide_string(buffer_size_needed, 0);
  const auto wide_string_size = MultiByteToWideChar(
      CP_UTF8, 0, utf8, utf8_length, &wide_string[0], buffer_size_needed);
  wide_string.resize(wide_string_size);
  return wide_string;
}

std::string wide_to_utf8(const wchar_t *wide, const size_t wide_length) {
  auto string_size = WideCharToMultiByte(CP_UTF8, 0, wide, wide_length, nullptr,
                                         0, nullptr, nullptr);
  std::string result_string(string_size, 0);
  WideCharToMultiByte(CP_UTF8, 0, wide, wide_length, &result_string[0],
                      string_size, nullptr, nullptr);
  return result_string;
}

#else

std::wstring utf8_to_wide(const char *utf8, const size_t utf8_length) {
  std::mbstate_t state{};
  const char *end = utf8 + utf8_length;
  int len = 0;
  std::wstring result;
  wchar_t wc;

  while ((len = std::mbrtowc(&wc, utf8, end - utf8, &state)) > 0) {
    result += wc;
    utf8 += len;
  }

  return result;
}

std::string wide_to_utf8(const wchar_t *wide, const size_t wide_length) {
  std::mbstate_t state{};
  std::string result;
  std::string mb(MB_CUR_MAX, '\0');

  for (size_t i = 0; i < wide_length; ++i) {
    const auto len = std::wcrtomb(&mb[0], wide[i], &state);
    result.append(mb.c_str(), len);
  }

  return result;
}

#endif

std::string truncate(const std::string &str, const size_t max_length) {
  return truncate(str.c_str(), str.length(), max_length);
}

std::string truncate(const char *str, const size_t length,
                     const size_t max_length) {
  return wide_to_utf8(truncate(utf8_to_wide(str, length), max_length));
}

std::wstring truncate(const std::wstring &str, const size_t max_length) {
  return truncate(str.c_str(), str.length(), max_length);
}

std::wstring truncate(const wchar_t *str, const size_t length,
                      const size_t max_length) {
#if (WCHAR_MAX + 0) <= 0xffff
  // UTF-16
  std::wstring truncated;
  std::size_t idx = 0;
  std::size_t truncated_length = 0;

  while (truncated_length < max_length && idx < length) {
    // detect high surrogate
    if (0xD800 == (str[idx] & 0xFC00)) {
      if (idx + 1 == length) {
        // no low surrogate, finish here
        break;
      }

      truncated.append(1, str[idx++]);
    }

    // low surrogate or a code point
    truncated.append(1, str[idx++]);
    ++truncated_length;
  }

  return truncated;
#else
  // UTF-32
  return std::wstring(str, std::min(length, max_length));
#endif
}

bool is_valid_utf8(const std::string &s) {
  auto c = reinterpret_cast<const unsigned char *>(s.c_str());
  const auto end = c + s.length();
  uint32_t cp = 0;
  size_t bytes = 0;

  while (c < end) {
    if (0x00 == (*c & 0x80)) {
      // 0xxxxxxx, U+0000 - U+007F
      bytes = 1;
      cp = *c & 0x7F;
    } else if (0xC0 == (*c & 0xE0)) {
      // 110xxxxx, U+0080 - U+07FF
      bytes = 2;
      cp = *c & 0x1F;
    } else if (0xE0 == (*c & 0xF0)) {
      // 1110xxxx, U+0800 - U+FFFF
      bytes = 3;
      cp = *c & 0x0F;
    } else if (0xF0 == (*c & 0xF8)) {
      // 11110xxx, U+10000 - U+10FFFF
      bytes = 4;
      cp = *c & 0x07;
    } else {
      return false;
    }

    // advance one byte
    ++c;

    for (size_t b = 1; b < bytes; ++b) {
      // each byte should be: 10xxxxxx
      if (0x80 != (*c & 0xC0)) {
        return false;
      }

      cp = (cp << 6) | (*c & 0x3F);

      // advance one byte
      ++c;
    }

    // invalid code points
    if ((cp <= 0x7F && 1 != bytes) ||  // overlong encoding
        (cp >= 0x80 && cp <= 0x07FF && 2 != bytes) ||
        (cp >= 0x0800 && cp <= 0xFFFF && 3 != bytes) ||
        (cp >= 0x10000 && cp <= 0x10FFFF && 4 != bytes) ||
        (cp >= 0xD800 && cp <= 0xDFFF) ||  // UTF-16 surrogate halves
        cp > 0x10FFFF) {                   // not encodable by UTF-16
      return false;
    }
  }

  return true;
}

namespace {

// Byte-values that are reserved and must be hex-encoded [0..255]
static const int k_reserved_chars[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// Numeric values for hex-digits [0..127]
static const int k_hex_values[] = {
    0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,
    0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,
    0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4, 5, 6, 7, 8,
    9, 0, 0,  0,  0,  0,  0,  0,  10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0,
    0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,
    0, 0, 10, 11, 12, 13, 14, 15, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,
    0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

}  // namespace

std::string pctencode(const std::string &s) {
  std::string enc;
  size_t offs = 0;

  enc.resize(s.size() * 3);

  for (unsigned char c : s) {
    unsigned int i = static_cast<unsigned int>(c);
    if (k_reserved_chars[i]) {
      sprintf(&enc[offs], "%%%02X", i);
      offs += 3;
    } else {
      enc[offs] = c;
      ++offs;
    }
  }

  enc.resize(offs);

  return enc;
}

std::string pctdecode(const std::string &s) {
  std::string dec;

  dec.reserve(s.size());

  for (size_t i = 0, c = s.size(); i < c;) {
    if (i <= c - 3 && s[i] == '%' && isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
      int ch = k_hex_values[static_cast<int>(s[i + 1])] << 4 |
               k_hex_values[static_cast<int>(s[i + 2])];
      dec.push_back(ch);
      i += 3;
    } else {
      dec.push_back(s[i]);
      ++i;
    }
  }

  return dec;
}

std::string get_random_string(size_t size, const char *source) {
  std::random_device rd;
  std::string data;

  std::uniform_int_distribution<int> dist_num(0, strlen(source) - 1);

  for (size_t i = 0; i < size; i++) {
    char random = source[dist_num(rd)];

    // Make sure there are no consecutive values
    if (i == 0) {
      data += random;
    } else {
      if (random != data[i - 1])
        data += random;
      else
        i--;
    }
  }

  return data;
}

}  // namespace shcore
