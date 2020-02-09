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
#include <list>
#include "iterbdds.h"
#include "tables.h"
#include "dict.h"
#include "input.h"
#include "err.h"
using namespace std;

#define add_bit rtbls.add_bit
#define add_bit_perm rtbls.add_bit_perm
#define permex_add_bit rtbls.permex_add_bit
#define deltail rtbls.deltail
#define tbls rtbls.tbls
//#define rules rtbls.rules
//#define depends rtbls.depends

bool iterbdds::permute_table(ntable tab, size_t arg) {
	map<ntable, bits_perm> argtbls;
	table& tb = tbls[tab];
	size_t bits = tb.bm.types[arg].bitness + 1;
	base_type type = tb.bm.types[arg].type;
	permute_table(tab, arg, argtbls, bits, type);
}

bool iterbdds::permute_table(ntable tab, size_t arg, 
	std::map<ntable, bits_perm>& argtbls, size_t bits, base_type type) {
	if (tab == -1) return false; // continue;
	table& tb = tbls[tab];
	if (has(tdone, tab)) {
		perminfo& rinfo = tblperms[{tab, arg}];
		argtbls[tab] = { tab, arg, tb.len, rinfo };
		return false; // continue;
	}
	//size_t args = tb.len; // .bm.get_args();
	DBG(assert(tb.bm.types[arg].bitness + 1 == bits););
	DBG(assert(tb.bm.types[arg].type == type););

	perminfo info = add_bit_perm(tb.bm, arg, tb.len);
	DBG(assert(info.bm.get_args() == tb.len););
	tblperms[{tab, arg}] = info;
	tb.bm = info.bm;
	tb.t = add_bit(tb.t, info, arg, tb.len);
	for (spbdd_handle& tadd : tb.add)
		tadd = add_bit(tadd, info, arg, tb.len);
	for (spbdd_handle& tdel : tb.del)
		tdel = add_bit(tdel, info, arg, tb.len);
	// stack to do all its dependencies (including rule)
	bits_perm tperm = { tab, arg, tb.len, move(info) };
	argtbls[tab] = tperm; // save to process body below
	vperms.push_back(tperm);
	return true;
}

bool iterbdds::permute_bodies(ntable tab, alt& a, 
	std::map<ntable, bits_perm>& argtbls, size_t bits, base_type type) {
	for (size_t n = 0; n != a.size(); ++n) {
		body& b = *a[n];
		// a.depends is no good here, we need tables per arg
		if (b.tab == tab || has(argtbls, b.tab)) {
			bits_perm& p = argtbls[b.tab];
			// permute body bdd-s (eq, last...)
			DBG(assert(p.perm.bm.get_bits(p.arg) == bits););
			DBG(assert(p.perm.bm.types[p.arg].type == type););
			b.q = add_bit(b.q, p.perm, p.arg, p.args);
			b.tlast = add_bit(b.tlast, p.perm, p.arg, p.args);
			b.rlast = add_bit(b.rlast, p.perm, p.arg, p.args);
			auto pex = 
				permex_add_bit(b.vals, a.vm, p.perm.bm, a.bm);
			b.ex = pex.first;
			b.perm = pex.second;
		}
	}
	return true;
}

bool iterbdds::permute_alt(ntable tab, size_t arg, size_t n, alt& a, 
	std::map<ntable, bits_perm>& argtbls, size_t bits, base_type type) {
	size_t args = a.bm.get_args(); // a.varslen;
	DBG(assert(a.bm.types[arg].bitness + 1 == bits););
	DBG(assert(a.bm.types[arg].type == type););
	perminfo info = add_bit_perm(a.bm, arg, args);
	altperms[{tab, arg, n}] = info;
	a.bm = info.bm;
	a.eq = add_bit(a.eq, info, arg, args);
	a.rng = add_bit(a.rng, info, arg, args);
	a.rlast = add_bit(a.rlast, info, arg, args);
	for (spbdd_handle& al : a.last)
		al = add_bit(al, info, arg, args);
	auto pex = deltail(a.bm, tbls[tab].bm);
	a.ex = pex.first;
	a.perm = pex.second;
	// a.vm remains the same (it's just indexes, no bits)
	// also get all other pairs for that arg (other rels)
	if (has(a.argsdep, arg)) {
		// iter over each table and add bit in bm, update tbl
		for (pair<ntable, size_t>& targ : a.argsdep[arg]) {
			permute_table(targ, argtbls, bits, type);
		}
	}
	permute_bodies(tab, a, argtbls, bits, type);
	return true;
}

// vector<bits_perm>& vperms, map<pair<ntable, size_t>, perminfo>& tblperms
bool iterbdds::permute_all() {
	//int_t ipop = vperms.size()-1; ipop >= 0; bits_perm& perm=vperms[ipop--];
	//set<ntable> tdone;
	//set<tuple<ntable, size_t>> rdone;
	//set<tuple<ntable, size_t, size_t>> altdone;
	////map<pair<ntable, size_t>, perminfo> tblperms;
	//map<tuple<ntable, size_t, size_t>, perminfo> altperms;
	size_t bits = 0;
	while (!vperms.empty()) {
		bits_perm perm = move(vperms.back()); 
		vperms.pop_back(); // optimize later
		if (has(tdone, perm.tab)) 
			continue; // shouldn't happen though?
		tdone.insert(perm.tab); // process a table at most once
		DBG(assert(bits == 0 || bits == perm.perm.bm.types[perm.arg].bitness););
		bits = perm.perm.bm.types[perm.arg].bitness;
		base_type type = perm.perm.bm.types[perm.arg].type;
		// D: this is brute force, un-optimized, we make too many passes
		for (rule& r : rtbls.rules) {
			//set<ntable>& dpnds = depends[perm.tab];
			if (r.tab == perm.tab) { 
				ntable tab = perm.tab; // r.tab;
				size_t arg = perm.arg;
				// look for other rel/tbls we depend on + permute all rule bdds
				DBG(assert(r.t.size() > arg););
				int_t var = r.t[arg];
				if (var >= 0) continue; // this invalidates all alt-s as well
				tuple<ntable, size_t> tblargid = { tab, arg };
				if (has(rdone, tblargid))
					continue;
				rdone.insert(tblargid);
				// perm all r bdd-s (eq, last)...
				// what should bm be for rule? use table's?
				// use table stuff, we just need full perm info, perm uints too.
				r.eq = add_bit(r.eq, perm.perm, arg, perm.args);
				r.rlast = add_bit(r.rlast, perm.perm, arg, perm.args);
				r.h = add_bit(r.h, perm.perm, arg, perm.args);
				for(spbdd_handle& rl : r.last)
					rl = add_bit(rl, perm.perm, arg, perm.args);
				for (size_t n = 0; n != r.size(); ++n) {
					alt& a = *r[n];
					map<ntable, bits_perm> argtbls;
					argtbls[tab] = perm;
					// perm all alt bdd-s eq, last, rng, we don't care about vm
					permute_alt(tab, arg, n, a, argtbls, bits, type);
					// D: what about EQ etc.? (non relations, w/ no tbl behind)
				}
			} else if (has(rtbls.depends[r.tab], perm.tab)) {
				ntable tab = r.tab;
				//size_t arg = perm.arg;
				// look for rules/tbls we depend on...
				int_t rulearg = -1; // numeric_limits<size_t>::max();
				for (size_t n = 0; n != r.size(); ++n) {
					alt& a = *r[n];
					if (!has(a.depends, perm.tab)) continue;
					pair<ntable, size_t> tblarg = { perm.tab, perm.arg };
					if (!has(a.invargsdep, tblarg)) continue; // ?
					vector<size_t>& vinv = a.invargsdep[tblarg];
					// add_bit for each arg in the vector (normally one)
					//DBG(assert(vinv.size() == 1););
					size_t arg, nargs = 0; // = vinv[0]; 
					// process as if just one // TODO
					for (size_t anarg : vinv)
						if (anarg < r.len) { arg = anarg; ++nargs; }
					DBG(assert(nargs <= 1););
					if (nargs == 0) arg = vinv[0]; // only temp vars, take any
					//DBG(assert(r.t[arg] < 0);); // dependencies are w/ vars
					map<ntable, bits_perm> argtbls;
					DBG(assert(rulearg == -1 || nargs == 0 || 
						size_t(rulearg) == arg););
					tuple<ntable, size_t> tblargid = { tab, arg };
					if (rulearg == -1 && has(rdone, tblargid))
						continue;
					// we should cache this info w/ rule, to do this first, tbl.
					if (rulearg == -1 && nargs != 0) { // only if rule vars
						rdone.insert(tblargid);
						rulearg = arg;
						DBG(assert(arg < r.t.size()););
						int_t var = r.t[rulearg];
						DBG(assert(var < 0);); // dependencies are only w/ vars

						if (permute_table(tab, arg, argtbls, bits, type)) {
							perminfo& info = vperms.back().perm;
							size_t len = info.bm.get_args();
							r.eq = add_bit(r.eq, info, arg, len);
							r.rlast = add_bit(r.rlast, info, arg, len);
							for (spbdd_handle& rl : r.last)
								rl = add_bit(rl, info, arg, len);
						} else {
							continue; // ? we should just go on to alt stuff
						}
					}
					DBG(assert(a.inv[arg] < 0););
					// permute all related bdd-s

					permute_alt(tab, arg, n, a, argtbls, bits, type);
				}
			}
		}
	}
	return true;
}

/* add a bit to an arg (as it's varbit now) */
//bits_perm tables::add_bit(ntable tab, size_t arg) {
//	table& tbl = tbls[tab];
//	spbdd_handle x = tbl.t;
//	size_t args = tbl.len;
//
//	// - make new bm (just for this arg-s bits+1)
//	// - then init new bits to '0' (bdd wise)
//	// ...and I guess important is the bits layout (bits-1st, args-1st, varying)
//	// perm is different now, sum of variable bits per each arg (like leftbits)
//	perminfo perm = add_bit_perm(tbl.bm, arg, args);
//
//	//bitsmeta newbm(tbl.bm, arg, 1); // make new bm, increment arg's bits
//	//uints perm = perm_init(tbl.bm.args_bits); // tbl.bm.perm_init(); 
//	//// map from old to new pos, affects all args, for the arg shift bits by 1, 
//	//// free up 0 bit, for others it's the same bit<->bit (pos may still change)
//	//// (bits+1) is handled internally by newbm.pos (it 'knows' each arg's bits)
//	//for (size_t n = 0; n != args; ++n)
//	//	for (size_t b = 0; b != tbl.bm.types[n].bitness; ++b)
//	//		if (n==arg) perm[tbl.bm.pos(b, n, args)] = newbm.pos(b+1, n, args);
//	//		else		perm[tbl.bm.pos(b, n, args)] = newbm.pos(b, n, args);
//
//	// D: this permutes/reorders previous bdd bits to new bits 
//	// we shift to left, as it's counted from right, leaving the top/0 bit free
//	// args-first and fixed pos/bits did make this easy to do, to keep in mind
//	bdd_handles v = { x ^ perm.perm };
//
//	// 'zero-out' just for the arg (other args didn't shift)
//	v.push_back(::from_bit(perm.bm.pos(0, arg, args), false));
//	tbl.t = bdd_and_many(move(v));
//	//bitsmeta& oldbm = tbl.bm;
//	tbl.bm = perm.bm;
//	return {tab, arg, args, move(perm)};
//
//	// D: add_bit adds 'zeros' for the last bit (all args), on universe change.
//	// this is problematic bits/pos wise, at a first glance what bm to use? 
//	// we don't know bits yet, as we add new bits, bits/size changes, but we're 
//	// relying on the bdd, bits ordering to stay the same i.e.=> args-first!!
//	// it gets even more complicated, we don't know the arg type / arg bitness?
//	//for (size_t n = 0; n != args; ++n)
//	//	v.push_back(::from_bit(pos(0, bits + 1, n, args), false));
//	//return tbl.t = bdd_and_many(move(v));
//}

//ntable tab = r.tab;
//table& tb = tbls[tab];
//if (!has(tdone, tab)) {
//	DBG(assert(tb.bm.types[arg].bitness + 1 == bits););
//	DBG(assert(tb.bm.types[arg].type == type););
//	perminfo info = add_bit_perm(tb.bm, arg, tb.len);
//	tblperms[{tab, arg}] = info;
//	tb.bm = info.bm;
//	tb.t = add_bit(tb.t, info, arg, tb.len);
//	for (spbdd_handle& tadd : tb.add)
//		tadd = add_bit(tadd, info, arg, tb.len);
//	for (spbdd_handle& tdel : tb.del)
//		tdel = add_bit(tdel, info, arg, tb.len);
//	r.eq = add_bit(r.eq, info, arg, tb.len);
//	r.rlast = add_bit(r.rlast, info, arg, tb.len);
//	for (spbdd_handle& rl : r.last)
//		rl = add_bit(rl, info, arg, tb.len);
//	bits_perm tperm = { tab, arg, tb.len, move(info) };
//	argtbls[tab] = tperm; // save to process below
//	vperms.push_back(tperm);
//} else {
//	perminfo& rinfo = tblperms[{tab, arg}];
//	argtbls[tab] = { tab, arg, tb.len, rinfo };
//	continue;
//}
