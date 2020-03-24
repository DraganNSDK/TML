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
#ifndef __TERM_H__
#define __TERM_H__
#include "defs.h"
#include "types.h"

struct term : public ints {
	bool neg = false, goal = false;
	enum textype { REL, EQ, LEQ, BLTIN, ARITH } extype = term::REL;
	t_arith_op arith_op = NOP;
	ntable tab = -1;
	size_t orderid = 0;
	// D: TODO: builtins are very different, handle as a same size union struct?
	int_t idbltin = -1; // size_t bltinsize;
	// D: there's a dillema how to store types (where, how much, w/ any arg or 
	// just for vars). Just w/ vars for the moment, but I'm not sure.
	argtypes types;
	ints nums;
	term() {}
	term(bool bneg, textype xtype, t_arith_op arith_op, ntable t,
		 const ints& args, const argtypes& ts, const ints& ns, size_t oid)
		: ints(args), neg(bneg), extype(xtype), arith_op(arith_op), tab(t),
		orderid(oid), types(ts), nums(ns) {}
	term(bool bneg, ntable t, const ints& args, const argtypes& ts,
		 const ints& ns, size_t oid, int_t idbltin)
		: ints(args), neg(bneg), extype(term::BLTIN), tab(t), orderid(oid),
		idbltin(idbltin), types(ts), nums(ns) {}
	bool operator<(const term& t) const {
		if (neg != t.neg) return neg;
		//if (extype != t.extype) return extype < t.extype;
		if (tab != t.tab) return tab < t.tab;
		if (goal != t.goal) return goal;
		// D: TODO: order types, bltin...
		return (const ints&)*this < t;
	}
	void replace(const std::map<int_t, int_t>& m);
};

std::wostream& operator<<(std::wostream& os, const term& t);
std::vector<term> to_vec(const term& h, const std::set<term>& b);
template<typename T> std::set<T> vec2set(const std::vector<T>& v, size_t n = 0);

#endif // __TERM_H__