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
#ifndef __BITSMETA_H__
#define __BITSMETA_H__
#include <map>
#include <vector>
#include "defs.h"
#include "bdd.h"
class dict_t;

// D: TODO: this is a temp fix to bring table/alt to share data, pass around.
// I'd rather do this by making pos and methods be part of alt/table, or similar
struct bitsmeta {
	// types.size() is always set at args (or table.len) right after .ctor
	argtypes types;
	// D: we need to track exact 'nums' for the range leq (bits are not enough).
	ints nums;
	std::vector<size_t> vargs;
	size_t nterms = 0; // D: # of terms/types processed so far.
	std::map<size_t, size_t> mleftbits;
	size_t args_bits = 0; // like args * bits (just now variable sum)

	size_t maxbits;
	std::map<size_t, std::map<size_t, size_t>> mleftargs;

	bitsmeta() {}
	bitsmeta(size_t len) : types(len), vargs(len, 0), nterms{0}, args_bits{0} {
		for (size_t i = 0; i != len; ++i) vargs[i] = i; // native ordering
	}
	/* sort of a copy .ctor w/ bits changed (for one arg) - supports add_bit */
	bitsmeta(const bitsmeta& src, size_t arg, size_t bits2add = 1)
		: bitsmeta(src.types.size()) {
		DBG(assert(src.types.size() > 0);); // don't call this if empty

		//bm.set_args(ints(src.types.size()), src.types, src.nums);
		types = src.types;
		nums = src.nums; // we don't need this, or increase ++nums[arg] too?
		vargs = src.vargs;
		++nterms; // set init 'flag'

		//bm.init(dict);
		// we allow only one bit add at the time (for the moment)
		DBG(assert(bits2add == 1););
		size_t lsum = 0, args = types.size(), maxb = 0;
		mleftbits.clear();
		mleftbits[vargs[0]] = lsum;
		++types[arg].bitness; // increase bits...
		// TODO: check if this makes sense (e.g. if it's CHR it has to be 8)
		// recalculate everything...
		for (size_t i = 0; i < args-1; ++i) { // process [0..args-2] (skip last)
			lsum += types[vargs[i]].bitness;
			mleftbits[vargs[i+1]] = lsum;
			maxb = std::max(maxb, types[vargs[i]].bitness);
		}
		args_bits = mleftbits.at(vargs[args-1]) + types[vargs[args-1]].bitness;
		maxbits = std::max(maxb, types[vargs[args-1]].bitness);

		size_t argsum = 0;
		if (maxbits == 0)
			return;
		mleftargs.clear();
		for (int_t bit = maxbits - 1; bit >= 0; --bit) {
			std::map<size_t, size_t>& mpos = mleftargs[bit];
			for (size_t arg = 0; arg != types.size(); ++arg)
				if (types[vargs[arg]].bitness > size_t(bit))
					mpos[vargs[arg]] = argsum++;
		}
		DBG(assert(argsum == args_bits););
	}

	int_t get_chars(size_t arg) const // TODO: 256 ? 
	{
		return types[arg].type == base_type::CHR ? 255 : 0;
	}
	int_t get_syms(size_t arg, size_t nsyms = 0) const // D: Q: syms per arg?
	{
		return types[arg].type == base_type::STR ? nsyms : 0;
	}
	size_t get_args() const { return types.size(); }
	size_t sum_bits() const {
		size_t args = types.size();
		return mleftbits.at(vargs[args - 1]) + types[vargs[args - 1]].bitness;
	}
	size_t get_bits(size_t arg) const { return types[arg].bitness; }
	base_type get_type(size_t arg) const { return types[arg].type; }

	void init(const dict_t& dict);
	void update_types(const argtypes& vtypes, const ints& vnums);
	bool set_args(const ints& args, const argtypes& vtypes, const ints& vnums);
	// BSR op equivalent
	inline static size_t BitScanR(int_t num, size_t min_bits = 0) {
		// D: could num be < 0 (in the future?)
		DBG(assert(num >= 0););
		// D: what about just '0'? bits is 0 too? also 0 vs 'null'?
		if (num == 0) num = 1; // just to force one bit at least. // n=!n?1:n;
		if (num < 1 << min_bits) return min_bits; // nums = max(nums, num);
		size_t bits;
		for (bits = 0; num > 0; num >>= 1) ++bits;
		return bits;
	}

	/*
	This is the ordered variable-bits pos
	(it's static atm but we should make this part of alt or table)
	- vbits - contains variable 'bits' # for each of the arg (needed to calc.)
	- vargs - vector of arg-s ordered (has to contain all args, no duplicates)
	*/
	size_t pos(size_t bit, size_t arg, size_t args) const { //, size_t lsum = 0
		const std::map<size_t, size_t>& mpos = mleftargs.at(bit);
		return mpos.at(arg); // vargs[arg]

		DBG(assert(args == types.size()););
		DBG(assert(bit < types[arg].bitness && arg < args););
		size_t bits = types[arg].bitness;
		DBG(assert(bit < bits && arg < args););
		// lsum - sum of all the bits 'on the left' (same for all bit-s)
		size_t lsum = mleftbits.at(arg);
		// doubled consts fix: reverse bits, ie make it from-left (as consts)
		return lsum + bit;
		//return lsum + (bits - bit - 1); // bit; // 
	}

	//DBG(assert(b < bits););
	// doubled consts fix: make it again from-left (consts the same as pos)
	/* this is bit mapping for consts basically */
	//static inline size_t bit(size_t b, size_t bits) { UNUSED(bits); return b; }
	static inline size_t bit(size_t b, size_t bits) { return (bits - b - 1); }

	/*
	- bit - bit from the right
	- arg - argument order #
	- args - # of arguments
	- val - const value to bit compare
	- lsum - temp cached (to be deprecated)
	*/
	inline spbdd_handle from_bit(size_t bit, size_t arg, size_t args,
		int_t val) const { // , size_t lsum = 0
		return ::from_bit(pos(bit, arg, args), val & (1 << bit));
	}

	inline spbdd_handle from_bit_re(size_t b, size_t bits, size_t arg, 
		size_t args, int_t val) const {
		//size_t b = bits - b - 1;
		return ::from_bit(pos(b, arg, args), val & (1 << bit(b,bits)));
		//return ::from_bit(pos(bit, arg, args), val & (1 << b));
	}

	/*
	This is the ordered variable-bits pos
	(it's static atm but we should make this part of alt or table)
	- vbits - contains variable 'bits' # for each of the arg (needed to calc.)
	- vargs - vector of arg-s ordered (has to contain all args, no duplicates)
	- lsum - sum of all the bits 'on the left' (to optimize, same for all bit-s)
	*/
	static size_t pos(size_t bit, size_t arg, size_t args, const bitsmeta& bm) {
		return bm.pos(bit, arg, args);
	}

	// inline not worth much anyways
	inline static spbdd_handle from_bit(size_t bit, size_t arg, size_t args,
		int_t val, const bitsmeta& bm) { // , size_t lsum = 0
		return ::from_bit(bm.pos(bit, arg, args), val & (1 << bit));
	}

	//inline static size_t pos(
	//	// for 'args-first', we'd need to iter bit by bit, count arg-s that have
	//	// that bit etc., it's not worth it, I'd suggest the above (bits-first).
	//	//size_t argsum = 0;
	//	//for (size_t b = 0; b != bit; ++b) { // bit < varbits[arg]
	//	//	size_t nargs = 0;
	//	//	// order and optimize this (if worth it)
	//	//	for (size_t i = 0; i != args; ++i) if (varbits[i] > b) ++nargs;
	//	//	argsum += nargs;
	//	//}
	//	//return argsum + arg;
	//}
};

typedef const bitsmeta& cr_bitsmeta;
typedef const bitsmeta c_bitsmeta;

struct perminfo {
	bitsmeta bm;
	uints perm;
};

struct bits_perm {
	//bitsmeta bm;
	ntable tab;
	size_t arg;
	size_t args;
	perminfo perm;
};

#endif // __BITSMETA_H__