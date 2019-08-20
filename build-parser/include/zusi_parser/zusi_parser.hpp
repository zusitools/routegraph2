#pragma once
#include "zusixml.hpp"
#include "zusi_parser/zusi_types.hpp"
#include <array>
#include <cstring>  // for memcmp
#include <cfloat>   // Workaround for https://svn.boost.org/trac10/ticket/12642
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

  static void parse_element_StrModul(const Ch *& text, StrModul* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node StrModul: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              StrModul* parseResult = static_cast<StrModul*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else if (name_size == 1 && !memcmp(name, "p", 1)) {
                  parse_element_Vec3(text, &parseResult->p);
            }
            else if (name_size == 3 && !memcmp(name, "phi", 3)) {
                  parse_element_Vec3(text, &parseResult->phi);
            }
            else {
              std::cerr << "Unexpected child of node StrModul: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Ersatzsignal(const Ch *& text, Ersatzsignal* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 20 && !memcmp(name, "ErsatzsigBezeichnung", 20)) {
          parse_string(text, parseResult->ErsatzsigBezeichnung, quote);
        }
        else if (name_size == 11 && !memcmp(name, "ErsatzsigID", 11)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_parser<uint32_t, 16, 1, 9>(), parseResult->ErsatzsigID);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 16 && !memcmp(name, "SigAssiErgaenzen", 16)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->SigAssiErgaenzen = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Ersatzsignal: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Ersatzsignal* parseResult = static_cast<Ersatzsignal*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 13 && !memcmp(name, "MatrixEintrag", 13)) {
                  parse_element_MatrixEintrag(text, &parseResult->MatrixEintrag);
            }
            else {
              std::cerr << "Unexpected child of node Ersatzsignal: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Beschreibung(const Ch *& text, Beschreibung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 12 && !memcmp(name, "Beschreibung", 12)) {
          parse_string(text, parseResult->Beschreibung, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node Beschreibung: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Beschreibung* parseResult = static_cast<Beschreibung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node Beschreibung: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_LoeschFahrstrasse(const Ch *& text, LoeschFahrstrasse* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 11 && !memcmp(name, "FahrstrName", 11)) {
          parse_string(text, parseResult->FahrstrName, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node LoeschFahrstrasse: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              LoeschFahrstrasse* parseResult = static_cast<LoeschFahrstrasse*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node LoeschFahrstrasse: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrSigHaltfall(const Ch *& text, FahrstrSigHaltfall* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrSigHaltfall: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrSigHaltfall* parseResult = static_cast<FahrstrSigHaltfall*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrSigHaltfall: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Strecke(const Ch *& text, Strecke* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 8 && !memcmp(name, "RekTiefe", 8)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->RekTiefe);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 13 && !memcmp(name, "Himmelsmodell", 13)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Himmelsmodell);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Strecke: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Strecke* parseResult = static_cast<Strecke*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else if (name_size == 13 && !memcmp(name, "PanoramaDatei", 13)) {
                // deprecated
                skip_element(text);
            }
            else if (name_size == 17 && !memcmp(name, "LoeschFahrstrasse", 17)) {
                  parse_element_LoeschFahrstrasse(text, parseResult->children_LoeschFahrstrasse.emplace_back(new struct LoeschFahrstrasse()).get());
            }
            else if (name_size == 16 && !memcmp(name, "HintergrundDatei", 16)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->HintergrundDatei);
            }
            else if (name_size == 20 && !memcmp(name, "BefehlsKonfiguration", 20)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->BefehlsKonfiguration);
            }
            else if (name_size == 10 && !memcmp(name, "Kachelpfad", 10)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Kachelpfad);
            }
            else if (name_size == 12 && !memcmp(name, "Beschreibung", 12)) {
                  parse_element_Beschreibung(text, &parseResult->Beschreibung);
            }
            else if (name_size == 3 && !memcmp(name, "UTM", 3)) {
                  std::unique_ptr<struct UTM, zusixml::deleter<struct UTM>> childResult(new struct UTM());
  parseResult->UTM.swap(childResult);
  parse_element_UTM(text, parseResult->UTM.get());
            }
            else if (name_size == 10 && !memcmp(name, "Huellkurve", 10)) {
                  parse_element_Huellkurve(text, parseResult->children_Huellkurve.emplace_back(new struct Huellkurve()).get());
            }
            else if (name_size == 6 && !memcmp(name, "Skybox", 6)) {
                  std::unique_ptr<struct Skybox, zusixml::deleter<struct Skybox>> childResult(new struct Skybox());
  parseResult->Skybox.swap(childResult);
  parse_element_Skybox(text, parseResult->Skybox.get());
            }
            else if (name_size == 7 && !memcmp(name, "SkyDome", 7)) {
                  std::unique_ptr<struct SkyDome, zusixml::deleter<struct SkyDome>> childResult(new struct SkyDome());
  parseResult->SkyDome.swap(childResult);
  parse_element_SkyDome(text, parseResult->SkyDome.get());
            }
            else if (name_size == 16 && !memcmp(name, "StreckenStandort", 16)) {
                  parse_element_StreckenStandort(text, parseResult->children_StreckenStandort.emplace_back(new struct StreckenStandort()).get());
            }
            else if (name_size == 12 && !memcmp(name, "ModulDateien", 12)) {
                  parse_element_ModulDateiVerknuepfung(text, parseResult->children_ModulDateien.emplace_back(new struct ModulDateiVerknuepfung()).get());
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
            else if (name_size == 11 && !memcmp(name, "Fahrstrasse", 11)) {
                  parse_element_Fahrstrasse(text, parseResult->children_Fahrstrasse.emplace_back(new struct Fahrstrasse()).get());
            }
            else {
              std::cerr << "Unexpected child of node Strecke: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_VsigBegriff(const Ch *& text, VsigBegriff* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 10 && !memcmp(name, "VsigGeschw", 10)) {
          parse_float(text, parseResult->VsigGeschw);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node VsigBegriff: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              VsigBegriff* parseResult = static_cast<VsigBegriff*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node VsigBegriff: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Stufe(const Ch *& text, Stufe* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 10 && !memcmp(name, "StufenWert", 10)) {
          parse_float(text, parseResult->StufenWert);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Stufe: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Stufe* parseResult = static_cast<Stufe*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node Stufe: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Pkt(const Ch *& text, Pkt* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 4 && !memcmp(name, "PktX", 4)) {
          parse_float(text, parseResult->PktX);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 4 && !memcmp(name, "PktY", 4)) {
          parse_float(text, parseResult->PktY);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Pkt: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Pkt* parseResult = static_cast<Pkt*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node Pkt: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Sound(const Ch *& text, Sound* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 5 && !memcmp(name, "dreiD", 5)) {
          parseResult->dreiD = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 4 && !memcmp(name, "Loop", 4)) {
          parseResult->Loop = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "Autostart", 9)) {
          parseResult->Autostart = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "PosAnlauf", 9)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->PosAnlauf);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 10 && !memcmp(name, "PosAuslauf", 10)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->PosAuslauf);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 11 && !memcmp(name, "Lautstaerke", 11)) {
          parse_float(text, parseResult->Lautstaerke);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 13 && !memcmp(name, "GeschwAendern", 13)) {
          parseResult->GeschwAendern = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "MinRadius", 9)) {
          parse_float(text, parseResult->MinRadius);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "MaxRadius", 9)) {
          parse_float(text, parseResult->MaxRadius);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Sound: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Sound* parseResult = static_cast<Sound*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node Sound: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_SignalFrame(const Ch *& text, SignalFrame* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 21 && !memcmp(name, "WeichenbaugruppeIndex", 21)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->WeichenbaugruppeIndex);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 18 && !memcmp(name, "WeichenbaugruppeNr", 18)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->WeichenbaugruppeNr);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 28 && !memcmp(name, "WeichenbaugruppeBeschreibung", 28)) {
          parse_string(text, parseResult->WeichenbaugruppeBeschreibung, quote);
        }
        else if (name_size == 20 && !memcmp(name, "WeichenbaugruppePos0", 20)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->WeichenbaugruppePos0 = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 20 && !memcmp(name, "WeichenbaugruppePos1", 20)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->WeichenbaugruppePos1 = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node SignalFrame: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              SignalFrame* parseResult = static_cast<SignalFrame*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 1 && !memcmp(name, "p", 1)) {
                  parse_element_Vec3(text, &parseResult->p);
            }
            else if (name_size == 3 && !memcmp(name, "phi", 3)) {
                  parse_element_Vec3(text, &parseResult->phi);
            }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else if (name_size == 8 && !memcmp(name, "Ereignis", 8)) {
                  parse_element_Ereignis(text, parseResult->children_Ereignis.emplace_back(new struct Ereignis()).get());
            }
            else {
              std::cerr << "Unexpected child of node SignalFrame: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_NachfolgerSelbesModul(const Ch *& text, NachfolgerSelbesModul* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 2 && !memcmp(name, "Nr", 2)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Nr);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node NachfolgerSelbesModul: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              NachfolgerSelbesModul* parseResult = static_cast<NachfolgerSelbesModul*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node NachfolgerSelbesModul: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Zusi(const Ch *& text, Zusi* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node Zusi: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Zusi* parseResult = static_cast<Zusi*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 4 && !memcmp(name, "Info", 4)) {
                  std::unique_ptr<struct Info, zusixml::deleter<struct Info>> childResult(new struct Info());
  parseResult->Info.swap(childResult);
  parse_element_Info(text, parseResult->Info.get());
            }
            else if (name_size == 7 && !memcmp(name, "Strecke", 7)) {
                  std::unique_ptr<struct Strecke, zusixml::deleter<struct Strecke>> childResult(new struct Strecke());
  parseResult->Strecke.swap(childResult);
  parse_element_Strecke(text, parseResult->Strecke.get());
            }
            else if (name_size == 8 && !memcmp(name, "Fahrplan", 8)) {
                  std::unique_ptr<struct Fahrplan, zusixml::deleter<struct Fahrplan>> childResult(new struct Fahrplan());
  parseResult->Fahrplan.swap(childResult);
  parse_element_Fahrplan(text, parseResult->Fahrplan.get());
            }
            else {
              std::cerr << "Unexpected child of node Zusi: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Kennfeld(const Ch *& text, Kennfeld* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 5 && !memcmp(name, "xText", 5)) {
          parse_string(text, parseResult->xText, quote);
        }
        else if (name_size == 5 && !memcmp(name, "yText", 5)) {
          parse_string(text, parseResult->yText, quote);
        }
        else if (name_size == 12 && !memcmp(name, "Beschreibung", 12)) {
          parse_string(text, parseResult->Beschreibung, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node Kennfeld: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Kennfeld* parseResult = static_cast<Kennfeld*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 3 && !memcmp(name, "Pkt", 3)) {
                  parse_element_Pkt(text, parseResult->children_Pkt.emplace_back(new struct Pkt()).get());
            }
            else if (name_size == 5 && !memcmp(name, "Stufe", 5)) {
                  parse_element_Stufe(text, parseResult->children_Stufe.emplace_back(new struct Stufe()).get());
            }
            else {
              std::cerr << "Unexpected child of node Kennfeld: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Skybox(const Ch *& text, Skybox* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node Skybox: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Skybox* parseResult = static_cast<Skybox*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 7 && !memcmp(name, "Skybox0", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Skybox0);
            }
            else if (name_size == 7 && !memcmp(name, "Skybox1", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Skybox1);
            }
            else if (name_size == 7 && !memcmp(name, "Skybox2", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Skybox2);
            }
            else if (name_size == 7 && !memcmp(name, "Skybox3", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Skybox3);
            }
            else if (name_size == 7 && !memcmp(name, "Skybox4", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Skybox4);
            }
            else if (name_size == 7 && !memcmp(name, "Skybox5", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Skybox5);
            }
            else {
              std::cerr << "Unexpected child of node Skybox: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrplanZugVerknuepfung(const Ch *& text, FahrplanZugVerknuepfung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node FahrplanZugVerknuepfung: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrplanZugVerknuepfung* parseResult = static_cast<FahrplanZugVerknuepfung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrplanZugVerknuepfung: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_AutorEintrag(const Ch *& text, AutorEintrag* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 7 && !memcmp(name, "AutorID", 7)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->AutorID);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "AutorName", 9)) {
          parse_string(text, parseResult->AutorName, quote);
        }
        else if (name_size == 10 && !memcmp(name, "AutorEmail", 10)) {
          parse_string(text, parseResult->AutorEmail, quote);
        }
        else if (name_size == 12 && !memcmp(name, "AutorAufwand", 12)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->AutorAufwand);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 11 && !memcmp(name, "AutorLizenz", 11)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->AutorLizenz);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 17 && !memcmp(name, "AutorBeschreibung", 17)) {
          parse_string(text, parseResult->AutorBeschreibung, quote);
        }
        else if (name_size == 9 && !memcmp(name, "Bemerkung", 9)) {
          // deprecated
          skip_attribute_value(text, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node AutorEintrag: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              AutorEintrag* parseResult = static_cast<AutorEintrag*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node AutorEintrag: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_StreckenStandort(const Ch *& text, StreckenStandort* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 7 && !memcmp(name, "StrInfo", 7)) {
          parse_string(text, parseResult->StrInfo, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node StreckenStandort: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              StreckenStandort* parseResult = static_cast<StreckenStandort*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 1 && !memcmp(name, "p", 1)) {
                  parse_element_Vec3(text, &parseResult->p);
            }
            else if (name_size == 6 && !memcmp(name, "lookat", 6)) {
                  parse_element_Vec3(text, &parseResult->lookat);
            }
            else if (name_size == 2 && !memcmp(name, "up", 2)) {
                  parse_element_Vec3(text, &parseResult->up);
            }
            else {
              std::cerr << "Unexpected child of node StreckenStandort: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Dateiverknuepfung(const Ch *& text, Dateiverknuepfung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 9 && !memcmp(name, "Dateiname", 9)) {
          parse_string(text, parseResult->Dateiname, quote);
        }
        else if (name_size == 4 && !memcmp(name, "inst", 4)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->inst = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 7 && !memcmp(name, "NurInfo", 7)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->NurInfo = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Dateiverknuepfung: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Dateiverknuepfung* parseResult = static_cast<Dateiverknuepfung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node Dateiverknuepfung: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Info(const Ch *& text, Info* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 8 && !memcmp(name, "DateiTyp", 8)) {
          parse_string(text, parseResult->DateiTyp, quote);
        }
        else if (name_size == 7 && !memcmp(name, "Version", 7)) {
          parse_string(text, parseResult->Version, quote);
        }
        else if (name_size == 10 && !memcmp(name, "MinVersion", 10)) {
          parse_string(text, parseResult->MinVersion, quote);
        }
        else if (name_size == 8 && !memcmp(name, "ObjektID", 8)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->ObjektID);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 12 && !memcmp(name, "Beschreibung", 12)) {
          parse_string(text, parseResult->Beschreibung, quote);
        }
        else if (name_size == 9 && !memcmp(name, "EinsatzAb", 9)) {
          skip_unlikely<whitespace_pred>(text);
          [[maybe_unused]] bool result = (unlikely(quote == Ch('\''))) ?
            parse_datetime<Ch('\'')>(text, parseResult->EinsatzAb) :
            parse_datetime<Ch('"')>(text, parseResult->EinsatzAb);
        }
        else if (name_size == 10 && !memcmp(name, "EinsatzBis", 10)) {
          skip_unlikely<whitespace_pred>(text);
          [[maybe_unused]] bool result = (unlikely(quote == Ch('\''))) ?
            parse_datetime<Ch('\'')>(text, parseResult->EinsatzBis) :
            parse_datetime<Ch('"')>(text, parseResult->EinsatzBis);
        }
        else if (name_size == 14 && !memcmp(name, "DateiKategorie", 14)) {
          parse_string(text, parseResult->DateiKategorie, quote);
        }
        else if (name_size == 5 && !memcmp(name, "Autor", 5)) {
          // deprecated
          skip_attribute_value(text, quote);
        }
        else if (name_size == 6 && !memcmp(name, "Lizenz", 6)) {
          // deprecated
          skip_attribute_value(text, quote);
        }
        else if (name_size == 6 && !memcmp(name, "DecSep", 6)) {
          // deprecated
          skip_attribute_value(text, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node Info: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Info* parseResult = static_cast<Info*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 12 && !memcmp(name, "AutorEintrag", 12)) {
                  parse_element_AutorEintrag(text, parseResult->children_AutorEintrag.emplace_back(new struct AutorEintrag()).get());
            }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, parseResult->children_Datei.emplace_back(new struct Dateiverknuepfung()).get());
            }
            else {
              std::cerr << "Unexpected child of node Info: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_StrElement(const Ch *& text, StrElement* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
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
        else if (name_size == 7 && !memcmp(name, "spTrass", 7)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->spTrass);
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
        else if (name_size == 16 && !memcmp(name, "Zwangshelligkeit", 16)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->Zwangshelligkeit);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 14 && !memcmp(name, "Streckennummer", 14)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Streckennummer);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node StrElement: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              StrElement* parseResult = static_cast<StrElement*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

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
              std::cerr << "Unexpected child of node StrElement: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrVSignal(const Ch *& text, FahrstrVSignal* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 19 && !memcmp(name, "FahrstrSignalSpalte", 19)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->FahrstrSignalSpalte);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrVSignal: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrVSignal* parseResult = static_cast<FahrstrVSignal*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrVSignal: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Vec3(const Ch *& text, Vec3* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
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
          std::cerr << "Unexpected attribute of node Vec3: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Vec3* parseResult = static_cast<Vec3*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node Vec3: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_UTM(const Ch *& text, UTM* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
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
        else if (name_size == 9 && !memcmp(name, "UTM_Zone2", 9)) {
          parse_string(text, parseResult->UTM_Zone2, quote);
        }
        else {
          std::cerr << "Unexpected attribute of node UTM: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              UTM* parseResult = static_cast<UTM*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node UTM: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_KoppelSignal(const Ch *& text, KoppelSignal* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 10 && !memcmp(name, "ReferenzNr", 10)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->ReferenzNr);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node KoppelSignal: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              KoppelSignal* parseResult = static_cast<KoppelSignal*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node KoppelSignal: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Abhaengigkeit(const Ch *& text, Abhaengigkeit* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 13 && !memcmp(name, "PhysikGroesse", 13)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->PhysikGroesse);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 14 && !memcmp(name, "LautstaerkeAbh", 14)) {
          parseResult->LautstaerkeAbh = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 14 && !memcmp(name, "SoundGeschwAbh", 14)) {
          parseResult->SoundGeschwAbh = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 13 && !memcmp(name, "SoundOperator", 13)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->SoundOperator);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 7 && !memcmp(name, "Trigger", 7)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Trigger);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 13 && !memcmp(name, "TriggerGrenze", 13)) {
          parse_float(text, parseResult->TriggerGrenze);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Abhaengigkeit: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Abhaengigkeit* parseResult = static_cast<Abhaengigkeit*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 8 && !memcmp(name, "Kennfeld", 8)) {
                  std::unique_ptr<struct Kennfeld, zusixml::deleter<struct Kennfeld>> childResult(new struct Kennfeld());
  parseResult->Kennfeld.swap(childResult);
  parse_element_Kennfeld(text, parseResult->Kennfeld.get());
            }
            else {
              std::cerr << "Unexpected child of node Abhaengigkeit: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Huellkurve(const Ch *& text, Huellkurve* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node Huellkurve: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Huellkurve* parseResult = static_cast<Huellkurve*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 8 && !memcmp(name, "PunktXYZ", 8)) {
                  parse_element_Vec3(text, parseResult->children_PunktXYZ.emplace_back(new struct Vec3()).get());
            }
            else {
              std::cerr << "Unexpected child of node Huellkurve: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Ereignis(const Ch *& text, Ereignis* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 2 && !memcmp(name, "Er", 2)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Er);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 6 && !memcmp(name, "Beschr", 6)) {
          parse_string(text, parseResult->Beschr, quote);
        }
        else if (name_size == 4 && !memcmp(name, "Wert", 4)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->Wert);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Ereignis: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Ereignis* parseResult = static_cast<Ereignis*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node Ereignis: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Fahrplan(const Ch *& text, Fahrplan* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 11 && !memcmp(name, "AnfangsZeit", 11)) {
          [[maybe_unused]] bool result = (unlikely(quote == Ch('\''))) ?
            parse_datetime<Ch('\'')>(text, parseResult->AnfangsZeit) :
            parse_datetime<Ch('"')>(text, parseResult->AnfangsZeit);
        }
        else if (name_size == 9 && !memcmp(name, "Zeitmodus", 9)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Zeitmodus);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Fahrplan: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Fahrplan* parseResult = static_cast<Fahrplan*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 20 && !memcmp(name, "BefehlsKonfiguration", 20)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->BefehlsKonfiguration);
            }
            else if (name_size == 17 && !memcmp(name, "Begruessungsdatei", 17)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Begruessungsdatei);
            }
            else if (name_size == 3 && !memcmp(name, "Zug", 3)) {
                  parse_element_FahrplanZugVerknuepfung(text, parseResult->children_Zug.emplace_back(new struct FahrplanZugVerknuepfung()).get());
            }
            else if (name_size == 8 && !memcmp(name, "StrModul", 8)) {
                  parse_element_StrModul(text, parseResult->children_StrModul.emplace_back(new struct StrModul()).get());
            }
            else if (name_size == 3 && !memcmp(name, "UTM", 3)) {
                  std::unique_ptr<struct UTM, zusixml::deleter<struct UTM>> childResult(new struct UTM());
  parseResult->UTM.swap(childResult);
  parse_element_UTM(text, parseResult->UTM.get());
            }
            else {
              std::cerr << "Unexpected child of node Fahrplan: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_SkyDome(const Ch *& text, SkyDome* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node SkyDome: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              SkyDome* parseResult = static_cast<SkyDome*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 9 && !memcmp(name, "HimmelTex", 9)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->HimmelTex);
            }
            else if (name_size == 8 && !memcmp(name, "SonneTex", 8)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->SonneTex);
            }
            else if (name_size == 16 && !memcmp(name, "SonneHorizontTex", 16)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->SonneHorizontTex);
            }
            else if (name_size == 7 && !memcmp(name, "MondTex", 7)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->MondTex);
            }
            else if (name_size == 8 && !memcmp(name, "SternTex", 8)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->SternTex);
            }
            else if (name_size == 8 && !memcmp(name, "NebelTex", 8)) {
                // deprecated
                skip_element(text);
            }
            else {
              std::cerr << "Unexpected child of node SkyDome: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrAufloesung(const Ch *& text, FahrstrAufloesung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrAufloesung: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrAufloesung* parseResult = static_cast<FahrstrAufloesung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrAufloesung: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Fahrstrasse(const Ch *& text, Fahrstrasse* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  if (false) { (void)parseResult; }
        else if (name_size == 11 && !memcmp(name, "FahrstrName", 11)) {
          parse_string(text, parseResult->FahrstrName, quote);
        }
        else if (name_size == 14 && !memcmp(name, "FahrstrStrecke", 14)) {
          parse_string(text, parseResult->FahrstrStrecke, quote);
        }
        else if (name_size == 6 && !memcmp(name, "RglGgl", 6)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->RglGgl);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 10 && !memcmp(name, "FahrstrTyp", 10)) {
          parse_string(text, parseResult->FahrstrTyp, quote);
        }
        else if (name_size == 11 && !memcmp(name, "ZufallsWert", 11)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->ZufallsWert);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 6 && !memcmp(name, "Laenge", 6)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->Laenge);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Fahrstrasse: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Fahrstrasse* parseResult = static_cast<Fahrstrasse*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 12 && !memcmp(name, "FahrstrStart", 12)) {
                  std::unique_ptr<struct FahrstrStart, zusixml::deleter<struct FahrstrStart>> childResult(new struct FahrstrStart());
  parseResult->FahrstrStart.swap(childResult);
  parse_element_FahrstrStart(text, parseResult->FahrstrStart.get());
            }
            else if (name_size == 11 && !memcmp(name, "FahrstrZiel", 11)) {
                  std::unique_ptr<struct FahrstrZiel, zusixml::deleter<struct FahrstrZiel>> childResult(new struct FahrstrZiel());
  parseResult->FahrstrZiel.swap(childResult);
  parse_element_FahrstrZiel(text, parseResult->FahrstrZiel.get());
            }
            else if (name_size == 15 && !memcmp(name, "FahrstrRegister", 15)) {
                  parse_element_FahrstrRegister(text, parseResult->children_FahrstrRegister.emplace_back(new struct FahrstrRegister()).get());
            }
            else if (name_size == 17 && !memcmp(name, "FahrstrAufloesung", 17)) {
                  parse_element_FahrstrAufloesung(text, parseResult->children_FahrstrAufloesung.emplace_back(new struct FahrstrAufloesung()).get());
            }
            else if (name_size == 21 && !memcmp(name, "FahrstrTeilaufloesung", 21)) {
                  parse_element_FahrstrTeilaufloesung(text, parseResult->children_FahrstrTeilaufloesung.emplace_back(new struct FahrstrTeilaufloesung()).get());
            }
            else if (name_size == 18 && !memcmp(name, "FahrstrSigHaltfall", 18)) {
                  parse_element_FahrstrSigHaltfall(text, parseResult->children_FahrstrSigHaltfall.emplace_back(new struct FahrstrSigHaltfall()).get());
            }
            else if (name_size == 13 && !memcmp(name, "FahrstrWeiche", 13)) {
                  parse_element_FahrstrWeiche(text, parseResult->children_FahrstrWeiche.emplace_back(new struct FahrstrWeiche()).get());
            }
            else if (name_size == 13 && !memcmp(name, "FahrstrSignal", 13)) {
                  parse_element_FahrstrSignal(text, parseResult->children_FahrstrSignal.emplace_back(new struct FahrstrSignal()).get());
            }
            else if (name_size == 14 && !memcmp(name, "FahrstrVSignal", 14)) {
                  parse_element_FahrstrVSignal(text, parseResult->children_FahrstrVSignal.emplace_back(new struct FahrstrVSignal()).get());
            }
            else {
              std::cerr << "Unexpected child of node Fahrstrasse: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrTeilaufloesung(const Ch *& text, FahrstrTeilaufloesung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrTeilaufloesung: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrTeilaufloesung* parseResult = static_cast<FahrstrTeilaufloesung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrTeilaufloesung: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_ReferenzElement(const Ch *& text, ReferenzElement* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
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
          std::cerr << "Unexpected attribute of node ReferenzElement: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              ReferenzElement* parseResult = static_cast<ReferenzElement*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node ReferenzElement: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_StreckenelementRichtungsInfo(const Ch *& text, StreckenelementRichtungsInfo* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 4 && !memcmp(name, "vMax", 4)) {
          parse_float(text, parseResult->vMax);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 2 && !memcmp(name, "km", 2)) {
          parse_float(text, parseResult->km);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 3 && !memcmp(name, "pos", 3)) {
          parseResult->pos = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 3 && !memcmp(name, "Reg", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Reg);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 14 && !memcmp(name, "KoppelWeicheNr", 14)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->KoppelWeicheNr);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 16 && !memcmp(name, "KoppelWeicheNorm", 16)) {
          parseResult->KoppelWeicheNorm = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node StreckenelementRichtungsInfo: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              StreckenelementRichtungsInfo* parseResult = static_cast<StreckenelementRichtungsInfo*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 8 && !memcmp(name, "Ereignis", 8)) {
                  parse_element_Ereignis(text, parseResult->children_Ereignis.emplace_back(new struct Ereignis()).get());
            }
            else if (name_size == 6 && !memcmp(name, "Signal", 6)) {
                  std::unique_ptr<struct Signal, zusixml::deleter<struct Signal>> childResult(new struct Signal());
  parseResult->Signal.swap(childResult);
  parse_element_Signal(text, parseResult->Signal.get());
            }
            else {
              std::cerr << "Unexpected child of node StreckenelementRichtungsInfo: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_NachfolgerAnderesModul(const Ch *& text, NachfolgerAnderesModul* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 2 && !memcmp(name, "Nr", 2)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Nr);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node NachfolgerAnderesModul: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              NachfolgerAnderesModul* parseResult = static_cast<NachfolgerAnderesModul*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node NachfolgerAnderesModul: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrStart(const Ch *& text, FahrstrStart* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrStart: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrStart* parseResult = static_cast<FahrstrStart*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrStart: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrZiel(const Ch *& text, FahrstrZiel* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrZiel: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrZiel* parseResult = static_cast<FahrstrZiel*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrZiel: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_MatrixEintrag(const Ch *& text, MatrixEintrag* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 10 && !memcmp(name, "Signalbild", 10)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Signalbild);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 12 && !memcmp(name, "MatrixGeschw", 12)) {
          parse_float(text, parseResult->MatrixGeschw);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 8 && !memcmp(name, "SignalID", 8)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->SignalID);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node MatrixEintrag: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              MatrixEintrag* parseResult = static_cast<MatrixEintrag*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 8 && !memcmp(name, "Ereignis", 8)) {
                  parse_element_Ereignis(text, parseResult->children_Ereignis.emplace_back(new struct Ereignis()).get());
            }
            else {
              std::cerr << "Unexpected child of node MatrixEintrag: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrRegister(const Ch *& text, FahrstrRegister* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrRegister: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrRegister* parseResult = static_cast<FahrstrRegister*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrRegister: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrWeiche(const Ch *& text, FahrstrWeiche* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 18 && !memcmp(name, "FahrstrWeichenlage", 18)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->FahrstrWeichenlage);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrWeiche: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrWeiche* parseResult = static_cast<FahrstrWeiche*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrWeiche: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_ModulDateiVerknuepfung(const Ch *& text, ModulDateiVerknuepfung* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else {
          std::cerr << "Unexpected attribute of node ModulDateiVerknuepfung: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              ModulDateiVerknuepfung* parseResult = static_cast<ModulDateiVerknuepfung*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node ModulDateiVerknuepfung: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_FahrstrSignal(const Ch *& text, FahrstrSignal* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 3 && !memcmp(name, "Ref", 3)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->Ref);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 18 && !memcmp(name, "FahrstrSignalZeile", 18)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->FahrstrSignalZeile);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 25 && !memcmp(name, "FahrstrSignalErsatzsignal", 25)) {
          parseResult->FahrstrSignalErsatzsignal = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node FahrstrSignal: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              FahrstrSignal* parseResult = static_cast<FahrstrSignal*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 5 && !memcmp(name, "Datei", 5)) {
                  parse_element_Dateiverknuepfung(text, &parseResult->Datei);
            }
            else {
              std::cerr << "Unexpected child of node FahrstrSignal: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_Signal(const Ch *& text, Signal* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
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
        else if (name_size == 11 && !memcmp(name, "ZufallsWert", 11)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->ZufallsWert);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 11 && !memcmp(name, "SignalFlags", 11)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->SignalFlags);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "SignalTyp", 9)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->SignalTyp);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 13 && !memcmp(name, "Weichenbauart", 13)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->Weichenbauart);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 20 && !memcmp(name, "WeichenGrundstellung", 20)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->WeichenGrundstellung);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 22 && !memcmp(name, "WeicheStumpfIgnorieren", 22)) {
          skip_unlikely<whitespace_pred>(text);
          parseResult->WeicheStumpfIgnorieren = (text[0] == '1');
          ++text;
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 9 && !memcmp(name, "BoundingR", 9)) {
          skip_unlikely<whitespace_pred>(text);
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::int_, parseResult->BoundingR);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 16 && !memcmp(name, "Zwangshelligkeit", 16)) {
          skip_unlikely<whitespace_pred>(text);
          parse_float(text, parseResult->Zwangshelligkeit);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node Signal: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (*text == Ch('>'))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              Signal* parseResult = static_cast<Signal*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else if (name_size == 1 && !memcmp(name, "p", 1)) {
                  parse_element_Vec3(text, &parseResult->p);
            }
            else if (name_size == 3 && !memcmp(name, "phi", 3)) {
                  parse_element_Vec3(text, &parseResult->phi);
            }
            else if (name_size == 12 && !memcmp(name, "KoppelSignal", 12)) {
                  std::unique_ptr<struct KoppelSignal, zusixml::deleter<struct KoppelSignal>> childResult(new struct KoppelSignal());
  parseResult->KoppelSignal.swap(childResult);
  parse_element_KoppelSignal(text, parseResult->KoppelSignal.get());
            }
            else if (name_size == 11 && !memcmp(name, "SignalFrame", 11)) {
                  parse_element_SignalFrame(text, parseResult->children_SignalFrame.emplace_back(new struct SignalFrame()).get());
            }
            else if (name_size == 11 && !memcmp(name, "HsigBegriff", 11)) {
                  parse_element_HsigBegriff(text, parseResult->children_HsigBegriff.emplace_back(new struct HsigBegriff()).get());
            }
            else if (name_size == 11 && !memcmp(name, "VsigBegriff", 11)) {
                  parse_element_VsigBegriff(text, parseResult->children_VsigBegriff.emplace_back(new struct VsigBegriff()).get());
            }
            else if (name_size == 13 && !memcmp(name, "MatrixEintrag", 13)) {
                  parse_element_MatrixEintrag(text, parseResult->children_MatrixEintrag.emplace_back(new struct MatrixEintrag()).get());
            }
            else if (name_size == 12 && !memcmp(name, "Ersatzsignal", 12)) {
                  parse_element_Ersatzsignal(text, parseResult->children_Ersatzsignal.emplace_back(new struct Ersatzsignal()).get());
            }
            else {
              std::cerr << "Unexpected child of node Signal: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
  static void parse_element_HsigBegriff(const Ch *& text, HsigBegriff* parseResult) {

      // For all attributes
      while (attribute_name_pred::test(*text))
      {
          // Extract attribute name
          const Ch *name = text;
          ++text;     // Skip first character of attribute name
          skip<attribute_name_pred>(text);
          size_t name_size = text - name;

          // Skip whitespace after attribute name
          skip_unlikely<whitespace_pred>(text);

          // Skip =
          if (*text != Ch('='))
              RAPIDXML_PARSE_ERROR("expected =", text);
          ++text;

          // Skip whitespace after =
          skip_unlikely<whitespace_pred>(text);

          // Skip quote and remember if it was ' or "
          Ch quote = *text;
          if (quote != Ch('\'') && quote != Ch('"'))
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;

          // Extract attribute value
                  skip_unlikely<whitespace_pred>(text);
        if (false) { (void)parseResult; }
        else if (name_size == 10 && !memcmp(name, "HsigGeschw", 10)) {
          parse_float(text, parseResult->HsigGeschw);
          skip_unlikely<whitespace_pred>(text);
        }
        else if (name_size == 10 && !memcmp(name, "FahrstrTyp", 10)) {
          boost::spirit::qi::parse(text, static_cast<const char*>(nullptr), boost::spirit::qi::long_long, parseResult->FahrstrTyp);
          skip_unlikely<whitespace_pred>(text);
        }
        else {
          std::cerr << "Unexpected attribute of node HsigBegriff: '" << std::string_view(name, name_size) << "'\n";
          skip_attribute_value(text, quote);
        }


          // Make sure that end quote is present
          if (*text != quote)
              RAPIDXML_PARSE_ERROR("expected ' or \"", text);
          ++text;     // Skip quote

          // Skip whitespace after attribute value
          skip<whitespace_pred>(text);
      }

      // Determine ending type
      if (unlikely(*text == Ch('>')))
      {
          ++text;
          parse_node_contents(text, [](const Ch *&text, void* parseResultUntyped) {
              HsigBegriff* parseResult = static_cast<HsigBegriff*>(parseResultUntyped);
              // Extract element name
              const Ch *name = text;
              skip<node_name_pred>(text);
              if (text == name)
                  RAPIDXML_PARSE_ERROR("expected element name", text);
              size_t name_size = text - name;

              // Skip whitespace between element name and attributes or >
              skip<whitespace_pred>(text);

              if (false) { (void)parseResult; }
            else {
              std::cerr << "Unexpected child of node HsigBegriff: '" << std::string_view(name, name_size) << "'\n";
              skip_element(text);
            }

          }, parseResult);
      }
      else if ((text[0] == Ch('/')) && (text[1] == Ch('>')))
      {
          text += 2;
      }
      else
          RAPIDXML_PARSE_ERROR("expected > or />", text);
    }
}  // namespace zusixml
