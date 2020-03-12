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
#include "types.h"
#include "err.h"
using namespace std;

tbl_arg::tbl_arg(const alt_arg& aa) : tab(aa.tab), arg(aa.arg) {
	DBG(assert(aa.alt == -1););
}

wostream& operator<<(wostream& os, const alt_arg& arg) {
	if (arg.alt == -1)
		return os << L"(" << arg.tab << L"," << arg.arg << L")"; // << endl;
	return os << L"(" << arg.tab << L"," << arg.alt << L"," << arg.arg << L")"; 
}
wostream& operator<<(wostream& os, const tbl_arg& arg) {
	return os << L"(" << arg.tab << L"," << arg.arg << L")"; // << endl;
}

