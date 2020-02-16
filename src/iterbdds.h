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
#ifndef __ITERBDDS_H__
#define __ITERBDDS_H__
#include <map>
#include <vector>
#include "bdd.h"
#include "term.h"
#include "bitsmeta.h"
#include "dict.h"
#include "defs.h"
class tables;
class alt;

struct iterbdds {
	tables& rtbls;
	std::set<ntable> tdone;
	std::set<std::tuple<ntable, size_t>> rdone;
	std::set<std::tuple<ntable, size_t, size_t>> altdone;
	std::map<std::pair<ntable, size_t>, perminfo> tblperms;
	std::map<std::tuple<ntable, size_t, size_t>, perminfo> altperms;
	std::vector<bits_perm> vperms;

	iterbdds(tables& tbls) :rtbls(tbls) {}

	void clear() {
		vperms.clear(); // clear any previous that are applied
		altperms.clear();
		tblperms.clear();
		altdone.clear();
		rdone.clear();
		tdone.clear();
	}

	bool permute_table(ntable tab, size_t arg);
	inline bool permute_table(const std::pair<ntable, size_t>& targ, 
		std::map<ntable, bits_perm>& argtbls, size_t bits, base_type type) {
		ntable tab = targ.first;
		size_t arg = targ.second;
		return permute_table(tab, arg, argtbls, bits, type);
	}
	bool permute_table(ntable tab, size_t arg, 
		std::map<ntable, bits_perm>& argtbls, size_t bits, base_type type);
	bool permute_bodies(ntable tab, alt& a, 
		std::map<ntable, bits_perm>& argtbls, const bits_perm& altperm,
		size_t bits, base_type type);
	bool permute_alt(ntable tab, size_t arg, size_t n, alt& a, 
		std::map<ntable, bits_perm>& argtbls, size_t bits, base_type type);
	bool permute_all();
};

#endif // __ITERBDDS_H__