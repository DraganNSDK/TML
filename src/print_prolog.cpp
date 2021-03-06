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
#include "driver.h"
using namespace std;

typedef pair<wstring, int_t> relarity;
typedef set<relarity> relarities;

relarity get_relarity(const raw_term& t);
void get_relarities(const raw_prog& p, relarities& ras);
wostream& output_prolog_rule(wostream& os, const raw_rule& r);
wostream& output_prolog_term(wostream& os, const raw_term& t);
wostream& output_prolog_elem(wostream& os, const elem& e);

#define output_lexeme_adjust_first(os, l, fn) (os) << (wchar_t)fn(*((l)[0])) <<\
	((l)[1]-((l)[0]+1)>0 ? lexeme{(l)[0]+1,(l)[1]} : lexeme{(l)[0], (l)[0]})

wostream& driver::print_xsb(wostream& os, const raw_prog& rp) const {
	return print_prolog(os, rp, XSB);
}

wostream& driver::print_swipl(wostream& os, const raw_prog& rp) const {
	return print_prolog(os, rp, SWIPL);
}

wostream& driver::print_prolog(wostream& os, const raw_prog& p,
	const prolog_dialect d) const {
	relarities ras;
	get_relarities(p, ras);
	wstring name = d == SWIPL ? L"SWI Prolog" : L"XSB";
	os << L"% start of " << name << " program" << endl;
	os << endl;
	os << L"% enable tabling to avoid inf. loops" << endl;
	for (auto ra : ras) os << L":- table " << ra.first <<
		L'/' << ra.second << L'.' << endl;
	os << endl;
	if (d == SWIPL) {
		os << L"% suppress singleton warnings" << endl;
		os << L":- style_check(-singleton)." << endl;
		os << endl;
		os << L"% enable discontiguation" << endl;
		for (auto ra : ras) os << L":- discontiguous " << ra.first <<
			L'/' << ra.second << L'.' << endl,
		//for (auto ra : ras)
			os << L":- discontiguous '" << ra.first <<
			L" tabled'" << L'/' << ra.second << L'.' << endl;
		os << endl;
		os << L"% declare dynamic predicates" << endl;
		for (auto ra : ras) os << L":- dynamic '" << ra.first <<
			L" tabled'" << L'/' << ra.second << L'.' << endl;
		os << endl;
	}
	os << L"% {" << endl;
	for (auto x : p.r) output_prolog_rule(os, x) << endl;
	os << L"% }" << endl;
	os << endl;
	os << L"% find all and dump to stdout" << endl;
	os << L"dump_list([])." << endl;
	os << L"dump_list([H|T]) :- writeln(H), dump_list(T)."<< endl;
	os << L"dump(Q) :- findall(Q, Q, X), dump_list(X)." << endl;
	for (auto ra : ras) {
		os << L":- dump(" << ra.first << L'(';
		for(int_t i = 0; i != ra.second; ++i) os << (i ? L",_" : L"_");
		os << L"))." << endl;
	}
	os << endl;
	os << L":- halt." << endl;
	os << endl;
	os << L"% end of "<< name << " program" << endl;
	return os;
}

relarity get_relarity(const raw_term& t) {
	wstring rel = lexeme2str(t.e[0].e);
	rel[0] = towlower(rel[0]);
	int_t depth = 0, ar = t.arity[0];
	if (ar == 0)
		for (int_t a : t.arity) {
			if (a == -1) depth++;
			else if (a == -2 && !--depth) ar++;
		}
	return { rel, ar };
}

void get_relarities(const raw_prog& p, relarities& ras) {
	for (auto r : p.r) {
		for (auto t : r.h)
			ras.insert(get_relarity(t));
		for (auto b : r.b) for (auto t : b)
			ras.insert(get_relarity(t));
	}
}

wostream& output_prolog_rule(wostream& os, const raw_rule& r) {
	switch (r.type) {
		case raw_rule::GOAL: os << L"% !" << endl; break;
		case raw_rule::TREE: os << L"% !!" << endl; break;
		default: ;
	}
	for (size_t n = 0; n < r.h.size(); ++n)
		if (output_prolog_term(os, r.h[n]), n != r.h.size() - 1)
			os << L',';
	if (!r.b.size()) return os << L'.';
	os << L" :- ";
	for (size_t m = 0; m < r.b.size(); ++m) {
		for (size_t n = 0; n < r.b[m].size(); ++n)
			if (output_prolog_term(os, r.b[m][n]),
				n != r.b[m].size() - 1) os << L',';
		if (m != r.b.size() - 1) os << L';';
	}
	return os << L'.';
}

wostream& output_prolog_term(wostream& os, const raw_term& t) {
	if (t.neg) os << L'~';
	output_prolog_elem(os, t.e[0]);
	os << L'(';
	for (size_t ar = 0, n = 1; ar != t.arity.size();) {
		while (t.arity[ar] == -1) ++ar, os << L'(';
		if (n >= t.e.size()) break;
		while (t.e[n].type == elem::OPENP) ++n;
		for (int_t k = 0; k != t.arity[ar];)
			if (output_prolog_elem(os,t.e[n++]), ++k != t.arity[ar])
				os << L", ";
		while (n < t.e.size() && t.e[n].type == elem::CLOSEP) ++n;
		++ar;
		while (ar < t.arity.size() && t.arity[ar] == -2)
			++ar, os << L')';
		if (ar > 0 && t.arity[ar-1] == -2 && ar != t.arity.size())
			os << ", ";
	}
	return os << L')';
}

wostream& output_prolog_elem(wostream& os, const elem& e) {
	switch (e.type) {
		case elem::CHR:
			os << '\'';
			switch (e.ch) {
				case L'\'':
				case L'\\': os << L'\\' << e.ch; break;
				case L'\r': os << L"\\r"; break;
				case L'\n': os << L"\\n"; break;
				case L'\t': os << L"\\t"; break;
				default: os << e.ch;
			}
			return os << L'\'';
		case elem::VAR: return output_lexeme_adjust_first(os,
			(lexeme{ e.e[0]+1, e.e[1] }), towupper);
		case elem::OPENP:
		case elem::CLOSEP: return os << *e.e[0];
		case elem::NUM: return os << e.num;
		default: return output_lexeme_adjust_first(os, e.e, towlower);
	}
}
