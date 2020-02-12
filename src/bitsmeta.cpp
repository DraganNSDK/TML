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
#include "bitsmeta.h"
#include "dict.h"
#include "input.h"
//#include "output.h"
#include "err.h"
using namespace std;

// this is going away anyways, copy it for now, who cares
//#define mkchr(x) ((((int_t)x)<<2)|1)
//#define mknum(x) ((((int_t)x)<<2)|2)
//#define mksym(x) (int_t(x)<<2)
//#define un_mknum(x) (int_t(x)>>2)
#define mkchr(x) (int_t(x))
#define mknum(x) (int_t(x))
#define mksym(x) (int_t(x))
#define un_mknum(x) (int_t(x))

/* prepare bits, bitness, caches if any */
void bitsmeta::init(const dict_t& dict) {
	// vargs should be set before entering, or rerun this on ordering change.
	mleftbits.clear();
	size_t lsum = 0, args = types.size(), maxb = 0;
	mleftbits[vargs[0]] = lsum;
	for (size_t i = 0; i != types.size(); ++i) {
		//arg_type& type = types[i]; // this is a bug
		arg_type& type = types[vargs[i]];
		// just temp, update all int, chr, str bits w/ 2 extra bits (as before).
		switch (type.type) {
			case base_type::STR:
				// D: Q: TODO: how to set up individual arg's sym universe?
				// always init for STR/CHR or not? may alt universe size differ?
				if (type.bitness == 0) {
					type.bitness = BitScanR(dict.nsyms()); // nsyms-1? I guess
					type.bitness += 2; // ...will be removed
				}
				break;
			case base_type::CHR:
				// it's always 8, always init (or if correct it's 0)
				if (type.bitness == 0) {
					type.bitness = 8;
					type.bitness += 2; // ...will be removed
				}
				break;
			case base_type::INT: 
				if (type.bitness == 0) {
					// calc bitness for ints (we just have nums at this point).
					type.bitness = BitScanR(nums[i]); // un_mknum(args[i])
					type.bitness += 2; // ...will be removed
				}
				break;
			case base_type::NONE:
				type.bitness = 1; // 8;
				type.bitness += 2; // ...will be removed
			default: ;
		}
		// init vbits (temp cache), any other caching if needed
		//vbits[i] = types[i].bitness;

		//if (vargs[i] == arg) break;
		if (i != args-1) {
			lsum += types[vargs[i]].bitness;
			mleftbits[vargs[i+1]] = lsum;
		}
		maxb = max(maxb, types[vargs[i]].bitness);
	}
	args_bits = mleftbits.at(vargs[args-1]) + types[vargs[args-1]].bitness;
	maxbits = maxb;

	size_t argsum = 0;
	if (maxbits == 0) {
		return;
	}
	for (int_t bit = maxbits-1; bit >= 0; --bit) {
		map<size_t, size_t>& mpos = mleftargs[bit];
		for (size_t arg = 0; arg != types.size(); ++arg)
			if (types[vargs[arg]].bitness > size_t(bit))
				mpos[vargs[arg]] = argsum++;
	}
	DBG(assert(argsum == args_bits););
}

bool bitsmeta::set_args(
	const ints& args, const argtypes& vtypes, const ints& vnums) {
	DBG(assert(vtypes.size() > 0);); // don't call this if nothing to do
	DBG(assert(args.size() == vtypes.size()););
	DBG(assert(args.size() == vnums.size()););
	// we're empty initialized already (to table len), so sizes need to match
	DBG(assert(types.size() == vtypes.size()););
	// !nterms meaning we have no previous types / bits data (size's always >0)
	if (!nterms) { // types.size() == 0)
		types = vtypes;
		nums = vnums;
	} else {
		for (size_t i = 0; i != types.size(); ++i) {
			// D: TODO: use update_types instead but we need some testing
			//update_types(vtypes, vnums);
			arg_type& type = types[i];
			const arg_type& newtype = vtypes[i];
			if (newtype.type == base_type::NONE) continue; // not set, skip
			if (type.type == base_type::NONE) 
				type = newtype; // first init...
			if (type.type != newtype.type) 
				parse_error(err_type, L""); //lexeme?
			if (type.type == base_type::INT) 
				nums[i] = max(nums[i], vnums[i]); // no need if NONE but cheap
			// we may not need this, it's 0 except for alt's (inheriting, once)
			if (newtype.bitness > type.bitness) 
				type.bitness = newtype.bitness; 
			//if (type.type == base_type::INT) nums[i] = vnums[i];
			//if (isset && type.type == base_type::INT) // calc bitness for ints
			//	type.bitness = BitScanR(un_mknum(args[i]), type.bitness);
		}
	}
	++nterms;
	return true;
}

/* 
we're init already, this is just to update table back from alt/rules 
not entirely nice but handy to sync types in between tbls, rules, alts, for now
*/
void bitsmeta::update_types(const argtypes& vtypes, const ints& vnums) {
	DBG(assert(types.size() <= vtypes.size()););
	for (size_t i = 0; i != types.size(); ++i) {
		arg_type& type = types[i];
		const arg_type& newtype = vtypes[i];
		if (newtype.type == base_type::NONE) continue; // not set, skip
		if (type.type == base_type::NONE)
			type = newtype; // first init...
		if (type.type != newtype.type)
			parse_error(err_type, L""); //lexeme?
		if (type.type == base_type::INT)
			nums[i] = max(nums[i], vnums[i]); // no need if NONE but cheap
		// we may not need this, it's 0 except for alt's (inheriting, once)
		if (newtype.bitness > type.bitness)
			type.bitness = newtype.bitness;
		//if (isset && type.type == base_type::INT) // calc bitness for ints
		//	type.bitness = BitScanR(un_mknum(args[i]), type.bitness);
	}
}


