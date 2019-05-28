// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <algorithm>
#include <string>
#include <set>

typedef enum dialect { TRANSFORMED, XSB, SWIPL, SOUFFLE } dialect;
typedef enum format { F_TML, F_CSV } format;
typedef enum ast_format { AST_TML, AST_JSON, AST_XML, AST_HTML } ast_format;

#define if_opt(o)       if (option(arg, (o)))
#define if_opts(o1, o2) if (option(arg, (o1)) || option(arg, (o2)))

struct options {
	std::set<format> formats = { F_TML };
	std::set<dialect> dialects = {};
	std::set<ast_format> ast_formats = {};
	options() {}
	options(int argc, char** argv) { parse(argc, argv); }
	void parse(int c, char** v) {
		for (int i = 1; i < c; ++i) {
			std::string arg = std::string(v[i]);
			if_opts("t", "transformed")dialects.insert(TRANSFORMED);
			else if_opt("xsb")         dialects.insert(XSB);
			else if_opt("swipl")       dialects.insert(SWIPL);
			else if_opt("souffle")     dialects.insert(SOUFFLE);
			else if_opt("csv")         formats.insert(F_CSV);
			else if_opt("ast")         ast_formats.insert(AST_TML);
			else if_opt("ast-json")    ast_formats.insert(AST_JSON);
			else if_opt("ast-xml")     ast_formats.insert(AST_XML);
			else if_opt("ast-html")    ast_formats.insert(AST_HTML);
		}
	}
	bool option(const std::string arg, const std::string o) const {
		return (arg == "--"+o || arg == "-"+o);
	}
	template<typename T>bool enabled(const std::set<T> &s, const T &o)const{
		auto it = std::find (s.begin(), s.end(), o);
		return (it != s.end());
	}
	bool enabled_dialect(const dialect d)const {return enabled(dialects,d);}
	bool enabled_format(const format f) const  { return enabled(formats,f);}
	bool enabled_ast_format(ast_format t) const {
		return enabled(ast_formats, t); }
};
