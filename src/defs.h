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
#ifndef __DEFS_H__
#define __DEFS_H__
#include <cassert>
#include <vector>
#include <set>
#include <unordered_map>
#include <array>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <map>

#ifndef __EMSCRIPTEN__
#include <execinfo.h>
#endif

//#include "types.h"

typedef int32_t int_t;
typedef uint32_t uint_t;
typedef std::vector<uint_t> uints;

typedef wchar_t* wstr;
typedef const wchar_t* cws;
typedef cws* pcws;
typedef std::array<cws, 2> cws_range;
typedef cws_range lexeme;
typedef std::vector<lexeme> lexemes;
typedef std::vector<int_t> ints;
struct lexcmp { bool operator()(const lexeme& x, const lexeme& y) const; };
typedef std::map<lexeme, std::wstring, lexcmp> strs_t;
typedef std::vector<bool> bools;
typedef std::vector<bools> vbools;
typedef int_t ntable;
typedef size_t nlevel;
//typedef std::vector<size_t> sizes;

//#define DEEPDEBUG
#ifdef DEBUG
#define DBG(x) x
#define NDBG(x)
#else
#define DBG(x)
#define NDBG(x) x
#endif
#define er(x) o::err() << x << endl, throw std::runtime_error(ws2s(x))
#define msb(x) ((sizeof(unsigned long long)<<3) - \
	__builtin_clzll((unsigned long long)(x)))
#define has(x, y) ((x).find(y) != (x).end())
#define hasb(x, y) std::binary_search(x.begin(), x.end(), y)
#define hasbc(x, y, f) std::binary_search(x.begin(), x.end(), y, f)
#define measure_time_start() start = clock()
#define measure_time_end() end = clock(), \
		o::ms() << std::fixed << std::setprecision(2) << \
		(double(end - start) / CLOCKS_PER_SEC) * 1000 \
		<< L" ms" << endl
#define measure_time(x) measure_time_start(); x; measure_time_end()
#define elem_openp elem(elem::OPENP, get_lexeme(L"("))
#define elem_closep elem(elem::CLOSEP, get_lexeme(L")"))
#define htrue bdd_handle::T
#define hfalse bdd_handle::F
template<typename T> T sort(const T& x){T t=x;return sort(t.begin(),t.end()),t;}
void parse_error(std::wstring e, lexeme l);
std::wstring s2ws(const std::string&);
std::string  ws2s(const std::wstring&);

namespace o { // call driver::init() before using any o::xxx() wostream
	std::wostream& out(); // for program output (in tml facts)
	std::wostream& err(); // for errors
	std::wostream& inf(); // for info (for debugging in Release)
	std::wostream& dbg(); // for debugging (point to null if not Debug)
	std::wostream& repl();// for REPL
	std::wostream& ms();  // benchmarks output for time measurings
}

typedef enum  {
	NOP, ADD, SUB, MULT, BITWOR, BITWAND, BITWXOR, SHR, SHL
} t_alu_op;

// argument type's base-type enum 
////enum class basetype : std::uint8_t { NONE = 0, INT, CHR, STR };
//enum class base_type { NONE = 0, INT, CHR, STR };
//
//// D: make this just an int_t, lower bits for type + bitness the rest.
//struct arg_type {
//	base_type type = base_type::NONE;
//	size_t bitness;
//	arg_type() : type(base_type::NONE), bitness(0) {}
//	arg_type(base_type type, size_t bitness) : type(type), bitness(bitness) {}
//};
//
//typedef std::vector<arg_type> argtypes;
//
//struct tbl_arg {
//	ntable tab;
//	size_t arg;
//	tbl_arg(ntable tab, size_t arg) : tab(tab), arg(arg) {}
//	tbl_arg(const alt_arg& aa) : tab(aa.tab), arg(aa.arg) {
//		DBG(assert(aa.alt == -1););
//	}
//	inline bool operator<(const tbl_arg& other) const {
//		if (tab != other.tab) return tab < other.tab;
//		return arg < other.arg;
//	}
//	inline bool operator==(const tbl_arg& other) const {
//		return tab == other.tab && arg == other.arg;
//	}
//	inline bool operator!=(const tbl_arg& r) { return !operator==(*this, r); }
//	inline bool operator> (const tbl_arg& r) { return  operator<(r, *this); }
//	inline bool operator<=(const tbl_arg& r) { return !operator>(*this, r); }
//	inline bool operator>=(const tbl_arg& r) { return !operator<(*this, r); }
//};
////inline bool operator!=(const tbl_arg& l, const tbl_arg& r)
////{ return !operator==(l, r); }
////inline bool operator> (const tbl_arg& l, const tbl_arg& r)
////{ return  operator<(r, l); }
////inline bool operator<=(const tbl_arg& l, const tbl_arg& r)
////{ return !operator>(l, r); }
////inline bool operator>=(const tbl_arg& l, const tbl_arg& r)
////{ return !operator<(l, r); }
//
//struct alt_arg {
//	ntable tab;
//	int_t alt;
//	size_t arg;
//	alt_arg(ntable tab, int_t alt, size_t arg) : tab(tab), alt(alt), arg(arg) {}
//	alt_arg(const tbl_arg& ta) : tab(ta.tab), alt(-1), arg(ta.arg) {}
//	bool operator<(const alt_arg& aa) const {
//		if (tab != aa.tab) return tab < aa.tab;
//		if (alt != aa.alt) return alt < aa.alt;
//		return arg < aa.arg;
//	}
//};
//
////bool operator<(const alt_arg& aarg, const tbl_arg& ta);

#endif // __DEFS_H__
//#define TRANSFORM_BIN_DRIVER
