#include <tst/set.hpp>
#include <tst/check.hpp>

#include "../../../src/treeml/tree.hpp"

namespace{
tst::set set1("parser_correctness", [](auto& suite){
	suite.template add<std::pair<const char*, const char*>>(
			"single_slash_is_parsed_as_node",
			// the test checks start 5th element is as expected
			{
				{"a b c d e f g", "f"},
				{"a b c d e / g", "/"},
				{"a b c d e /\rg", "/"},
				{"a b c d e /\tg", "/"},
				{"a b c d e /\ng", "/"},
				{"a b c d e / /g", "/"},
				{"a b c d e /f g", "/f"},
				{"a b c d e /{z} g", "/"},
				{"a b c d e //{z} g\nf g", "f"},
			},
			[](auto& p){
				auto r = treeml::read(p.first);

				tst::check_ge(r.size(), size_t(5), SL);

				tst::check_eq(r[5].value.to_string(), std::string(p.second), SL);
			}
		);
	
	suite.template add<std::string>(
			"comment_in_the_beginning_is_ignored",
			{
				"// bla bla" "\n"
				"/* bla bla bla" "\n"
				"bla bla */ first" "\n"
				"second",

				"\n"
				"// bla bla" "\n"
				"/* bla bla bla" "\n"
				"bla bla */ first" "\n"
				"second",

				"\n"
				" // bla bla" "\n"
				"/* bla bla bla" "\n"
				"bla bla */ first" "\n"
				"second",

				"/* bla bla bla" "\n"
				"bla bla */ first" "\n"
				"second",

				"\n"
				"/* bla bla bla" "\n"
				"bla bla */ first" "\n"
				"second",

				"\n"
				" /* bla bla bla" "\n"
				"bla bla */ first" "\n"
				"second",
			},
			[](auto& p){
				auto in = treeml::read(p);
				
				tst::check_eq(in.size(), size_t(2), SL);
				tst::check_eq(in[0].value.to_string(), std::string("first"), SL);
				tst::check_eq(in[1].value.to_string(), std::string("second"), SL);
			}
		);
	
	suite.add("only_python_style_raw_string_in_the_document", [](){
		auto r = treeml::read("\"\"\"hello\"\"\"");
		tst::check_eq(r.size(), size_t(1), SL);
		tst::check_eq(r.front().children.size(), size_t(0), SL);
		tst::check_eq(r.front().value.to_string(), std::string("hello"), SL);
	});

	suite.template add<std::pair<std::string, treeml::forest>>(
			"parsed_tree_is_as_expected",
			{
				// python-style raw strings
				{"\"\"\"hello\"\"\"", {{"hello"}}},
				{" \"\"\"hello\"\"\"", {{"hello"}}},
				{"\n\"\"\"hello\"\"\" ", {{"hello"}}},
				{"\t\"\"\"hello\"\"\" ", {{"hello"}}},
				{"pre\"\"\"hello\"\"\" ", {{"pre"}, {"hello"}}},
				{"pre{}\"\"\"hello\"\"\"{child} ", {{"pre"}, {"hello", {{"child"}}}}},
				{"pre{} \"\"\"hello\"\"\" {child} ", {{"pre"}, {"hello", {{"child"}}}}},
				{"pre{} \"\"\"hello\"\"\"\n{child} ", {{"pre"}, {"hello", {{"child"}}}}},
				{"pre{} \"\"\"hello\"\"\"\t{child} ", {{"pre"}, {"hello", {{"child"}}}}},
				{"\"pre\"\"\"\"hello\"\"\" ", {{"pre"}, {"hello"}}},
				{"\"pre\" \"\"\"hello\"\"\" ", {{"pre"}, {"hello"}}},
				{"\"\" \"\"\"hello\"\"\" ", {{""}, {"hello"}}},
				{"\"\" \"\"\"hello\"\"\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\"\"hello\" \"\"\"", {{"hello\" "}}},
				{"\"\"\"he\"\"llo\"\"\"", {{"he\"\"llo"}}},	// #14

				// if new line goes as very first or very last char of the raw string, then it is ignored.
				// python-style raw strings
				{"\"\" \"\"\"\nhello\"\"\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\" \"\"\"hello\n\"\"\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\" \"\"\"h\nello\n\"\"\"\"\" ", {{""}, {"h\nello"}, {""}}},
				{"\"\" \"\"\"\nhello\n\"\"\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\" \"\"\"hello\n\n\"\"\"\"\" ", {{""}, {"hello\n"}, {""}}},
				{"\"\" \"\"\"\n\nhello\"\"\"\"\" ", {{""}, {"\nhello"}, {""}}},
				{"\"\"\"\r\nhello\"\"\"", {{"hello"}}},
				{"\"\"\"hello\r\n\"\"\"", {{"hello"}}},
				{"\"\"\"\r\nhello\r\n\"\"\"", {{"hello"}}},
				{"\"\"\"\r \nhello\"\"\"", {{"\r \nhello"}}},
				{"\"\"\"hello\r \n\"\"\"", {{"hello\r "}}},
				{"\"\"\"\r\n\r\nhello\r\n\r\n\"\"\"", {{"\r\nhello\r\n"}}}, // #26

				// cpp-style raw strings
				{"\"\" R\"(\nhello)\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\" R\"(hello\n)\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\" R\"(h\nello\n)\"\"\" ", {{""}, {"h\nello"}, {""}}},
				{"\"\" R\"(\nhello\n)\"\"\" ", {{""}, {"hello"}, {""}}},
				{"\"\" R\"(hello\n\n)\"\"\" ", {{""}, {"hello\n"}, {""}}},
				{"\"\" R\"(\n\nhello)\"\"\" ", {{""}, {"\nhello"}, {""}}},
				{"R\"(\r\nhello)\"", {{"hello"}}},
				{"R\"(hello\r\n)\"", {{"hello"}}},
				{"R\"(\r\nhello\r\n)\"", {{"hello"}}},
				{"R\"(\r \nhello)\"", {{"\r \nhello"}}},
				{"R\"(hello\r \n)\"", {{"hello\r "}}},
				{"R\"(\r\n\r\nhello\r\n\r\n)\"", {{"\r\nhello\r\n"}}}, // #38

				// unquoted string terminated by comment
				{"hello//", {{"hello"}}},
				{"hello//\n", {{"hello"}}},
				{"hello//bla bla", {{"hello"}}},
				{"hello//bla bla\n", {{"hello"}}},
				{"hello/**/", {{"hello"}}},
				{"hello/*bla bla*/", {{"hello"}}},

				// parsing empty document
				{"", {}}, // #45
			},
			[](auto& p){
				auto r = treeml::read(p.first);
				tst::check_eq(r, p.second, SL);
			}
		);
});
}
