
#include <sstream>
#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <cctype>


#define ctpg exaudf_ctpg
#include "ctpg/ctpg.hpp"
#undef ctpg

using namespace exaudf_ctpg;
using namespace exaudf_ctpg::ftors;


struct Option {
    std::string key;
    std::string value;
    source_point start;
    source_point end;
};

using options_type = std::vector<Option>;

auto empty_options()
{
    options_type ob;
    return ob;
}

auto to_options(Option&& e)
{
    options_type ob{e};
    return ob;
}

auto to_option(std::string_view key, std::string value, source_point sp_begin, source_point sp_end)
{
    std::cerr << "Found option:" << key << " - '" << value << "'" << std::endl;
    return Option{std::string(key), value, sp_begin, sp_end};
}

auto&& add_option(Option&& e, options_type&& ob)
{
    ob.push_back(std::move(e));
    return std::move(ob);
}



constexpr char alpha_numeric_pattern[] = R"_([0-9a-zA-Z_]+)_";
constexpr char option_char_pattern[] = R"_([^;])_";
constexpr char whitespaces_pattern[] = R"_([ \x09]+)_";


constexpr char_term start_option_tag('%');
constexpr char_term end_option_tag(';');
constexpr regex_term<alpha_numeric_pattern> alpha_numeric("alpha_numeric");
constexpr regex_term<option_char_pattern> option_char("option_char");
constexpr regex_term<whitespaces_pattern> whitespaces("whitespace");
constexpr string_term semicolon_escape(R"_(\;)_");

constexpr nterm<options_type> text("text");
constexpr nterm<options_type> options("options");
constexpr nterm<Option> option_element("option_element");
constexpr nterm<int> rest("rest");
constexpr nterm<std::string> option_value("option_value");

constexpr nterm<std::string> option_key("option_key");


constexpr parser option_parser(
    text,
    terms(start_option_tag, semicolon_escape, whitespaces, end_option_tag, alpha_numeric, option_char),
    nterms(text, option_key, option_value, options, option_element, rest),
    rules(
        text(rest)
            >= [] (skip) {return empty_options();},
        text(options, rest)
            >= [] (auto o, skip) {return o;},
        text(options)
            >= [] (auto o) {return o;},
        options(option_element)
            >= [](auto&& e) { return to_options(std::move(e)); },
        options(options, option_element)
            >= [](auto&& ob, auto&& e) { return add_option(std::move(e), std::move(ob)); },
        option_element(start_option_tag, alpha_numeric, whitespaces, option_value, end_option_tag)
            >= [](auto st, auto ok, skip, auto ov, auto e) { return to_option(ok.get_value(), ov, st.get_sp(), e.get_sp()); },
        option_element(whitespaces, start_option_tag, alpha_numeric, whitespaces, option_value, end_option_tag)
            >= [](skip, auto st, auto ok, skip, auto ov, auto e) { return to_option(ok.get_value(), ov, st.get_sp(), e.get_sp()); },
        option_value(alpha_numeric)
            >= [](auto o) { return std::string(o.get_value()); },
        option_value(option_char)
            >= [](auto o) { return std::string(o.get_value()); },
        option_value(whitespaces)
            >= [](auto o) { return std::string(o.get_value()); },
        option_value(semicolon_escape)
            >= [](auto o) { return std::string(";"); },
        option_value(option_value, option_char)
            >= [](auto&& ov, auto v) { return std::move(ov.append(v.get_value())); },
        option_value(option_value, semicolon_escape)
            >= [](auto&& ov, auto v) { return std::move(ov.append(";")); },
        option_value(option_value, start_option_tag)
            >= [](auto&& ov, auto v) { return std::move(ov.append("%")); },
        option_value(option_value, alpha_numeric)
            >= [](auto&& ov, auto v) { return std::move(ov.append(v.get_value())); },
        option_value(option_value, whitespaces)
            >= [](auto&& ov, auto v) { return std::move(ov.append(v.get_value())); },
        rest(alpha_numeric)
            >= [](auto r) { return 0;},
        rest(whitespaces)
            >= [](auto r) { return 0;},
        rest(semicolon_escape)
            >= [](auto r) { return 0;},
        rest(end_option_tag)
            >= [](auto r) { return 0;},
        rest(option_char)
            >= [](auto r) { return 0;},
        rest(rest, alpha_numeric)
            >= [](auto r, skip) { return 0;},
        rest(rest, option_char)
            >= [](auto r, skip) { return 0;},
        rest(rest, whitespaces)
            >= [](auto r, skip) { return 0;},
        rest(rest, end_option_tag)
            >= [](auto r, skip) { return 0;},
        rest(rest, start_option_tag)
            >= [](auto r, skip) { return 0;}
    )
);

int main(int argc, char* argv[])
{

//        std::cout << escaped_sequence_pattern << std::endl << std::endl;
//
//      if (std::regex_match ("\"Option0\" \"some_value\";", std::regex(option_pattern) )) {
//        std::cout << "string:literal => matched\n";
//      }
//      else {
//        std::cout << "Mot matched\n";
//        return 0;
//      }

    if (argc > 1 && strcmp(argv[1], "--print-parser") == 0) {
        option_parser.write_diag_str(std::cerr);
        return 0;
    }


    std::vector<std::string> lines;
    {
        std::string line;
        while (std::getline(std::cin, line))
        {
            if (!line.empty() && !std::all_of(line.begin(),line.end(), [](const char c) {return std::isspace(c);})) {
                lines.push_back(line);
            }
        }
    }
    for ( const std::string & line : lines) {
        std::cerr << "Parsing line: '" << line << "'" << std::endl;
        auto res = option_parser.parse(
            parse_options{}.set_verbose(true).set_skip_whitespace(false),
            buffers::string_buffer(line.c_str()),
            std::cout);
        if (res.has_value())
        {
    //        std::cout << res.value() << std::endl;

            for (const auto& w : res.value())
            {
                std::cout <<  w.key << " (" << w.value << ") pos: "<< w.start << "-" << w.end << std::endl;
            }
        }
    }

    return 0;
}
