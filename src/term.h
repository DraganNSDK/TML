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

//struct intscmp {
//	bool operator()(const int_t& l, const int_t& r) const {
//		if (l < 0) return r >= 0 || l < r;
//		if (r < 0) return true;
//		return reverse(l) < reverse(r);
//	}
//	template <typename T>
//	T reverse(T n, size_t b) //  = sizeof(T) * CHAR_BIT
//	{
//		DBG(assert(b <= std::numeric_limits<T>::digits););
//		T rv = 0;
//		for (size_t i = 0; i < b; ++i, n >>= 1) rv = (rv << 1) | (n & 0x01);
//		return rv;
//	}
//};

struct term : public ints {
	bool neg = false, goal = false;
	enum textype { REL, EQ, LEQ, BLTIN, ALU } extype = term::REL;
	t_alu_op alu_op = NOP;
	ntable tab = -1;
	size_t orderid = 0;
	// D: TODO: builtins are very different, handle as a same size union struct?
	int_t idbltin = -1; // size_t bltinsize;
	// D: there's a dillema how to store types (where, how much, w/ any arg or 
	// just for vars). Just w/ vars for the moment, but I'm not sure.
	argtypes types;
	ints nums, irevs;
	term() {}
	term(bool neg, textype extype, t_alu_op alu_op, ntable tab, 
		const ints& args, const argtypes& types, const ints& nums,
		size_t orderid)
		: ints(args), neg(neg), extype(extype), alu_op(alu_op), tab(tab), 
		orderid(orderid), types(types), nums(nums) {
		//irevs = reverse(args, types);
	}
	term(bool neg, ntable tab, const ints& args, 
		const argtypes& types, const ints& nums, size_t orderid,
		int_t idbltin)
		: ints(args), neg(neg), extype(term::BLTIN), tab(tab), orderid(orderid),
		idbltin(idbltin), types(types), nums(nums) {
		//irevs = reverse(args, types);
	}
	bool operator<(const term& t) const {
		if (neg != t.neg) return neg;
		//if (extype != t.extype) return extype < t.extype;
		//if (isalu != t.isalu) return isalu;
		if (tab != t.tab) return tab < t.tab;
		if (goal != t.goal) return goal;
		// D: TODO: order types, bltin...
		// D: this's now more complex, if bits order changes can't just cmp ints
		//return irevs < t.irevs;
		return (const ints&)*this < t;
		//size_t i = 0;
		//const ints& l = (const ints&)*this;
		//const ints& r = (const ints&)t;
		//for (; i < l.size() && i < r.size(); ++i) {
		//	if (revless(l[i], r[i], i, types)) return true;
		//	if (revless(r[i], l[i], i, types)) return false;
		//}
		//return i == l.size() && !(i == r.size());
		//for (;(first1 != last1) && (first2 != last2); ++first1, (void)++first2){
		//	if (*first1 < *first2) return true;
		//	if (*first2 < *first1) return false;
		//}
		//return (first1 == last1) && (first2 != last2);
		//const ints& l = (const ints&)*this;
		//const ints& r = (const ints&)t;
		//return std::lexicographical_compare(
		//	l.begin(), l.end(), r.begin(), r.end(), intscmp());
	}

	inline ints reverse() const {
		const ints& args = (const ints&)*this;
		ints out(args.size());
		for (size_t i = 0; i < args.size(); ++i)
			if (args[i] > 0) {
				size_t bits = types[i].bitness;
				out[i] = reverse(args[i], bits);
			}
		return out;
	}

	static ints reverse(const ints& args, const argtypes& types) {
		ints out(args.size());
		for (size_t i = 0; i < args.size(); ++i)
			if (args[i] > 0) { 
				size_t bits = types[i].bitness;
				out[i] = reverse(args[i], bits);
			}
		return out;
	}
	static inline bool revless(
		const int_t& l, const int_t& r, size_t i, const argtypes& types) {
		if (l < 0) return r >= 0 || l < r;
		if (r < 0) return true;
		size_t bits = types[i].bitness;
		return reverse(l, bits) < reverse(r, bits);

	}
	//template <typename T> 
	static inline int_t reverse(int_t n, size_t b) //  = sizeof(T) * CHAR_BIT
	{
		//DBG(assert(b <= std::numeric_limits<T>::digits););
		int_t rv = 0;
		for (size_t i = 0; i < b; ++i, n >>= 1) rv = (rv << 1) | (n & 0x01);
		return rv;
	}
	void replace(const std::map<int_t, int_t>& m);
};

struct cmptermrev {
	bool operator()(const term& l, const term& r) const {
		if (l.orderid != r.orderid) return l.orderid < r.orderid;
		if (l.neg != r.neg) return l.neg;
		if (l.goal != r.goal) return l.goal;
		return l.irevs < r.irevs;
		//return (const ints&)l < r;
	}
};

std::wostream& operator<<(std::wostream& os, const term& t);
std::vector<term> to_vec(const term& h, const std::set<term>& b);
template<typename T> std::set<T> vec2set(const std::vector<T>& v, size_t n = 0);

#endif // __TERM_H__