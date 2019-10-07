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
#include "tables.h"
#include "dict.h"
#include "input.h"
#include "output.h"
using namespace std;

#define mkchr(x) ((((int_t)x)<<2)|1)
#define mknum(x) ((((int_t)x)<<2)|2)

size_t sig_len(const sig& s) {
	size_t r = 0;
	for (int_t x : get<ints>(s)) if (x > 0) r += x;
	return r;
}

void unquote(wstring& str) {
	for (size_t i = 0; i != str.size(); ++i)
		if (str[i] == L'\\') str.erase(str.begin() + i);
}

wstring _unquote(wstring str) { unquote(str); return str; }

#ifdef DEBUG
vbools tables::allsat(spbdd_handle x, size_t args) const {
//	const size_t args = siglens[tab];
	vbools v = ::allsat(x, bits * args), s;
	for (bools b : v) {
		s.emplace_back(bits * args);
		for (size_t n = 0; n != bits; ++n)
			for (size_t k = 0; k != args; ++k)
				s.back()[(k+1)*bits-n-1] = b[pos(n, k, args)];
	}
	return s;
}
#endif

spbdd_handle tables::leq_const(int_t c, size_t arg, size_t args, size_t bit)
	const {
	if (!--bit)
		return	(c & 1) ? bdd_handle::T :
			::from_bit(pos(0, arg, args), false);
	return (c & (1 << bit)) ?
		bdd_ite_var(pos(bit, arg, args), leq_const(c, arg, args, bit),
			bdd_handle::T) :
		bdd_ite_var(pos(bit, arg, args), bdd_handle::F,
			leq_const(c, arg, args, bit));
}

typedef tuple<size_t, size_t, size_t, int_t> skmemo;
typedef tuple<size_t, size_t, size_t, int_t> ekmemo;
map<skmemo, spbdd_handle> smemo;
map<ekmemo, spbdd_handle> ememo;
map<ekmemo, spbdd_handle> leqmemo;

spbdd_handle tables::leq_var(size_t arg1, size_t arg2, size_t args) const {
	static ekmemo x;
	static map<ekmemo, spbdd_handle>::const_iterator it;
	if ((it = leqmemo.find(x = { arg1, arg2, args, bits })) != leqmemo.end())
		return it->second;
	spbdd_handle r = leq_var(arg1, arg2, args, bits);
	return leqmemo.emplace(x, r), r;
}
spbdd_handle tables::leq_var(size_t arg1, size_t arg2, size_t args, size_t bit)
	const {
	if (!--bit)
		return	bdd_ite(::from_bit(pos(0, arg2, args), true),
				bdd_handle::T,
				::from_bit(pos(0, arg1, args), false));
	return	bdd_ite(::from_bit(pos(bit, arg2, args), true),
			bdd_ite_var(pos(bit, arg1, args),
				leq_var(arg1, arg2, args, bit), bdd_handle::T),
			bdd_ite_var(pos(bit, arg1, args), bdd_handle::F,
				leq_var(arg1, arg2, args, bit)));
}

void tables::range(size_t arg, size_t args, bdd_handles& v) {
	spbdd_handle	ischar= ::from_bit(pos(0, arg, args), true) &&
			::from_bit(pos(1, arg, args), false);
	spbdd_handle	isnum = ::from_bit(pos(0, arg, args), false) &&
			::from_bit(pos(1, arg, args), true);
	spbdd_handle	issym = ::from_bit(pos(0, arg, args), false) &&
			::from_bit(pos(1, arg, args), false);
	// nums is set to max NUM, not universe size. While for syms it's the size.
	// It worked before because for arity==1 fact(nums) is always negated.
	bdd_handles r = {ischar || isnum || issym,
		(!chars	? bdd_handle::T%ischar : bdd_impl(ischar,
			leq_const(mkchr(chars-1), arg, args, bits))),
		(!nums 	? bdd_handle::T%isnum : bdd_impl(isnum,
			leq_const(mknum(nums), arg, args, bits))),
		(!syms 	? bdd_handle::T%issym : bdd_impl(issym,
			leq_const(((syms-1)<<2), arg, args, bits)))};
	v.insert(v.end(), r.begin(), r.end());
}

spbdd_handle tables::range(size_t arg, ntable tab) {
	array<int_t, 6> k = { syms, nums, chars, (int_t)tab, (int_t)arg,
		(int_t)bits };
	auto it = range_memo.find(k);
	if (it != range_memo.end()) return it->second;
	bdd_handles v;
	return	range(arg, tbls[tab].len, v),
		range_memo[k] = bdd_and_many(move(v));
}

uints perm_init(size_t n) {
	uints p(n);
	while (n--) p[n] = n;
	return p;
}

spbdd_handle tables::add_bit(spbdd_handle x, size_t args) {
	uints perm = perm_init(args * bits);
	for (size_t n = 0; n != args; ++n)
		for (size_t k = 0; k != bits; ++k)
			perm[pos(k, n, args)] = pos(k+1, bits+1, n, args);
	bdd_handles v = { x ^ perm };
	for (size_t n = 0; n != args; ++n)
		v.push_back(::from_bit(pos(0, bits + 1, n, args), false));
	return bdd_and_many(move(v));
}

void tables::add_bit() {
	range_clear_memo();
	spbdd_handle x = bdd_handle::F;
	bdd_handles v;
	for (auto& x : tbls)
//	for (size_t n = 0; n != ts.size(); ++n)
//		x.second.t = add_bit(x.second.t, x.second.len);
		x.t = add_bit(x.t, x.len);
	++bits;
}

//typedef tuple<size_t, size_t, size_t, int_t> skmemo;
//typedef tuple<size_t, size_t, size_t, int_t> ekmemo;
//map<skmemo, spbdd_handle> smemo;
//map<ekmemo, spbdd_handle> ememo;
//map<ekmemo, spbdd_handle> leqmemo;

spbdd_handle tables::from_sym(size_t pos, size_t args, int_t i) const {
	static skmemo x;
	static map<skmemo, spbdd_handle>::const_iterator it;
	if ((it = smemo.find(x = { i, pos, args, bits })) != smemo.end())
		return it->second;
	spbdd_handle r = bdd_handle::T;
	for (size_t b = 0; b != bits; ++b) r = r && from_bit(b, pos, args, i);
	return smemo.emplace(x, r), r;
}

spbdd_handle tables::from_sym_eq(size_t p1, size_t p2, size_t args) const {
	static ekmemo x;
	// a typo should be ekmemo, all the same at the moment
	//static map<skmemo, spbdd_handle>::const_iterator it;
	static map<ekmemo, spbdd_handle>::const_iterator it;
	if ((it = ememo.find(x = { p1, p2, args, bits })) != ememo.end())
		return it->second;
	spbdd_handle r = bdd_handle::T;
	for (size_t b = 0; b != bits; ++b)
		r = r && ::from_eq(pos(b, p1, args), pos(b, p2, args));
	return ememo.emplace(x, r), r;
}

/*spbdd_handle tables::from_ground(const term& t) {
	spbdd_handle r = bdd_handle::T;
	for (size_t n = 0, args = t.size(); n != args; ++n)
		r = r && from_sym(n, args, t[n]);
	return r;
}*/

spbdd_handle tables::from_fact(const term& t) {
	// TODO: memoize
	spbdd_handle r = bdd_handle::T;
	static varmap vs;
	vs.clear();
	auto it = vs.end();
	for (size_t n = 0, args = t.size(); n != args; ++n)
		if (t[n] >= 0)
			r = r && from_sym(n, args, t[n]);
		else if (vs.end() == (it = vs.find(t[n]))) {
			vs.emplace(t[n], n);
			if (!t.neg) r = r && range(n, t.tab);
		} else r = r && from_sym_eq(n, it->second, args);
	return r;
}

sig tables::get_sig(const raw_term&t) {return{dict.get_rel(t.e[0].e),t.arity};}
sig tables::get_sig(const lexeme& rel, const ints& arity) {
	return { dict.get_rel(rel), arity };
}

term tables::from_raw_term(const raw_term& r) {
	ints t;
	lexeme l;
	// skip the first symbol unless it's EQ/NEQ (which has VAR as it's first)
	for (size_t n = (r.iseq || r.isleq) ? 0 : 1; n < r.e.size(); ++n)
		switch (r.e[n].type) {
			case elem::NUM: t.push_back(mknum(r.e[n].num)); break;
			case elem::CHR: t.push_back(mkchr(r.e[n].ch)); break;
			case elem::VAR:
				t.push_back(dict.get_var(r.e[n].e)); break;
			case elem::STR:
				l = r.e[n].e;
				++l[0], --l[1];
				t.push_back(dict.get_sym(dict.get_lexeme(
					_unquote(lexeme2str(l)))));
				break;
			case elem::SYM: t.push_back(dict.get_sym(r.e[n].e));
			default: ;
		}
	// ints t is elems (VAR, consts) mapped to unique ints/ids for perms. 
	auto tbl = (r.iseq || r.isleq) ? -1 : get_table(get_sig(r));
	return term(r.neg, r.iseq, r.isleq, tbl, t);
}

void tables::out(wostream& os) const {
	for (ntable tab = 0; (size_t)tab != tbls.size(); ++tab)
		out(os, tbls[tab].t, tab);
}

void tables::out(const rt_printer& f) const {
	for (ntable tab = 0; (size_t)tab != tbls.size(); ++tab)
		out(tbls[tab].t, tab, f);
}

void tables::out(wostream& os, spbdd_handle x, ntable tab) const {
	out(x, tab, [&os](const raw_term& rt) { os << rt << L'.' << endl; });
}

void tables::decompress(spbdd_handle x, ntable tab, const cb_decompress& f,
	size_t len) const {
	if (!len) len = tbls.at(tab).len;
	allsat_cb(x/*&&ts[tab].t*/, len * bits,
		[tab, &f, len, this](const bools& p, int_t DBG(y)) {
		DBG(assert(abs(y) == 1);)
		term r(false, false, false, tab, ints(len, 0));
		for (size_t n = 0; n != len; ++n)
			for (size_t k = 0; k != bits; ++k)
				if (p[pos(k, n, len)])
					r[n] |= 1 << k;
		f(r);
	})();
}

set<term> tables::decompress() {
	set<term> r;
//	for (const auto& x : tbls)
	for (ntable tab = 0; (size_t)tab != tbls.size(); ++tab)
		decompress(tbls[tab].t, tab, [&r](const term& t) {
		//decompress(x.second.t, x.first, [&r](const term& t) {
			r.insert(t);});
	return r;
}

#define get_var_lexeme(v) dict.get_lexeme(wstring(L"?v") + to_wstring(-v))

raw_term tables::to_raw_term(const term& r) const {
	raw_term rt;
	rt.neg = r.neg;
	const size_t args = tbls.at(r.tab).len;
	DBG(assert(args == r.size());)
	rt.e.resize(args + 1),
	rt.e[0] = elem(elem::SYM, dict.get_rel(get<0>(tbls.at(r.tab).s)));
	for (size_t n = 1; n != args + 1; ++n) {
		const int_t arg = r[n - 1];
		if (arg < 0) rt.e[n] = elem(elem::VAR, get_var_lexeme(arg));
		else if (arg & 1) rt.e[n]=elem((wchar_t)(arg>>2));
		else if (arg & 2) rt.e[n]=elem((int_t)(arg>>2));
		else rt.e[n]=elem(elem::SYM, dict.get_sym(arg));
	}
	return	rt.arity = get<ints>(tbls.at(r.tab).s),
			rt.insert_parens(dict.op, dict.cl), rt;
}

void tables::out(spbdd_handle x, ntable tab, const rt_printer& f) const {
	decompress(x&&tbls.at(tab).t, tab, [f, this](const term& r) {
		f(to_raw_term(r));
	});
}

void term::replace(const map<int_t, int_t>& m) {
	auto it = m.end();
	for (int_t& i : *this) if (m.end() != (it = m.find(i))) i = it->second;
}

void tables::align_vars(vector<term>& v) const {
	set<int_t> vs;
	for (const term& t : v) for (int_t i : t) if (i < 0) vs.insert(i);
	if (vs.empty()) return;
	vs.clear();
	map<int_t, int_t> m;
	for (size_t k = 0; k != v.size(); ++k)
		for (size_t n = 0; n != v[k].size(); ++n)
			if (v[k][n] < 0 && !has(m, v[k][n]))
				m.emplace(v[k][n], -m.size() - 1);
	for (term& t : v) t.replace(m);
}

uints tables::get_perm(const term& t, const varmap& m, size_t len) const {
	uints perm = perm_init(t.size() * bits);
	for (size_t n = 0, b; n != t.size(); ++n)
		if (t[n] < 0)
			for (b = 0; b != bits; ++b)
				perm[pos(b,n,t.size())] = pos(b,m.at(t[n]),len);
	return perm;
}

template<typename T>
varmap tables::get_varmap(const term& h, const T& b, size_t &varslen) {
	varmap m;
	varslen = h.size();
	for (size_t n = 0; n != h.size(); ++n)
		if (h[n] < 0 && !has(m, h[n])) m.emplace(h[n], n);
	for (const term& t : b)
		for (size_t n = 0; n != t.size(); ++n)
			if (t[n] < 0 && !has(m, t[n]))
				m.emplace(t[n], varslen++);
	return m;
}

spbdd_handle tables::get_alt_range(const term& h, const set<term>& a,
	const varmap& vm, size_t len) {
	set<int_t> pvars, nvars, eqvars, leqvars;
	std::vector<const term*> eqterms, leqterms;
	// first pass, just enlist eq terms (that have at least one var)
	for (const term& t : a) {
		bool haseq = false, hasleq = false;
		for (size_t n = 0; n != t.size(); ++n) {
			if (t[n] < 0) {
				if (t.iseq) haseq = true;
				else if (t.isleq) hasleq = true;
				else (t.neg ? nvars : pvars).insert(t[n]);
			}
		}
		// only if iseq and has at least one var
		if (haseq) eqterms.push_back(&t);
		else if (hasleq) leqterms.push_back(&t);
	}
	for (const term* pt : eqterms) {
		const term& t = *pt;
		bool noeqvars = true;
		std::vector<int_t> tvars;
		for (size_t n = 0; n != t.size(); ++n)
			if (t[n] < 0) {
				// nvars add range already, so skip all in that case...
				// and per 1.3 - if any one is contrained (outside) bail out
				if (has(nvars, t[n])) { noeqvars = false; break; }
				// if neither pvars has this var it should be ranged
				if (!has(pvars, t[n])) tvars.push_back(t[n]);
				else if (!t.neg) { noeqvars = false; break; }
				// if is in pvars and == then other var is covered too, skip.
				// this isn't covered by 1.1-3 (?) but further optimization.
			}
		if (!noeqvars) continue;
		for (const int_t tvar : tvars) {
			eqvars.insert(tvar);
			// 1.3 one is enough (we have one constrained, no need to do both).
			// but this doesn't work well, we need to range all that fit.
			//break;
		}
	}
	for (const term* pt : leqterms) {
		// - for '>' (~(<=)) it's enough if 2nd var is in nvars/pvars.
		// - for '<=' it's enough if 2nd var is in nvars/pvars.
		// - if 1st/greater is const, still can't skip, needs to be ranged.
		// - if neither var appears elsewhere (nvars nor pvars) => do both.
		//   (that is a bit strange, i.e. if appears outside one is enough)
		// ?x > ?y => ~(?x <= ?y) => ?y - 2nd var is limit for both LEQ and GT.
		const term& t = *pt;
		assert(t.size() == 2);
		int_t v1 = t[0], v2 = t[1];
		if (v1 == v2) { if (!has(nvars, v2)) leqvars.insert(v2); continue; }
		if (v2 < 0) {
			if (has(nvars, v2) || has(pvars, v2)) continue; // skip both
			leqvars.insert(v2); // add and continue to 1st
		}
		if (v1 < 0 && !has(nvars, v1) && !has(pvars, v1))
			leqvars.insert(v1);
	}

	for (int_t i : pvars) nvars.erase(i);
	if (h.neg) for (int_t i : h) if (i < 0) { 
		nvars.erase(i); 
		eqvars.erase(i);
		leqvars.erase(i);
	}
	bdd_handles v;
	for (int_t i : nvars) range(vm.at(i), len, v); 
	for (int_t i : eqvars) range(vm.at(i), len, v);
	for (int_t i : leqvars) range(vm.at(i), len, v);
	if (!h.neg) {
		set<int_t> hvars;
		for (int_t i : h) if (i < 0) hvars.insert(i);
		for (const term& t : a) for (int_t i : t) hvars.erase(i);
		for (int_t i : hvars) range(vm.at(i), len, v);
	}
	return bdd_and_many(v);
}

map<size_t, int_t> varmap_inv(const varmap& vm) {
	map<size_t, int_t> inv;
	for (auto x : vm) {
		assert(!has(inv, x.second));
		inv.emplace(x.second, x.first);
	}
	return inv;
}

body tables::get_body(const term& t, const varmap& vm, size_t len) const {
	body b;
	b.neg = t.neg, b.tab = t.tab, b.perm = get_perm(t, vm, len),
	b.q = bdd_handle::T, b.ex = bools(t.size() * bits, false);
	varmap m;
	auto it = m.end();
	for (size_t n = 0; n != t.size(); ++n)
		if (t[n] >= 0)
			b.q = b.q && from_sym(n, t.size(), t[n]),
			get_var_ex(n, t.size(), b.ex);
		else if (m.end() == (it = m.find(t[n]))) m.emplace(t[n], n);
		else	b.q = b.q && from_sym_eq(n, it->second, t.size()),
			get_var_ex(n, t.size(), b.ex);
	return b;
}

void tables::get_facts(const flat_prog& m) {
	map<ntable, set<spbdd_handle>> f;
	for (const auto& r : m)
		if (r.size() != 1) continue;
		else if (r[0].goal) goals.insert(r[0]);
		else f[r[0].tab].insert(from_fact(r[0]));
	clock_t start, end;
	measure_time_start();
	bdd_handles v;
	for (auto x : f) {
		spbdd_handle r = bdd_handle::F;
		for (auto y : x.second) r = r || y;
		tbls[x.first].t = r;
	}
	measure_time_end();
}

/*bool tables::cqc(const set<rule>& rs, const rule& r, size_t a, tables& tb)const{
	int_t m = 0;
	for (int_t i : r.t) m = max(m, i);
	for (int_t i : r.r[a]) m = max(m, i);
	bool bsym = false, bnum = false, bchr = false;

}

bool tables::cqc(const set<rule>& rs, rule& r) {
	tables tb(false);
	tb.bits = bits, tb.ts = ts, tb.smap = smap;
	for (const rule& x : rs)
		if (!x.equals_termwise(r))
			tb.rules.push_back(x);
	for (size_t n = 0; n < r.size();)
		if (cqc(rs, r, n, tb)) r.erase(n);
		else ++n;
}*/

void tables::get_nums(const raw_term& t) {
	for (const elem& e : t.e)
		if (e.type == elem::NUM)
			nums = max(nums, e.num);
}

flat_prog tables::to_terms(const raw_prog& p) {
	flat_prog m;
	vector<term> v;
	term t;
	for (const raw_rule& r : p.r)
		if (r.type == raw_rule::NONE && !r.b.empty())
			for (const raw_term& x : r.h) {
				get_nums(x), t = from_raw_term(x),
				v.push_back(t);
				for (const vector<raw_term>& y : r.b) {
					for (const raw_term& z : y)
						v.push_back(from_raw_term(z)),
						get_nums(z);
					align_vars(v), m.insert(move(v));
				}
			}
		else for (const raw_term& x : r.h)
			t = from_raw_term(x), t.goal = r.type == raw_rule::GOAL,
			//m[t] = {},
			m.insert({t}), get_nums(x);
	return m;
}

template<typename T> void set2vec(const set<T>& s, vector<T>& v) {
	copy(s.begin(), s.end(), v.begin());
}

vector<term> to_vec(const term& h, const set<term>& b) {
	vector<term> v;
	v.reserve(b.size() + 1), v.emplace_back(h);
	for (const term& t : b) v.emplace_back(t);
	return v;
}

template<typename T> set<T> vec2set(const vector<T>& v, size_t n) {
	set<T> r;
	r.insert(v.begin() + n, v.end());
	return r;
}

void freeze(vector<term>& v) {
	int_t m = 0;
	map<int_t, int_t> p;
	map<int_t, int_t>::const_iterator it;
	for (const term& t : v) for (int_t i : t) if (i & 2) m = max(m, i >> 2);
	for (term& t : v)
		for (int_t& i : t)
			if (i >= 0) continue;
			else if ((it = p.find(i)) != p.end()) i = it->second;
			else p.emplace(i, mknum(m)), i = mknum(m++);
}

void freeze(term& t, env& e1, env& e2, int_t& m) {
	env::const_iterator it;
	for (int_t& i : t)
		if (i >= 0) continue;
		else if ((it = e1.find(i)) == e1.end())
			e1.emplace(i, m), e2.emplace(m, i), i = m++;
		else i = it->second;
}

env freeze(term& h, vector<term>& b) {
	env e1, e2;
	int_t m = 0;
	for (int_t i : h) m = max(m, i);
	for (const term& t : b) for (int_t i : t) m = max(m, i);
	freeze(h, e1, e2, ++m);
	for (term& t : b) freeze(t, e1, e2, m);
	return e2;
}

bool tables::cqc(const vector<term>& x, vector<term> y, bool tmp) const {
	if (tmp) y[0].tab = x[0].tab;
	set<term> r;
	flat_prog m;
//	map<term, set<set<term>>> m;
	for (const term& t : x)
		if(t.neg) return false;
		//throw "cqc not supported yet for terms with negation";
	for (const term& t : y)
		if(t.neg) return false;
		//throw "cqc not supported yet for terms with negation";
	m.insert(x);
//	m[x[0]].insert(vec2set(x, 1));
	freeze(y);
	for (size_t n = 1; n != y.size(); ++n) m.insert({y[n]});
	tables t(false, false, true);
	t.dict = dict, t.bcqc = false, t.run_nums(move(m), r, 1);
	//DBG(print(wcout, r) << endl;)
	return has(r, y[0]);
}

bool tables::cqc(const vector<term>& v, const flat_prog& m, bool tmp) const {
	for (const vector<term>& x : m) if (cqc(x, v, tmp)) return true;
	return false;
}

void tables::cqc_minimize(vector<term>& v) const {
	if (v.size() < 2) return;
	const vector<term> v1 = v;
	term t;
	for (size_t n = 1; n != v.size(); ++n) {
		t = move(v[n]), v.erase(v.begin() + n);
		if (!cqc(v1, v, false)) v.insert(v.begin() + n, t);
	}
	DBG(if (v.size() != v1.size())
		print(print(wcerr<<L"Rule\t\t", v)<<endl<<L"minimized into\t"
		, v1)<<endl;)
}

ntable tables::prog_add_rule(flat_prog& p, vector<term> x) {
//	set<term> b(x.begin() + 1, x.end());
	if (bcqc && has(tmps, x[0][0])) {
		for (const vector<term>& y : p)
			if (has(tmps, y[0].tab))
				if (cqc(x, y, true) && cqc(y, x, true))
					return y[0].tab;
		return x[0].tab;
	}
	if (!bcqc || x.size() == 1) return p.emplace(x), x[0].tab;
	if (x.size() > 3) cqc_minimize(x);
	if (!cqc(x, p, false)) p.emplace(x);
	return x[0].tab;
}

/*wostream& tables::print(wostream& os, const vector<term>& b) const {
	for (const term& t : b) os << to_raw_term(t) << L'.' << endl;
	return os;
}

wostream& tables::print(wostream& os, const set<term>& b) const {
	for (const term& t : b) os << to_raw_term(t) << L'.' << endl;
	return os;
}*/

wostream& tables::print(wostream& os, const vector<term>& v) const {
	os << to_raw_term(v[0]);
	if (v.size() == 1) return os << L'.';
	os << L" :- ";
	for (size_t n = 1; n != v.size(); ++n) {
		if (v[n].goal) os << L'!';
		os << to_raw_term(v[n]) << (n == v.size() - 1 ? L"." : L", ");
	}
	return os;
}

wostream& tables::print(wostream& os, const flat_prog& p) const {
	for (const auto& x : p) print(os, x) << endl;
	return os;
}

void tables::get_rules(flat_prog p) {
	bcqc = false;
	get_facts(p);
	for (const vector<term>& x : p)
		for (size_t n = 1; n != x.size(); ++n)
			exts.insert(x[n].tab);
	for (const vector<term>& x : p) if (x.size() > 1) exts.erase(x[0].tab);
#ifndef TRANSFORM_BIN_DRIVER
	if (bin_transform) transform_bin(p);
#endif
	set<rule> rs;
	varmap::const_iterator it;
	set<body*, ptrcmp<body>>::const_iterator bit;
	set<alt*, ptrcmp<alt>>::const_iterator ait;
	body* y;
	alt* aa;
	flat_prog q(move(p));
	for (const auto& x : q) prog_add_rule(p, x);
	if (optimize) bdd::gc();
//	print(wcout, m);
	map<term, set<set<term>>> m;
	for (const auto& x : p)
		if (x.size() == 1) m[x[0]] = {};
		else m[x[0]].insert(set<term>(x.begin() + 1, x.end()));
	for (pair<term, set<set<term>>> x : m) {
		if (x.second.empty()) continue;
		varmap v;
		set<int_t> hvars;
		const term &t = x.first;
		rule r;
		if (t.neg) datalog = false;
		tbls[t.tab].ext = false;
		r.neg = t.neg, r.tab = t.tab, r.eq = bdd_handle::T, r.t = t;
		for (size_t n = 0; n != t.size(); ++n)
			if (t[n] >= 0) get_sym(t[n], n, t.size(), r.eq);
			else if (v.end() == (it = v.find(t[n])))
				v.emplace(t[n], n);
			else r.eq = r.eq&&from_sym_eq(n, it->second, t.size());
		set<alt> as;
		r.len = t.size();
		for (const set<term>& al : x.second) {
			alt a;
			set<int_t> vs;
			set<pair<body, term>> b;
			spbdd_handle leq = bdd_handle::T;
			a.vm = get_varmap(t, al, a.varslen),
			a.inv = varmap_inv(a.vm);
			for (const term& t : al) {
				if (t.iseq && t.size() == 2) {
					bool has0 = has(a.vm, t[0]);
					bool has1 = has(a.vm, t[1]);
					if (has0 && has1) {
						size_t arg0 = a.vm.at(t[0]), arg1 = a.vm.at(t[1]);
						if (t.neg)
							a.eq = a.eq % from_sym_eq(arg0, arg1, a.varslen);
						else
							a.eq = a.eq && from_sym_eq(arg0, arg1, a.varslen);
					}
					else if (has0) {
						size_t arg0 = a.vm.at(t[0]);
						if (t.neg)
							a.eq = a.eq % from_sym(arg0, a.varslen, t[1]);
						else
							a.eq = a.eq && from_sym(arg0, a.varslen, t[1]);
					}
					else if (has1) {
						size_t arg1 = a.vm.at(t[1]);
						if (t.neg)
							a.eq = a.eq % from_sym(arg1, a.varslen, t[0]);
						else
							a.eq = a.eq && from_sym(arg1, a.varslen, t[0]);
					}
					else { // just consts?
						auto tf = t[0] == t[1] ? bdd_handle::T : bdd_handle::F;
						if (t.neg) a.eq = a.eq % tf;
						else a.eq = a.eq && tf;
					}
				}
				else if (t.isleq && t.size() == 2) {
					bool has0 = has(a.vm, t[0]);
					bool has1 = has(a.vm, t[1]);
					if (has0 && has1) {
						size_t arg0 = a.vm.at(t[0]), arg1 = a.vm.at(t[1]);
						if (t.neg)
							leq = leq % leq_var(arg0, arg1, a.varslen, bits);
						else
							leq = leq && leq_var(arg0, arg1, a.varslen, bits);
					}
					else if (has0) {
						size_t arg0 = a.vm.at(t[0]);
						if (t.neg)
							leq = leq % leq_const(t[1], arg0, a.varslen, bits);
						else
							leq = leq && leq_const(t[1], arg0, a.varslen, bits);
					}
					else if (has1) {
						size_t arg1 = a.vm.at(t[1]);
						spbdd_handle geq = bdd_handle::T;
						// 1 <= v1, v1 >= 1, ~(v1 <= 1) || v1==1.
						geq = geq % leq_const(t[0], arg1, a.varslen, bits);
						geq = geq || from_sym(arg1, a.varslen, t[0]);
						if (t.neg)
							leq = leq % geq;
						else{
							leq = leq && geq;
						}
					}
					else { // TODO: just consts: how to do <= for consts?
						auto tf = t[0] <= t[1] ? bdd_handle::T : bdd_handle::F;
						if (t.neg) leq = leq % tf;
						else leq = leq && tf;
					}
				}
				else b.insert({
					get_body(t, a.vm, a.varslen), t});
			}
			a.rng = get_alt_range(t, al, a.vm, a.varslen);
			a.rng = bdd_and_many({ a.rng, leq });
			for (auto x : b) {
				a.t.push_back(x.second);
				if ((bit=bodies.find(&x.first))!=bodies.end())
					a.push_back(*bit);
				else	*(y = new body) = x.first,
					a.push_back(y), bodies.insert(y);
			}
			auto d = deltail(a.varslen, r.len);
			a.ex = d.first, a.perm = d.second;
			as.insert(a);
		}
		for (alt x : as)
			if ((ait = alts.find(&x)) != alts.end())
				r.push_back(*ait);
			else	*(aa = new alt) = x,
				r.push_back(aa), alts.insert(aa);
		rs.insert(r);
	}
	for (rule r : rs)
		tbls[r.t.tab].r.push_back(rules.size()), rules.push_back(r);
	sort(rules.begin(), rules.end(), [this](const rule& x, const rule& y) {
			return tbls[x.tab].priority > tbls[y.tab].priority; });
}

void tables::load_string(lexeme r, const wstring& s) {
	int_t rel = dict.get_rel(r);
	const ints ar = {0,-1,-1,1,-2,-2,-1,1,-2,-1,-1,1,-2,-2};
	const ntable t1 = get_table({rel, ar}), t2 = get_table({rel, {3}});
	const int_t sspace = dict.get_sym(dict.get_lexeme(L"space")),
		salpha = dict.get_sym(dict.get_lexeme(L"alpha")),
		salnum = dict.get_sym(dict.get_lexeme(L"alnum")),
		sdigit = dict.get_sym(dict.get_lexeme(L"digit")),
		sprint = dict.get_sym(dict.get_lexeme(L"printable"));
	term t;
	bdd_handles b1, b2;
	b1.reserve(s.size()), b2.reserve(s.size());
	t.resize(3), t.neg = false;
	for (int_t n = 0; n != (int_t)s.size(); ++n) {
		t[0] = mknum(n), t[1] = mkchr(s[n]), t[2] = mknum(n+1),
		b1.push_back(from_fact(t)), t[1] = t[0];
		if (iswspace(s[n])) t[0] = sspace, b2.push_back(from_fact(t));
		if (iswdigit(s[n])) t[0] = sdigit, b2.push_back(from_fact(t));
		if (iswalpha(s[n])) t[0] = salpha, b2.push_back(from_fact(t));
		if (iswalnum(s[n])) t[0] = salnum, b2.push_back(from_fact(t));
		if (iswprint(s[n])) t[0] = sprint, b2.push_back(from_fact(t));
	}
	clock_t start, end;
	if (optimize) output::to(L"debug")<<"load_string or_many: ";
	measure_time_start();
	tbls[t1].t = bdd_or_many(move(b1)), tbls[t2].t = bdd_or_many(move(b2));
	if (optimize) measure_time_end();
}

template<typename T> bool subset(const set<T>& small, const set<T>& big) {
	for (const T& t : small) if (!has(big, t)) return false;
	return true;
}

void tables::get_var_ex(size_t arg, size_t args, bools& b) const {
	for (size_t k = 0; k != bits; ++k) b[pos(k, arg, args)] = true;
}

void tables::get_sym(int_t sym, size_t arg, size_t args, spbdd_handle& r) const{
	for (size_t k = 0; k != bits; ++k) r = r && from_bit(k, arg, args, sym);
}

ntable tables::get_table(const sig& s, size_t priority) {
	auto it = smap.find(s);
	if (it != smap.end()) return it->second;
	ntable nt = tbls.size();
	size_t len = sig_len(s);
	max_args = max(max_args, len);
	table tb;
	return	tb.t = bdd_handle::F, tb.s = s, tb.len = len,
		tb.priority = priority, tbls.push_back(tb),
		smap.emplace(s,nt), nt;
}

term to_nums(term t) {
	for (int_t& i : t)  if (i > 0) i = mknum(i);
	return t;
}

//term from_nums(term t) {
//	for (int_t& i : t)  if (i > 0) i >>= 2;
//	return t;
//}

vector<term> to_nums(const vector<term>& v) {
	vector<term> r;
	for (const term& t : v) r.push_back(to_nums(t));
	return r;
}

//set<term> from_nums(const set<term>& s) {
//	set<term> ss;
//	for (const term& t : s) ss.insert(from_nums(t));
//	return ss;
//}

void to_nums(flat_prog& m) {
	flat_prog mm;
	for (auto x : m) mm.insert(to_nums(x));
	m = move(mm);
}

void tables::add_prog(const raw_prog& p, const strs_t& strs) {
	if (!strs.empty()) chars = 256;
	for (auto x : strs) nums = max(nums, (int_t)x.second.size()+1);
	add_prog(move(to_terms(p)), strs);
}

bool tables::run_nums(flat_prog m, set<term>& r, size_t nsteps) {
	map<ntable, ntable> m1, m2;
	auto f = [&m1, &m2](ntable *x) {
		auto it = m1.find(*x);
		if (it != m1.end()) return *x = it->second;
		const int_t y = (int_t)m2.size();
		m1.emplace(*x, y), m2.emplace(y, *x);
		return *x = y;
	};
	auto g = [&m2](const set<term>& s) {
		set<term> r;
		for (term t : s) {
			auto it = m2.find(t.tab);
			if (it == m2.end()) r.insert(t);
			else t.tab = it->second, r.insert(t);
		}
		return r;
	};
	auto h = [this, f](const set<term>& s) {
		set<term> r;
		for (term t : s)
			get_table({ f(&t.tab), {(int_t)t.size()}}), r.insert(t);
		return r;
	};
	flat_prog p;
	for (vector<term> x : m) {
		get_table({ f(&x[0].tab), { (int_t)x[0].size() } });
		auto s = h(set<term>(x.begin() + 1, x.end()));
		x.erase(x.begin() + 1, x.end()),
		x.insert(x.begin() + 1, s.begin(), s.end()), p.insert(x);
	}
//	DBG(print(wcout<<L"run_nums for:"<<endl, p)<<endl<<L"returned:"<<endl;)
	add_prog(move(p), {}, false);//true);
	if (!pfp(nsteps)) return false;
	r = g(decompress());
	return true;
}

ntable tables::get_new_tab(int_t x, ints ar, size_t priority) {
	return get_table({ x, ar }, priority);
}

void tables::add_prog(flat_prog m, const strs_t& strs, bool mknums) {
	if (mknums) to_nums(m);
	rules.clear(), datalog = true;
	syms = dict.nsyms();
	while (max(max(nums, chars), syms) >= (1 << (bits - 2))) add_bit();
	get_rules(move(m));
//	clock_t start, end;
//	output::to(L"debug")<<"load_string: ";
//	measure_time_start();
	for (auto x : strs) load_string(x.first, x.second);
//	measure_time_end();
	smemo.clear(), ememo.clear(), leqmemo.clear();
	if (optimize) bdd::gc();
}

pair<bools, uints> tables::deltail(size_t len1, size_t len2) const {
	bools ex(len1 * bits, false);
	uints perm = perm_init(len1 * bits);
	for (size_t n = 0; n != len1; ++n)
		for (size_t k = 0; k != bits; ++k)
			if (n >= len2) ex[pos(k, n, len1)] = true;
			else perm[pos(k, n, len1)] = pos(k, n, len2);
	return { ex, perm };
}

uints tables::addtail(size_t len1, size_t len2) const {
	uints perm = perm_init(len1 * bits);
	for (size_t n = 0; n != len1; ++n)
		for (size_t k = 0; k != bits; ++k)
			perm[pos(k, n, len1)] = pos(k, n, len2);
	return perm;
}

spbdd_handle tables::addtail(cr_spbdd_handle x, size_t len1, size_t len2) const{
	if (len1 == len2) return x;
	return x ^ addtail(len1, len2);
}

spbdd_handle tables::body_query(body& b, size_t /*DBG(len)*/) {
//	if (b.a) return alt_query(*b.a, 0);
//	if (b.ext) return b.q;
//	DBG(assert(bdd_nvars(b.q) <= b.ex.size());)
//	DBG(assert(bdd_nvars(get_table(b.tab, db)) <= b.ex.size());)
	if (b.tlast && b.tlast->b == tbls[b.tab].t->b) return b.rlast;
	b.tlast = tbls[b.tab].t;
	return b.rlast = (b.neg ? bdd_and_not_ex_perm : bdd_and_ex_perm)
		(b.q, tbls[b.tab].t, b.ex, b.perm);
//	DBG(assert(bdd_nvars(b.rlast) < len*bits);)
//	if (b.neg) b.rlast = bdd_and_not_ex_perm(b.q, ts[b.tab].t, b.ex,b.perm);
//	else b.rlast = bdd_and_ex_perm(b.q, ts[b.tab].t, b.ex, b.perm);
//	return b.rlast;
//	return b.rlast = bdd_permute_ex(b.neg ? b.q % ts[b.tab].t :
//			(b.q && ts[b.tab].t), b.ex, b.perm);
}

auto handle_cmp = [](const spbdd_handle& x, const spbdd_handle& y) {
	return x->b < y->b;
};

spbdd_handle tables::alt_query(alt& a, size_t /*DBG(len)*/) {
/*	spbdd_handle t = bdd_handle::T;
	for (auto x : a.order) {
		bdd_handles v1;
		v1.push_back(t);
		for (auto y : x.first) v1.push_back(body_query(*a[y]));
		t = bdd_and_many(move(v1)) / x.second;
	}
	v.push_back(a.rlast = deltail(t && a.rng, a.varslen, len));*/
//	DBG(bdd::gc();)
	bdd_handles v1 = { a.rng, a.eq };
	spbdd_handle x;
	//DBG(assert(!a.empty());)
	for (size_t n = 0; n != a.size(); ++n)
		if (bdd_handle::F == (x = body_query(*a[n], a.varslen))) {
			a.insert(a.begin(), a[n]), a.erase(a.begin() + n + 1);
			return bdd_handle::F;
		} else v1.push_back(x);
	sort(v1.begin(), v1.end(), handle_cmp);
	if (v1 == a.last) return a.rlast;// { v.push_back(a.rlast); return; }
	if (!bproof)
		return	a.rlast =
			bdd_and_many_ex_perm(a.last = move(v1), a.ex, a.perm);
	a.levels.emplace(nstep, x = bdd_and_many(v1));
//	if ((x = bdd_and_many_ex(a.last, a.ex)) != bdd_handle::F)
//		v.push_back(a.rlast = x ^ a.perm);
//	bdd_handles v;
	return a.rlast = bdd_permute_ex(x, a.ex, a.perm);
//	if ((x = bdd_and_many_ex_perm(a.last, a.ex, a.perm)) != bdd_handle::F)
//		v.push_back(a.rlast = x);
//	return x;
//	DBG(assert(bdd_nvars(a.rlast) < len*bits);)
}

bool table::commit(DBG(size_t /*bits*/)) {
	if (add.empty() && del.empty()) return false;
	spbdd_handle x;
	if (add.empty()) x = t % bdd_or_many(move(del));
	else if (del.empty()) add.push_back(t), x = bdd_or_many(move(add));
	else {
		spbdd_handle a = bdd_or_many(move(add)),
				 d = bdd_or_many(move(del)), s = a % d;
//		DBG(assert(bdd_nvars(a) < len*bits);)
//		DBG(assert(bdd_nvars(d) < len*bits);)
		if (s == bdd_handle::F) return unsat = true;
		x = (t || a) % d;
	}
//	DBG(assert(bdd_nvars(x) < len*bits);)
	return x != t && (t = x, true);
}

char tables::fwd() noexcept {
	bdd_handles add, del;
//	DBG(out(wcout<<"db before:"<<endl);)
	for (rule& r : rules) {
		bdd_handles v(r.size());
		for (size_t n = 0; n != r.size(); ++n)
			v[n] = alt_query(*r[n], r.len);
		spbdd_handle x;
		if (v == r.last) { if (datalog) continue; x = r.rlast; }
		// applying the r.eq and or-ing all alt-s
		else r.last = v, x = r.rlast = bdd_or_many(move(v)) && r.eq;
//		DBG(assert(bdd_nvars(x) < r.len*bits);)
		if (x == bdd_handle::F) continue;
		(r.neg ? tbls[r.tab].del : tbls[r.tab].add).push_back(x);
	}
	bool b = false;
	for (auto& t : tbls) {
		b |= t.commit(DBG(bits));
		if (t.unsat) return unsat = true;
		//b |= t.second.commit(DBG(bits));
		//if (t.second.unsat) return unsat = true;
	}
	return b;
/*	if (!b) return false;
	for (auto x : goals)
		for (auto y : x.second)
			b &= (y && ts[x.first].t) == y;
	if (b) return (wcout <<"found"<<endl), false;
	return b;*/
}

level tables::get_front() const {
	level r(tbls.size());
	for (ntable n = 0; n != (ntable)tbls.size(); ++n) r[n] = tbls.at(n).t;
	return r;
}

bool tables::pfp(size_t nsteps) {
	set<level> s;
	if (bproof) levels.emplace_back(get_front());
	level l;
	for (;;) {
		if (optimize) output::to(L"info") << "step: " << nstep << endl;
		++nstep;
		if (!fwd()) return bproof ? get_goals(), true : true;
		if (unsat) throw unsat_exception();
		if (nsteps && nstep == nsteps) return true;
		l = get_front();
		if (!datalog && !s.emplace(l).second) return false;
		if (bproof) levels.push_back(move(l));
	}
	throw 0;
}

bool tables::run_prog(const raw_prog& p, const strs_t& strs) {
	clock_t start, end;
	double t;
//	output::to(L"@stderr") << L"add_prog: ";
	if (optimize) measure_time_start();
	add_prog(p, strs);
	if (optimize) {
		end = clock(), t = double(end - start) / CLOCKS_PER_SEC;
		wcerr << L"pfp: ";
		measure_time_start();
	}
//	output::to(L"@stderr")
	bool r = pfp();
	if (optimize)
		(wcerr << L"add_prog: " << t << L" pfp: "),measure_time_end();
	return r;
}

tables::tables(bool bproof, bool optimize, bool bin_transform,
	bool print_transformed) : dict(*new dict_t), bproof(bproof),
	optimize(optimize), bin_transform(bin_transform),
	print_transformed(print_transformed) {}

tables::~tables() {
	if (optimize) delete &dict;
	while (!bodies.empty()) {
		body *b = *bodies.begin();
		bodies.erase(bodies.begin());
		delete b;
	}
	while (!alts.empty()) {
		alt *a = *alts.begin();
		alts.erase(alts.begin());
		delete a;
	}
}

//set<body*, ptrcmp<body>> body::s;
//set<alt*, ptrcmp<alt>> alt::s;
