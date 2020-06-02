#include "pch.h"

#include "rsa.h"

TRSA::TRSA()
{
	mpz_init(n);
	mpz_init2(d, 1024);
}

TRSA::~TRSA()
{
	mpz_clear(n);
	mpz_clear(d);
}

void TRSA::setKey(const char* pString, const char* qString)
{
	mpz_t p, q, e;
	mpz_init2(p, 1024);
	mpz_init2(q, 1024);
	mpz_init(e);

	mpz_set_str(p, pString, 10);
	mpz_set_str(q, qString, 10);

	// e = 65537
	mpz_set_ui(e, 65537);

	// n = p * q
	mpz_mul(n, p, q);

	mpz_t p_1, q_1, pq_1;
	mpz_init2(p_1, 1024);
	mpz_init2(q_1, 1024);
	mpz_init2(pq_1, 1024);

	mpz_sub_ui(p_1, p, 1);
	mpz_sub_ui(q_1, q, 1);

	// pq_1 = (p -1)(q - 1)
	mpz_mul(pq_1, p_1, q_1);

	// d = e^-1 mod (p - 1)(q - 1)
	mpz_invert(d, e, pq_1);

	mpz_clear(p_1);
	mpz_clear(q_1);
	mpz_clear(pq_1);

	mpz_clear(p);
	mpz_clear(q);
	mpz_clear(e);
}

void TRSA::decrypt(char* msg) const
{
	mpz_t c, m;
	mpz_init2(c, 1024);
	mpz_init2(m, 1024);

	mpz_import(c, 128, 1, 1, 0, 0, msg);

	// m = c^d mod n
	mpz_powm(m, c, d, n);

	uint32_t amount = (mpz_sizeinbase(m, 2) + 7) / 8;
	memset(msg, 0, 128 - amount);
	mpz_export(msg + (128 - amount), nullptr, 1, 1, 0, 0, m);

	mpz_clear(c);
	mpz_clear(m);
}
