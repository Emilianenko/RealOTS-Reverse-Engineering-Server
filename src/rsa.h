#pragma once

#include <gmp.h>

class TRSA
{
public:
	TRSA();
	~TRSA();

	void setKey(const char* pString, const char* qString);
	void decrypt(char* msg) const;
private:
	//use only GMP
	mpz_t n, d;
};

extern TRSA RSA;