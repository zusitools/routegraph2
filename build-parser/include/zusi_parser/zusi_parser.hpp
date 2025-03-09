#pragma once
#include "zusixml.hpp"
#include "zusi_parser/zusi_types.hpp"
#include <array>
#include <cstring>  // for memcmp
#include <cfloat>   // Workaround for https://svn.boost.org/trac10/ticket/12642
#include <iostream> // for cerr
#include <string_view>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/version.hpp>
template <typename T>
struct decimal_comma_real_policies : boost::spirit::qi::real_policies<T>
{
    template <typename Iterator> static bool parse_dot(Iterator& first, Iterator const& last)
    {
        if (first == last) {
            return false;
        }

        const auto ch = *first;
        if (ch != ',' && ch != '.') {
            return false;
        }

        ++first;
        return true;
    }
};

namespace zusixml {

static void parse_string(const Ch*& text, std::string& result, Ch quote) {
  const Ch* const value = text;
  skip_attribute_value_pure(text, quote);
  if (*text == quote) {
    // No character refs in attribute value, copy the string verbatim
    result = std::string(value, text - value);
  } else if (*text == Ch('&')) {
    const Ch* first_ampersand = text;
    skip_attribute_value(text, quote);
    result.resize(text - value);
    // Copy characters until the first ampersand verbatim, use copy_and_expand_character_refs for the rest.
    memcpy(&result[0], value, first_ampersand - value);
    result.resize(first_ampersand - value + copy_and_expand_character_refs(first_ampersand, &result[first_ampersand - value], quote));
  }
  // else: *text == '\0'
}

static void parse_float(const Ch*& text, float& result) {
  const Ch* text_save = text;
  // Fast path for numbers of the form "-XXX.YYY" (enclosed in double quotes), where X and Y are both <= 7 characters long and the minus sign is optional
  bool neg = false;
  if (*text == Ch('-')) {
    neg = true;
    ++text;  // Skip "-"
  }
  const Ch* const integer_start = text;
  for (size_t i = 0; i < 7; i++) {
    if (!digit_pred::test(*text)) {
      break;
    }
    ++text;
  }
  const Ch* const dot_and_fractional_start = text;
  if (*text == Ch('.')) {
    ++text;  // skip "."
    for (size_t i = 0; i < 7; i++) {
      if (!digit_pred::test(*text)) {
        break;
      }
      ++text;
    }
  }

  if (*text == Ch('"')) {
    size_t len_integer = dot_and_fractional_start - integer_start;
    size_t len_dot_and_fractional = text - dot_and_fractional_start;

    uint32_t result_integer = 0;
    switch(len_integer) {
      case 7: result_integer += (*(integer_start + len_integer - 7) - '0') * 1000000; [[fallthrough]];
      case 6: result_integer += (*(integer_start + len_integer - 6) - '0') * 100000; [[fallthrough]];
      case 5: result_integer += (*(integer_start + len_integer - 5) - '0') * 10000; [[fallthrough]];
      case 4: result_integer += (*(integer_start + len_integer - 4) - '0') * 1000; [[fallthrough]];
      case 3: result_integer += (*(integer_start + len_integer - 3) - '0') * 100; [[fallthrough]];
      case 2: result_integer += (*(integer_start + len_integer - 2) - '0') * 10; [[fallthrough]];
      case 1: result_integer += (*(integer_start + len_integer - 1) - '0') * 1; [[fallthrough]];
      case 0: break;
    }

    if (len_dot_and_fractional > 0) {
      constexpr float scaleFactors[] = { 1E0, /* len_dot_and_fractional == 1 */ 1E0, /* == 2 etc. */ 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7 };
      uint32_t result_fractional = 0;
      switch(len_dot_and_fractional - 1) {
        case 7: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 7) - '0') * 1000000; [[fallthrough]];
        case 6: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 6) - '0') * 100000; [[fallthrough]];
        case 5: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 5) - '0') * 10000; [[fallthrough]];
        case 4: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 4) - '0') * 1000; [[fallthrough]];
        case 3: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 3) - '0') * 100; [[fallthrough]];
        case 2: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 2) - '0') * 10; [[fallthrough]];
        case 1: result_fractional += (*(dot_and_fractional_start + len_dot_and_fractional - 1) - '0') * 1; [[fallthrough]];
        case 0: break;
      }
      result = result_integer + result_fractional / scaleFactors[len_dot_and_fractional];
    } else {
      result = result_integer;
    }
    if (neg) {
      result = -result;
    }
    return;
  }

  // Slow path for everything else
  text = text_save;
  // std::cerr << "Delegating to slow float parser: " << std::string_view(text, 20) << "\n";
  boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::real_parser<float, decimal_comma_real_policies<float> >(), result);
}

template<Ch Quote>
static bool parse_datetime(const Ch*& text, struct tm& result) {
  // Delphi (and Zusi) accept a very wide range of things here,
  // e.g. two-digit years, times that don't specify seconds or minutes, etc.
  // We are more restrictive: we parse a date yyyy-mm-dd, or a time hh:nn:ss,
  // or both separated by a blank,

  const Ch* prev = text;
  skip_max<digit_pred, 4>(text);

  if (*text == Ch('-')) {
    // date
    const size_t year_len = text - prev;
    int year = 0;
    switch (year_len) {
      case 4: year += (*(prev + (year_len-4)) - '0') * 1000; [[fallthrough]];
      case 3: year += (*(prev + (year_len-3)) - '0') * 100; [[fallthrough]];
      case 2: year += (*(prev + (year_len-2)) - '0') * 10; [[fallthrough]];
      case 1: year += (*(prev + (year_len-1)) - '0') * 1; break;
      default: return false;
    }

    result.tm_year = year - 1900;

    ++text;
    prev = text;
    skip_max<digit_pred, 2>(text);

    if (*text != Ch('-')) {
      return false;
    }

    const size_t month_len = text - prev;
    int month = 0;
    switch (month_len) {
      case 2: month += (*(prev + (month_len-2)) - '0') * 10; [[fallthrough]];
      case 1: month += (*(prev + (month_len-1)) - '0') * 1; break;
      default: return false;
    }

    result.tm_mon = month;

    ++text;
    prev = text;
    skip_max<digit_pred, 2>(text);

    const size_t day_len = text - prev;
    int day = 0;
    switch (day_len) {
      case 2: day += (*(prev + (day_len-2)) - '0') * 10; [[fallthrough]];
      case 1: day += (*(prev + (day_len-1)) - '0') * 1; break;
      default: return false;
    }

    result.tm_mday = day;

    if (*text == Quote) {
      return true;
    } else if (*text == Ch(' ')) {
      ++text;
      prev = text;
      skip_max<digit_pred, 2>(text);
    }
  }

  if (*text == Ch(':')) {
    const size_t hour_len = text - prev;
    int hour = 0;
    switch (hour_len) {
      case 4: hour += (*(prev + (hour_len-4)) - '0') * 1000; [[fallthrough]];
      case 3: hour += (*(prev + (hour_len-3)) - '0') * 100; [[fallthrough]];
      case 2: hour += (*(prev + (hour_len-2)) - '0') * 10; [[fallthrough]];
      case 1: hour += (*(prev + (hour_len-1)) - '0') * 1; break;
      default: return false;
    }

    result.tm_hour = hour;

    ++text;
    prev = text;
    skip_max<digit_pred, 2>(text);

    if (*text != Ch(':')) {
      return false;
    }

    const size_t minute_len = text - prev;
    int minute = 0;
    switch (minute_len) {
      case 2: minute += (*(prev + (minute_len-2)) - '0') * 10; [[fallthrough]];
      case 1: minute += (*(prev + (minute_len-1)) - '0') * 1; break;
      default: return false;
    }

    result.tm_min = minute;

    ++text;
    prev = text;
    skip_max<digit_pred, 2>(text);

    const size_t second_len = text - prev;
    int second = 0;
    switch (second_len) {
      case 2: second += (*(prev + (second_len-2)) - '0') * 10; [[fallthrough]];
      case 1: second += (*(prev + (second_len-1)) - '0') * 1; break;
      default: return false;
    }

    result.tm_sec = second;
  }

  return true;
}

#define RAPIDXML_PARSE_ERROR(what, where) throw parse_error(what, where)

[[noreturn]] static void parse_error_expected_semicolon(const Ch*& text) {
  RAPIDXML_PARSE_ERROR("expected ';'", text);
}
[[noreturn]] static void parse_error_expected_equals(const Ch*& text) {
  RAPIDXML_PARSE_ERROR("expected '='", text);
}
[[noreturn]] static void parse_error_expected_quote(const Ch*& text) {
  RAPIDXML_PARSE_ERROR("expected ' or \"", text);
}
[[noreturn]] static void parse_error_expected_tag_end(const Ch*& text) {
  RAPIDXML_PARSE_ERROR("expected > or />", text);
}
[[noreturn]] static void parse_error_expected_element_name(const Ch*& text) {
  RAPIDXML_PARSE_ERROR("expected element name", text);
}
[[noreturn]] static void parse_error_value_too_long(const Ch*& text) {
  RAPIDXML_PARSE_ERROR("value too long", text);
}

  static void parse_element_StrModul(const Ch *& text, struct StrModul* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct StrModul* parseResult = static_cast<struct StrModul*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_Vec3(const Ch *& text, struct Vec3* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        if (name_size == 1 && *name >= 'X' && *name <= 'Z') {
          std::array<float Vec3::*, 3> members = {{ &Vec3::X, &Vec3::Y, &Vec3::Z }};
          parse_float(text, parseResult->*members[*name - 'X']);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct Vec3* parseResult = static_cast<struct Vec3*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_Zusi(const Ch *& text, struct Zusi* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct Zusi* parseResult = static_cast<struct Zusi*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 8 && !memcmp(name, "Fahrplan", 8)) {
                  std::unique_ptr<struct Fahrplan, zusixml::deleter<struct Fahrplan>> childResult(new struct Fahrplan());
  parseResult->Fahrplan.swap(childResult);
  parse_element_Fahrplan(text, parseResult->Fahrplan.get());
            }
            else if (name_size == 7 && !memcmp(name, "Strecke", 7)) {
                  std::unique_ptr<struct Strecke, zusixml::deleter<struct Strecke>> childResult(new struct Strecke());
  parseResult->Strecke.swap(childResult);
  parse_element_Strecke(text, parseResult->Strecke.get());
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_Dateiverknuepfung(const Ch *& text, struct Dateiverknuepfung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 9 && !memcmp(name, "Dateiname", 9)) {
          parse_string(text, parseResult->Dateiname, quote);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct Dateiverknuepfung* parseResult = static_cast<struct Dateiverknuepfung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_Signal(const Ch *& text, struct Signal* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 18 && !memcmp(name, "NameBetriebsstelle", 18)) {
          parse_string(text, parseResult->NameBetriebsstelle, quote);
        }
        else if (name_size == 9 && !memcmp(name, "Stellwerk", 9)) {
          parse_string(text, parseResult->Stellwerk, quote);
        }
        else if (name_size == 10 && !memcmp(name, "Signalname", 10)) {
          parse_string(text, parseResult->Signalname, quote);
        }
        else if (name_size == 9 && !memcmp(name, "SignalTyp", 9)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->SignalTyp);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct Signal* parseResult = static_cast<struct Signal*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_Fahrplan(const Ch *& text, struct Fahrplan* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct Fahrplan* parseResult = static_cast<struct Fahrplan*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 8 && !memcmp(name, "StrModul", 8)) {
                  parse_element_StrModul(text, parseResult->children_StrModul.emplace_back(new struct StrModul()).get());
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_NachfolgerSelbesModul(const Ch *& text, struct NachfolgerSelbesModul* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 2 && !memcmp(name, "Nr", 2)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Nr);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct NachfolgerSelbesModul* parseResult = static_cast<struct NachfolgerSelbesModul*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_UTM(const Ch *& text, struct UTM* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 6 && !memcmp(name, "UTM_WE", 6)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->UTM_WE);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 6 && !memcmp(name, "UTM_NS", 6)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->UTM_NS);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 8 && !memcmp(name, "UTM_Zone", 8)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->UTM_Zone);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct UTM* parseResult = static_cast<struct UTM*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_StreckenelementRichtungsInfo(const Ch *& text, struct StreckenelementRichtungsInfo* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 4 && !memcmp(name, "vMax", 4)) {
          parse_float(text, parseResult->vMax);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct StreckenelementRichtungsInfo* parseResult = static_cast<struct StreckenelementRichtungsInfo*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 6 && !memcmp(name, "Signal", 6)) {
                  std::unique_ptr<struct Signal, zusixml::deleter<struct Signal>> childResult(new struct Signal());
  parseResult->Signal.swap(childResult);
  parse_element_Signal(text, parseResult->Signal.get());
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_Strecke(const Ch *& text, struct Strecke* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct Strecke* parseResult = static_cast<struct Strecke*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 3 && !memcmp(name, "UTM", 3)) {
                  std::unique_ptr<struct UTM, zusixml::deleter<struct UTM>> childResult(new struct UTM());
  parseResult->UTM.swap(childResult);
  parse_element_UTM(text, parseResult->UTM.get());
            }
            else if (name_size == 16 && !memcmp(name, "ReferenzElemente", 16)) {
                  std::unique_ptr<ReferenzElement> childResult(new ReferenzElement());
  parse_element_ReferenzElement(text, childResult.get());
  size_t index = childResult->ReferenzNr;
  if (index == parseResult->children_ReferenzElemente.size()) {
    parseResult->children_ReferenzElemente.push_back(std::move(childResult));
  } else {
    if (index > parseResult->children_ReferenzElemente.size()) {
      parseResult->children_ReferenzElemente.resize(index + 1);
    } else if (parseResult->children_ReferenzElemente[index]) {
      std::cerr << "Ignoriere doppelten Eintrag fuer ReferenzElemente: " << index << "\n";
    }
    parseResult->children_ReferenzElemente[index] = std::move(childResult);
  }
            }
            else if (name_size == 10 && !memcmp(name, "StrElement", 10)) {
                  std::unique_ptr<StrElement> childResult(new StrElement());
  parse_element_StrElement(text, childResult.get());
  size_t index = childResult->Nr;
  if (index == parseResult->children_StrElement.size()) {
    parseResult->children_StrElement.push_back(std::move(childResult));
  } else {
    if (index > parseResult->children_StrElement.size()) {
      parseResult->children_StrElement.resize(index + 1);
    } else if (parseResult->children_StrElement[index]) {
      std::cerr << "Ignoriere doppelten Eintrag fuer StrElement: " << index << "\n";
    }
    parseResult->children_StrElement[index] = std::move(childResult);
  }
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_ReferenzElement(const Ch *& text, struct ReferenzElement* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 10 && !memcmp(name, "ReferenzNr", 10)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->ReferenzNr);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 10 && !memcmp(name, "StrElement", 10)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->StrElement);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 7 && !memcmp(name, "StrNorm", 7)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->StrNorm = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 6 && !memcmp(name, "RefTyp", 6)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->RefTyp);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 4 && !memcmp(name, "Info", 4)) {
          parse_string(text, parseResult->Info, quote);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct ReferenzElement* parseResult = static_cast<struct ReferenzElement*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_StrElement(const Ch *& text, struct StrElement* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 2 && !memcmp(name, "Nr", 2)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Nr);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 6 && !memcmp(name, "Ueberh", 6)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->Ueberh);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 2 && !memcmp(name, "kr", 2)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->kr);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "Anschluss", 9)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Anschluss);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 3 && !memcmp(name, "Fkt", 3)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Fkt);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 7 && !memcmp(name, "Oberbau", 7)) {
          parse_string(text, parseResult->Oberbau, quote);
        }
        else if (name_size == 4 && !memcmp(name, "Volt", 4)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Volt);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 10 && !memcmp(name, "Drahthoehe", 10)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->Drahthoehe);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct StrElement* parseResult = static_cast<struct StrElement*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 1 && !memcmp(name, "g", 1)) {
                  parse_element_Vec3(text, &parseResult->g);
            }
            else if (name_size == 1 && !memcmp(name, "b", 1)) {
                  parse_element_Vec3(text, &parseResult->b);
            }
            else if (name_size == 16 && !memcmp(name, "InfoNormRichtung", 16)) {
                  parseResult->InfoNormRichtung.emplace();
  parse_element_StreckenelementRichtungsInfo(text, &*parseResult->InfoNormRichtung);
            }
            else if (name_size == 17 && !memcmp(name, "InfoGegenRichtung", 17)) {
                  parseResult->InfoGegenRichtung.emplace();
  parse_element_StreckenelementRichtungsInfo(text, &*parseResult->InfoGegenRichtung);
            }
            else if (name_size == 8 && !memcmp(name, "NachNorm", 8)) {
                #if BOOST_VERSION < 106200
  parseResult->children_NachNorm.emplace_back();
  parse_element_NachfolgerSelbesModul(text, &parseResult->children_NachNorm.back());
#else
  parse_element_NachfolgerSelbesModul(text, &parseResult->children_NachNorm.emplace_back());
#endif
            }
            else if (name_size == 9 && !memcmp(name, "NachGegen", 9)) {
                #if BOOST_VERSION < 106200
  parseResult->children_NachGegen.emplace_back();
  parse_element_NachfolgerSelbesModul(text, &parseResult->children_NachGegen.back());
#else
  parse_element_NachfolgerSelbesModul(text, &parseResult->children_NachGegen.emplace_back());
#endif
            }
            else if (name_size == 13 && !memcmp(name, "NachNormModul", 13)) {
                #if BOOST_VERSION < 106200
  parseResult->children_NachNormModul.emplace_back();
  parse_element_NachfolgerAnderesModul(text, &parseResult->children_NachNormModul.back());
#else
  parse_element_NachfolgerAnderesModul(text, &parseResult->children_NachNormModul.emplace_back());
#endif
            }
            else if (name_size == 14 && !memcmp(name, "NachGegenModul", 14)) {
                #if BOOST_VERSION < 106200
  parseResult->children_NachGegenModul.emplace_back();
  parse_element_NachfolgerAnderesModul(text, &parseResult->children_NachGegenModul.back());
#else
  parse_element_NachfolgerAnderesModul(text, &parseResult->children_NachGegenModul.emplace_back());
#endif
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
  static void parse_element_NachfolgerAnderesModul(const Ch *& text, struct NachfolgerAnderesModul* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          const size_t name_size [[maybe_unused]] = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              parse_error_expected_equals(text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              parse_error_expected_quote(text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 2 && !memcmp(name, "Nr", 2)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Nr);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              parse_error_expected_quote(text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              struct NachfolgerAnderesModul* parseResult = static_cast<struct NachfolgerAnderesModul*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  parse_error_expected_element_name(text);
              const size_t name_size [[maybe_unused]] = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          parse_error_expected_tag_end(text);
    }
}  // namespace zusixml
