/*
	Copyright 2009 Christopher A. Taylor

	This file is part of LibCat.

	LibCat is free software: you can redistribute it and/or modify
	it under the terms of the Lesser GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	LibCat is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	Lesser GNU General Public License for more details.

	You should have received a copy of the Lesser GNU General Public
	License along with LibCat.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
	Unit test for the Elliptic Curve Cryptography code

	Performs simulated runs of the Tunnel protocol, terminating on any error

	Demonstrates how to use the ECC code
*/

#include <cat/AllMath.hpp>
#include <iostream>
#include <map>
#include "SecureServerDemo.hpp"
#include "SecureClientDemo.hpp"
#include <cat/crypt/rand/Fortuna.hpp>
using namespace std;
using namespace cat;

static Clock *m_clock = 0;
static TunnelTLS *m_tls = 0;

// Generate candidate values for c for the ECC.cpp code
void GenerateCandidatePrimes();

// Generate table used for w-MOF in the ECC.cpp code
void GenerateMOFTable(int window_bits);

// Compare Barrett, Montgomery, and Special modulus functions for speed
void TimeModulusCode();

// ECC protocol test
void ECCTest();

// Full handshake test
void HandshakeTest();

// IV reconstruction test
void TestIVReconstruction();

void TestSkein256();
void TestSkein512();

void TestCurveParameterD();

void TestChaCha();
/*
void test1()
{
	u32 x = Degree32(0x7ffffff);
}

void test2()
{
	u32 x = SecondDegree32(0x7ffff);
}
*/

void GenerateWMOFTable()
{
	const int w = 8;

	const int W = 1 << w;

	cout << "{" << 0 << "," << 0 << "},";

	int print = 0;
	for (int kk = 0; kk < 2; ++kk)
	{
		for (int ii = 0; ii < W; ++ii)
		{
			int top = ii;
			int bot = (ii >> 1) | (kk << (w-1));
			int val = 0;

			for (int jj = 0; jj < w; ++jj)
			{
				val += (top & (1 << jj)) - (bot & (1 << jj));
			}

			int x = ii;

			if (kk)
			{
				x = (~ii & (W-1));
			}

			if (val)
			{
				int input = val - 1;

				int d = 0;
				while ((val & 1) == 0)
				{
					val >>= 1;
					++d;
				}

				int neg = val < 0;
				if (val < 0) val = -val;

				int index = (val + 1) / 2;

				if (neg)
				{
					//cout << "last=" << kk << ", x=" << x << " -> -" << index << "d" << d << endl;
				}
				else
				{
					if ((ii & 1) == 0)
					{
						cout << "{" << index << "," << d << "},";
						if (++print % 8 == 0) cout << endl;
					}
					//cout << "last=" << kk << ", in=" << input << " -> +" << index << "d" << d << endl;
				}
			}
			else
			{
				//cout << "last=" << kk << ", x=" << x << " -> zero" << endl;
			}
		}
	}
}

void AddTest(const Leg *in_a, const Leg *in_b, Leg *out)
{
	const int library_legs = 4;
	const Leg modulus_c = 189;
}

int TestDivide()
{
	BigRTL x(10, 256);
	Leg *a = x.Get(0);
	Leg *b = x.Get(1);
	Leg *q = x.Get(2);
	Leg *r = x.Get(3);
	Leg *p = x.Get(4);

	MersenneTwister mtprng;
	mtprng.Initialize();

	CAT_FOREVER
	{
		mtprng.Generate(a, x.RegBytes());
		mtprng.Generate(b, x.RegBytes());

		cout << hex << "a = ";
		for (int ii = x.Legs()-1; ii >= 0; --ii)
			cout << hex << a[ii] << " ";
		cout << endl;

		cout << hex << "b = ";
		for (int ii = x.Legs()-1; ii >= 0; --ii)
			cout << hex << b[ii] << " ";
		cout << endl;

		x.Divide(a, b, q, r);

		cout << hex << "a' = ";
		for (int ii = x.Legs()-1; ii >= 0; --ii)
			cout << hex << a[ii] << " ";
		cout << endl;

		cout << hex << "b' = ";
		for (int ii = x.Legs()-1; ii >= 0; --ii)
			cout << hex << b[ii] << " ";
		cout << endl;

		cout << hex << "q = ";
		for (int ii = x.Legs()-1; ii >= 0; --ii)
			cout << hex << q[ii] << " ";
		cout << endl;

		cout << hex << "r = ";
		for (int ii = x.Legs()-1; ii >= 0; --ii)
			cout << hex << r[ii] << " ";
		cout << endl;

		x.Multiply(q, b, p);

		cout << hex << "p' = ";
		for (int ii = 2*x.Legs()-1; ii >= 0; --ii)
			cout << hex << p[ii] << " ";
		cout << endl;

		x.Add(p, r, p);

		cout << hex << "p = ";
		for (int ii = 2*x.Legs()-1; ii >= 0; --ii)
			cout << hex << p[ii] << " ";
		cout << endl;

		if (!x.Equal(p, a))
		{
			cout << "FAILURE: Divide" << endl;
			return 1;
		}
	}
}

int TestModularInverse()
{
	BigPseudoMersenne x(10, 256, 189);
	Leg *a = x.Get(0);
	Leg *modulus = x.Get(1);
	Leg *inverse = x.Get(2);
	Leg *p = x.Get(3);

	MersenneTwister mtprng;
	mtprng.Initialize();

	CAT_FOREVER
	{
		mtprng.Generate(a, x.RegBytes());

		x.MrInvert(a, inverse);
		x.MrMultiply(a, inverse, p);
		x.MrReduce(p);

		if (!x.EqualX(p, 1))
		{
			cout << "FAILURE: Inverse" << endl;
			return 1;
		}
	}
}

int TestSquareRoot()
{
	BigPseudoMersenne x(10, 256, 189);
	Leg *a = x.Get(0);
	Leg *modulus = x.Get(1);
	Leg *inverse = x.Get(2);
	Leg *p = x.Get(3);
	Leg *s = x.Get(4);
	Leg *t = x.Get(5);
	Leg *u = x.Get(6);

	MersenneTwister mtprng;
	mtprng.Initialize();

	CAT_FOREVER
	{
		mtprng.Generate(a, x.RegBytes());

		x.MrSquare(a, s);

		x.MrSquareRoot(s, t);

		if (!x.Equal(a, t))
		{
			x.MrNegate(t, t);

			if (!x.Equal(a, t))
			{
				cout << "FAILURE: Square" << endl;
				return 1;
			}
		}
	}

	return 0;
}

void CheckTatePairing()
{
	static const u32 BITS = 256;
	static const u32 C = KeyAgreementCommon::EDWARD_C_256;
	static const char *Q = "28948022309329048855892746252171976963461314589887294264891545010474297951221";

	// Precomputed factors of Q-1
	static const int FACTOR_COUNT = 7;
	static const char *FACTORS[FACTOR_COUNT] = {
		"2",
		"2",
		"5",
		"383",
		"547",
		"7297916317756141998510491241679",
		"946681572513972859833295814226169421059"
	};
/*
	static const u32 BITS = 384;
	static const u32 C = KeyAgreementCommon::EDWARD_C_384;
	static const char *Q = "9850501549098619803069760025035903451269934817616361666989904550033111091443156013902778362733054842505505091826087";

	// Precomputed factors of Q-1
	static const int FACTOR_COUNT = 4;
	static const char *FACTORS[FACTOR_COUNT] = {
		"2",
		"3",
		"314917",
		"5213279239237968418699615044088814646012512724313793617042958700246472928551097598151670843816547874786851293"
	};
*/
/*
	static const u32 BITS = 512;
	static const u32 C = KeyAgreementCommon::EDWARD_C_512;
	static const char *Q = "3351951982485649274893506249551461531869841455148098344430890360930441007518400663276759147760258803969928535656862608840637469081771824858089662568826887";

	// Precomputed factors of Q-1
	static const int FACTOR_COUNT = 6;
	static const char *FACTORS[FACTOR_COUNT] = {
		"2",
		"3",
		"19",
		"41",
		"947",
		"757284558829257736385628342718523674263080957668745240229124867649623681006570455646201876104541740028513467897150293958182804849079028668802470737"
	};
*/
	{
		BigMontgomery mont(6+FACTOR_COUNT, BITS);
		Leg *q = mont.Get(0);
		Leg *p = mont.Get(1);
		Leg *p_rns = mont.Get(2);
		Leg *e = mont.Get(3);
		Leg *r_rns = mont.Get(4);
		Leg *r = mont.Get(5);
		Leg *factors[7];

		for (int ii = 0; ii < FACTOR_COUNT; ++ii)
		{
			factors[ii] = mont.Get(6+ii);
		}

		if (!mont.LoadFromString(Q, 10, q))
		{
			cout << "Failure to load q" << endl;
			return;
		}

		for (int ii = 0; ii < FACTOR_COUNT; ++ii)
		{
			mont.LoadFromString(FACTORS[ii], 10, factors[ii]);
		}

		mont.SetModulus(q);

		mont.CopyX(0, p);
		mont.SubtractX(p, C);

		mont.MonInput(p, p_rns);

		for (u32 ii = 0; ii < (1 << FACTOR_COUNT); ++ii)
		{
			mont.CopyX(1, e);
			for (u32 jj = 0; jj < FACTOR_COUNT; ++jj)
			{
				if (ii & (1 << jj))
				{
					mont.Copy(e, r);
					mont.MultiplyLow(factors[jj], r, e);
				}
			}

			mont.MonExpMod(p_rns, e, r_rns);

			mont.MonOutput(r_rns, r);

			if (mont.EqualX(r, 1))
			{
				cout << endl;

				cout << "p ^ [(q-1)/(1";
				for (u32 jj = 0; jj < FACTOR_COUNT; ++jj)
				{
					if (!(ii & (1 << jj)))
					{
						cout << "*" << FACTORS[jj];
					}
				}
				cout << ")] = 1 (mod q)" << endl;
			}
			else
			{
				cout << ".";
			}
		}
	}
}

int GenerateCurveParameterC()
{
	FortunaOutput *out = FortunaFactory::ref()->Create();
	CAT_ENFORCE(out);

	const int bits = 256;

	for (int ii = 1; ii <= 65535; ii += 2)
	{
		BigMontgomery mont(16, bits);
		Leg *x = mont.Get(1);
		Leg *y = mont.Get(2);
		Leg *z = mont.Get(3);

		Leg *ctest = mont.Get(0);
		mont.CopyX(0, ctest);
		mont.SubtractX(ctest, ii);
/*
		if (mont.ModulusX(ctest, 4) != 1)
		{
			// Only suggest p = 3 mod 4 because we can use a simple square root formula for these
			continue;
		}*/
/*
		mont.SetModulus(ctest);
		mont.CopyX(12, x);
		mont.CopyX(9, y);
		mont.MonInput(x, x);
		mont.MonExpMod(x, y, z);
		mont.MonOutput(z, z);

		cout << "prime = ";
		for (int ii = mont.Legs() - 1; ii >= 0; --ii)
		{
			cout << setw(16) << setfill('0') << hex << z[ii] << ".";
		}
		cout << endl;
*/
		if (mont.IsRabinMillerPrime(out, ctest))
		{
			//cout << "Candidate value for c (with p = 3 mod 4): " << ii << endl;

			BigPseudoMersenne mer(4, bits, ii);
			Leg *a = mer.Get(0);
			Leg *p = mer.Get(1);
			mer.CopyX(0, a);
			mer.MrSubtractX(a, 1);
			mer.MrSquareRoot(a, p);
			mer.MrSquare(p, p);
			mer.MrReduce(p);

			if (mer.Equal(a, p))
			{
				//cout << "SUCCESS: a = -1 is a square in Fp" << endl;
				cout << "Candidate value for c: " << ii << " -- p mod 8 = " << mont.ModulusX(ctest, 8) << endl;
			}
			else
			{
				//cout << "FAILURE: a = -1 is NOT a square in Fp" << endl;
				//cout << "NOT Candidate value for c: " << ii << " -- p mod 8 = " << mont.ModulusX(ctest, 8) << endl;
			}
		}
	}

	return 0;
}

bool TestCurveParameters()
{
	const int BITS[] = {256, 384, 512};

	for (int ii = 0; ii < 3; ++ii)
	{
		cout << "Testing curve parameters for " << BITS[ii] << "-bit modulus:" << endl;
		FortunaOutput *out = FortunaFactory::ref()->Create();
		CAT_ENFORCE(out);
		BigTwistedEdwards *x = KeyAgreementCommon::InstantiateMath(BITS[ii]);

		Leg *a = x->Get(0);
		Leg *modulus = x->Get(1);
		Leg *inverse = x->Get(2);
		Leg *p = x->Get(3);
		Leg *s = x->Get(4);
		Leg *t = x->Get(5);
		Leg *u = x->Get(6);
		Leg *pt = x->Get(7);

		Leg d = x->GetCurveD();
		{
			x->CopyX(d, a);
			x->MrSquareRoot(a, p);
			x->MrSquare(p, p);
			x->MrReduce(p);

			if (x->Equal(a, p))
			{
				cout << "FAILURE: d is a square in Fp" << endl;
				return false;
			}
			else
			{
				cout << "SUCCESS: d is not a square in Fp. d = " << d << endl;
			}
		}

		// Test a = -1

		x->CopyX(0, a);
		x->MrSubtractX(a, 1);
		x->MrSquareRoot(a, p);
		x->MrSquare(p, p);
		x->MrReduce(p);

		if (x->Equal(a, p))
		{
			cout << "SUCCESS: a = -1 is a square in Fp" << endl;
		}
		else
		{
			cout << "FAILURE: a = -1 is NOT a square in Fp" << endl;
			return false;
		}

		//x->PtGenerate(out, pt);
		//x->PtNormalize(pt, pt);

		x->PtMultiply(x->GetGenerator(), x->GetCurveQ(), 0, p);
		x->PtNormalize(p, p); // needed since PtMultiply() result cannot normally be followed by a PtAdd()
		x->PtEAdd(p, x->GetGenerator(), p);
		x->PtNormalize(p, p);

		if (!x->Equal(x->GetGenerator(), p))
		{
			cout << "FAILURE: G*(q+1) != G" << endl;
			return false;
		}
		else
		{
			cout << "SUCCESS: G*(q+1) = G -- Verifies order of large prime subgroup" << endl;
		}

		delete out;
		delete x;

		cout << endl;
	}

	return true;
}

void GenerateMOFTable(int window_bits)
{
	cout << "When we see each combinations of w+1 bits, what operations should be performed?" << endl;
	cout << "It will be a number of doubles, then an addition by an odd number, then some more doublings." << endl;

	int top = 1 << (window_bits + 1);
	int cnt = 0;

	for (int ii = 0; ii < top; ++ii)
	{
		int r = 0;

		for (int jj = window_bits; jj >= 1; --jj)
		{
			int top = ii & (1 << (jj-1));
			int bottom = ii & (1 << jj);

			if (top)
			{
				if (bottom)
				{
					// 0
				}
				else
				{
					// 1
					r += 1 << (jj - 1);
				}
			}
			else
			{
				if (bottom)
				{
					// -1
					r -= 1 << (jj - 1);
				}
				else
				{
					// 0
				}
			}
		}

		int squares_before = window_bits;
		int squares_after = 0;
		if (r)
		{
			while (!(r&1))
			{
				--squares_before;
				++squares_after;
				r >>= 1;
			}
		}

		cout << ii << "(";

		for (int jj = 1 << window_bits; jj; jj >>= 1)
		{
			if (ii & jj)	cout << '1';
			else			cout << '0';
		}

		cout << ") -> " << squares_before << "D + (" << r << ") + " << squares_after << "D" << endl;

		// test code for the bit twiddling version of w-MOF

		u32 bits = ii;
		u32 w = window_bits;

		// invert low bits if negative, and mask out high bit
		u32 z = (bits ^ -(s32)(bits >> w)) & ((1 << w) - 1);
		// perform shift and subtract to get positive index
		u32 x = z - (z >> 1);
		// compute number of trailing zeroes in x
		u32 y = x ^ (x - 1);
		u32 shift = ((15 - y) & 16) >> 2;
		y >>= shift;
		u32 s = shift;
		shift = ((3 - y) & 4) >> 1; y >>= shift; s |= shift;
		s |= (y >> 1);
		x >>= s;

		cout << "+ " << hex << (u32)x << endl;
		cout << "D " << dec << (u32)s << endl;

		if (x)
		{
			// compute table index
			x = ((x - 1) >> 1) + ((bits & (1 << w)) >> 2);
			cout << "Table # " << dec << (u32)x << endl;
		}
		else
			cout << "Table !Zero" << endl;

/*
		if (ii & 1)
		{
			if (r >= 0)
			{
				cout << "{" << ((r + 1) >> 1) - 1 << "," << squares_after << "},";
				if (++cnt == 8)
				{
					cnt = 0;
					cout << endl;
				}
			}
		}*/
	}
}

/*
void TestCurveParameterD()
{
	// Verify that d is not a square in Fp.
	// Attempt to find its square root and then square the root to see if it matches

	u32 d[CAT_EDWARD_LIMBS];
	u32 root[CAT_EDWARD_LIMBS];
	u32 square[CAT_EDWARD_LIMBS*2];
	u32 reduced[CAT_EDWARD_LIMBS];

	Set32(d, CAT_EDWARD_LIMBS, CAT_EDWARD_D);

	SpecialSquareRoot(CAT_EDWARD_LIMBS, d, CAT_EDWARD_C, root);

	Square(CAT_EDWARD_LIMBS, square, root);

	SpecialModulus(square, CAT_EDWARD_LIMBS*2, CAT_EDWARD_C, CAT_EDWARD_LIMBS, reduced);

	if (Equal(CAT_EDWARD_LIMBS, reduced, d))
	{
		cout << "FAILURE: Twisted Edwards curve parameter d is a square in Fp" << endl;
		return;
	}

	// Negate d and try it again
	Negate(CAT_EDWARD_LIMBS, d, d);
	Subtract32(d, CAT_EDWARD_LIMBS, CAT_EDWARD_C);

	SpecialSquareRoot(CAT_EDWARD_LIMBS, d, CAT_EDWARD_C, root);

	Square(CAT_EDWARD_LIMBS, square, root);

	SpecialModulus(square, CAT_EDWARD_LIMBS*2, CAT_EDWARD_C, CAT_EDWARD_LIMBS, reduced);

	if (Equal(CAT_EDWARD_LIMBS, reduced, d))
	{
		cout << "FAILURE: Twisted Edwards curve parameter d is a square in Fp" << endl;
		return;
	}

	cout << "SUCCESS: Twisted Edwards curve parameter d is NOT a square in Fp" << endl;
}
*//*
void TimeModulusCode()
{
#define LIMBS 8
	u32 a[LIMBS*2], b[LIMBS], modulus[LIMBS];
	u32 p_x[LIMBS], p_y[LIMBS];
	u32 three[LIMBS];

	Set32(three, LIMBS, 3);
	Set32(b, LIMBS, 11);
	Set32(p_x, LIMBS, 8);
	Set32(p_y, LIMBS, 15);

	//INFO("Test") << "Generating a pseudo-prime modulus...";
	//GenerateStrongPseudoPrime(modulus, LIMBS);
	Set32(modulus, LIMBS, 0);
	const u32 C = 189;
	Subtract32(modulus, LIMBS, C);
	cout << "Modulus: " << ToStr(modulus, LIMBS, 16) << endl;

	u32 v[LIMBS*2], v_inverse[LIMBS];
	u32 p[LIMBS*2+1], presidue[LIMBS*2+1], q[LIMBS*2], r[LIMBS];

	Set(v, LIMBS*2, modulus, LIMBS);
	BarrettModulusPrecomp(LIMBS, modulus, v_inverse);

	cout << "v_inverse = " << ToStr(v_inverse, LIMBS, 16) << endl;

	MersenneTwister prng;
	prng.Initialize();

	double ac1 = 0, ac2 = 0, ac3 = 0;
	for (int ii = 0; ii < 100000; ++ii)
	{
		prng.Generate(a, LIMBS*4);
		prng.Generate(b, LIMBS*4);
		Multiply(LIMBS, p, a, b);

		MonInputResidue(p, LIMBS*2, v, LIMBS, presidue);
		u32 mod_inv = MonReducePrecomp(v[0]);

		double clk1 = Clock::usec();
		BarrettModulus(LIMBS, p, v, v_inverse, q);
		double clk2 = Clock::usec();
		MonFinish(LIMBS, presidue, v, mod_inv);
		double clk3 = Clock::usec();
		SpecialModulus(p, LIMBS*2, C, LIMBS, r);
		double clk4 = Clock::usec();

		double t3 = clk4 - clk3;
		double t2 = clk3 - clk2;
		double t1 = clk2 - clk1;

		ac1 += t1;
		ac2 += t2;
		ac3 += t3;

		for (int ii = 0; ii < LIMBS; ++ii)
		{
			if (presidue[ii] != q[ii])
			{
				cout << "FAIL: Montgomery doesn't match Barrett" << endl;
				break;
			}
			if (presidue[ii] != r[ii])
			{
				cout << "FAIL: Montgomery doesn't match Special" << endl;
				cout << "Expected = " << ToStr(q, LIMBS, 16) << endl;
				cout << "Got	  = " << ToStr(r, LIMBS, 16) << endl;
				Subtract(q, LIMBS, r, LIMBS);
				cout << "Difference = " << ToStr(q, LIMBS, 16) << endl;
				u32 crem = Divide32(LIMBS, q, C);
				cout << "Delta Divides C = " << ToStr(q, LIMBS, 16) << " times" << endl;
				cout << "Delta Modulus C = " << crem << endl;
				break;
			}
		}
	}

	cout << "MonFinish()/SpecialModulus(): " << ac2 / ac3 << endl;
	cout << "BarrettModulus()/MonFinish(): " << ac1 / ac2 << endl;
}
*/
/*
void ECCTest()
{
	u8 server_private_key[CAT_SERVER_PRIVATE_KEY_BYTES(256)];
	u8 server_public_key[CAT_SERVER_PUBLIC_KEY_BYTES(256)];

	cout << "OFFLINE PRECOMPUTATION: Generating server private and public keys" << endl;
	double t1 = Clock::usec();
	TwistedEdwardServer::GenerateOfflineStuff(256, server_private_key, server_public_key);
	double t2 = Clock::usec();
	cout << "-- Operation completed in " << (t2 - t1) << " usec" << endl;

	u32 server_private_key_init[] = {0xf51bf87c, 0x1a54bd3f, 0x604f3bee, 0xfa9456a5, 0xfc92b38d, 0xafd5e227, 0xe1c39b5c, 0x72a23ae1 };
	u32 server_public_key_init[] = {0x94731fb8, 0xd1a4ccc1, 0x4e209634, 0x71b6e742, 0x7fb0597e, 0x8339f95a, 0xb8604807, 0x8ac5191d, 0xe151bedc, 0x529bf61a, 0x1abd54ce, 0x8717df10, 0x956fd368, 0x242e1573, 0x3e32bcc7, 0x766027c7, 0x3daac87a, 0x44071c19, 0xd354c8d6, 0x14f8d20b, 0xa21dd55, 0x8c007ae3, 0xd73c7ed1, 0xcdb089a6, 0xa4513555, 0xd6e2c24d, 0x81c6394a, 0x807359a2, 0x325ff9db, 0x2521f08c, 0x9d9ae16, 0xe7752bad };

	memcpy(server_private_key, server_private_key_init, sizeof(server_private_key));
	memcpy(server_public_key, server_public_key_init, sizeof(server_public_key));

	cout << "u32 server_private_key_init[] = {";
	for (int ii = 0; ii < sizeof(server_private_key); ii += sizeof(Leg))
		cout << hex << "0x" << *(Leg*)&server_private_key[ii] << ", ";
	cout << "};" << endl;

	cout << "u32 server_public_key_init[] = {";
	for (int ii = 0; ii < sizeof(server_public_key); ii += sizeof(Leg))
		cout << hex << "0x" << *(Leg*)&server_public_key[ii] << ", ";
	cout << dec << "};" << endl;

	//for (int jj = 0; jj < 10000; ++jj)
	{
		double runtime = 0;
		int bins[100] = { 0 };
		int ii;
		for (ii = 0; ii < 1000; ++ii)
		{
			TwistedEdwardServer server;
			u8 server_shared_secret[TwistedEdwardCommon::MAX_BYTES];

			TwistedEdwardClient client;
			u8 client_shared_secret[TwistedEdwardCommon::MAX_BYTES];
			u8 client_public_key[TwistedEdwardCommon::MAX_BYTES*2];

			//cout << "ON SERVER START-UP: Loading server private key" << endl;
			t1 = Clock::usec();
			if (!server.Initialize(256))
			{
				cout << "FAILURE: Server init" << endl;
				return;
			}
			server.SetPrivateKey(server_private_key);
			t2 = Clock::usec();
			//cout << "-- Operation completed in " << (t2 - t1) << " usec" << endl;

			//cout << "ON CLIENT START-UP: Initializing" << endl;
			t1 = Clock::usec();
			if (!client.Initialize(256))
			{
				cout << "FAILURE: Client init" << endl;
				return;
			}
			t2 = Clock::usec();
			//cout << "-- Operation completed in " << (t2 - t1) << " usec" << endl;

			//cout << "CLIENT: ONLINE NOT TIME-CRITICAL: Computing shared secret" << endl;
			t1 = Clock::usec();
			if (!client.ComputeSharedSecret(server_public_key, client_public_key, client_shared_secret))
			{
				cout << "FAILURE: Client rejected server public key" << endl;
				//cout << "seed was " << ii + jj * 10000 << endl;
				return;
			}
			t2 = Clock::usec();
			//cout << "-- Operation completed in " << (t2 - t1) << " usec" << endl;

			//cout << "SERVER: ONLINE AND TIME-CRITICAL: Computing shared secret" << endl;
			t1 = Clock::usec();
			if (!server.ComputeSharedSecret(client_public_key, server_shared_secret))
			{
				cout << "FAILURE: Server rejected public key" << endl;
				//cout << "seed was " << ii + jj * 10000 << endl;
				return;
			}
			else
			{
				t2 = Clock::usec();
				//cout << "SUCCESS: Server accepted public key" << endl;
			}
			double x = t2 - t1;
			runtime += x;
			int y = (int)x - 100;
			if (y >= 0 && y < 20)
				bins[y]++;
			//cout << "-- Operation completed in " << (t2 - t1) << " usec" << endl;

			if (memcmp(server_shared_secret, client_shared_secret, client.GetSharedSecretBytes()))
			{
				cout << "FAILURE: Server shared secret did not match client shared secret" << endl;
				//cout << "seed was " << ii + jj * 10000 << endl;
				return;
			}
			else
			{
				//cout << "SUCCESS: Server shared secret matched client shared secret" << endl;
			}
		}
		cout << "Average server computation time = " << runtime/(double)ii << " usec" << endl;

		cout << "Histogram of timing with 1 usec bins:" << endl;
		for (int ii = 0; ii < 20; ++ii)
		{
			cout << bins[ii] << " <- " << 100 + ii << " usec" << endl;
		}
	}
}
*/
void HandshakeTest()
{
	for (int ii = 0; ii < 5; ++ii)
	{
		// Offline:

		double t0 = m_clock->usec();

		TunnelKeyPair key_pair;

		//cout << "Generating server public and private keys..." << endl;
		if (!key_pair.Generate(m_tls))
		{
			cout << "FAILURE: Unable to generate key pair" << endl;
			return;
		}

		double t1 = m_clock->usec();

		cout << "Key Pair Generation time = " << t1 - t0 << " usec" << endl;
/*
		cout << "Generator point = " << endl;
		for (int ii = 0; ii < tls_math->Legs() * 2 * CAT_LEG_BITS/8; ++ii)
		{
			cout << (int)server_public_key[ii] << ",";
		}
		cout << endl;

		u8 q[CAT_DEMO_PRIVATE_KEY_BYTES];
		tls_math->Save(tls_math->GetCurveQ(), q, sizeof(q));

		cout << "Q = " << endl;
		for (int ii = 0; ii < tls_math->Legs() * CAT_LEG_BITS/8; ++ii)
		{
			cout << (int)q[ii] << ",";
		}
		cout << endl;
*/
		// Startup:

		//cout << "Starting up..." << endl;
		SecureServerDemo server;
		SecureClientDemo client;

		server.Reset(m_tls, &client, key_pair);

		TunnelPublicKey public_key(key_pair);

		client.Reset(m_tls, &server, public_key);

		// Online:

		//cout << "Client wants to connect." << endl;
		client.SendHello(m_tls);

		if (client.success)
		{
			//cout << "SUCCESS: Handshake succeeded and we were able to exchange messages securely!" << endl;
		}
		else
		{
			cout << "FAILURE: Handshake failed somehow.  See messages above." << endl;
			break;
		}
	}
}

bool got_iv(u64 correct)
{
	u32 new_iv_low = (u32)(correct & AuthenticatedEncryption::IV_MASK);

	static u64 last_iv = 0;

	u64 recon = ReconstructCounter<AuthenticatedEncryption::IV_BITS>(last_iv, new_iv_low);
	last_iv = recon;
	return recon == correct;
}

void TestIVReconstruction()
{
	for (u64 iv = 0; iv < 0x5000000; iv += 10000)
	{
		if (!got_iv(iv - 17))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv - 19))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv + 3))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv + 3))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv + 2))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv - 3))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv - 1))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
		if (!got_iv(iv))
		{
			cout << "FAILURE: IV reconstruction failed at IV = " << iv << endl;
			return;
		}
	}
	cout << "SUCCESS: IV reconstruction is working properly" << endl;
}
/*
void SHA256OneRun()
{
	static const char *msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

	SHA256 sha;
	sha.Crunch(msg, (u32)strlen(msg));
	const u8 *out = sha.Finish();
}

void TestSHA256()
{
	const char *msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

	double t1 = m_clock->usec();
	SHA256 sha;
	sha.Crunch(msg, (u32)strlen(msg));
	const u8 *out = sha.Finish();
	double t2 = m_clock->usec();

	static u32 test_array[8] = {
		0x248d6a61, 0xd20638b8, 0xe5c02693, 0x0c3e6039,
		0xa33ce459, 0x64ff2167, 0xf6ecedd4, 0x19db06c1
	};

	const u32 *out32 = (const u32*)out;

	for (int ii = 0; ii < 8; ++ii)
	{
		if (out32[ii] != getBE(test_array[ii]))
		{
			cout << "FAILURE: SHA-256 output does not match example output" << endl;
			return;
		}
	}

	cout << "SUCCESS: SHA-256 output matches example output. Time: " << (t2 - t1) << " usec" << endl;

	cout << "SHA-256 ran in " << m_clock->MeasureClocks(1000, SHA256OneRun) << " clock cycles (median of test data)" << endl;
}

void SHA512OneRun()
{
	static const char *msg = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

	SHA512 sha;
	sha.Crunch(msg, (u32)strlen(msg));
	const u8 *out = sha.Finish();
}

void TestSHA512()
{
	const char *msg = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

	double t1 = m_clock->usec();
	SHA512 sha;
	sha.Crunch(msg, (u32)strlen(msg));
	const u8 *out = sha.Finish();
	double t2 = m_clock->usec();

	static u64 test_array[8] = {
		0x8e959b75dae313daLL, 0x8cf4f72814fc143fLL, 0x8f7779c6eb9f7fa1LL, 0x7299aeadb6889018LL,
		0x501d289e4900f7e4LL, 0x331b99dec4b5433aLL, 0xc7d329eeb6dd2654LL, 0x5e96e55b874be909LL
	};

	const u64 *out64 = (const u64*)out;

	for (int ii = 0; ii < 8; ++ii)
	{
		if (out64[ii] != getBE(test_array[ii]))
		{
			cout << "FAILURE: SHA-512 output does not match example output" << endl;
			return;
		}
	}

	cout << "SUCCESS: SHA-512 output matches example output. Time: " << (t2 - t1) << " usec" << endl;

	cout << "SHA-512 ran in " << m_clock->MeasureClocks(1000, SHA512OneRun) << " clock cycles (median of test data)" << endl;
}

void TestSHA384()
{
	const char *msg = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

	double t1 = m_clock->usec();
	SHA512 sha(384);
	sha.Crunch(msg, (u32)strlen(msg));
	const u8 *out = sha.Finish();
	double t2 = m_clock->usec();

	static u64 test_array[6] = {
		0x09330c33f71147e8LL, 0x3d192fc782cd1b47LL, 0x53111b173b3b05d2LL, 0x2fa08086e3b0f712LL,
		0xfcc7c71a557e2db9LL, 0x66c3e9fa91746039LL
	};

	const u64 *out64 = (const u64*)out;

	for (int ii = 0; ii < 6; ++ii)
	{
		if (out64[ii] != getBE(test_array[ii]))
		{
			cout << "FAILURE: SHA-384 output does not match example output" << endl;
			return;
		}
	}

	cout << "SUCCESS: SHA-384 output matches example output. Time: " << (t2 - t1) << " usec" << endl;
}
*/

int ParseHexText(const char *text, u8 *data, int data_max_len)
{
	char ch;
	int high_nibble = 0;
	bool seen_high = false;
	int data_len = 0;

	while ((ch = *text++))
	{
		int nibble;

		if (ch >= 'A' && ch <= 'F')
			nibble = ch - 'A' + 10;
		else if (ch >= 'a' && ch <= 'f')
			nibble = ch - 'a' + 10;
		else if (ch >= '0' && ch <= '9')
			nibble = ch - '0';
		else
			continue;

		if (!seen_high)
		{
			seen_high = true;
			high_nibble = nibble;
		}
		else
		{
			*data++ = (high_nibble << 4) | nibble;
			if (++data_len >= data_max_len)
				return data_len;
			seen_high = false;
		}
	}

	return data_len;
}

bool VerifyCryptHash(ICryptHash *hash, const char *msg, const char *expected)
{
	u8 msg_data[512];
	int msg_bytes = ParseHexText(msg, msg_data, sizeof(msg_data));

	u8 expected_data[512];
	int expected_bytes = ParseHexText(expected, expected_data, sizeof(expected_data));

	u8 actual_data[512];
	int actual_bytes = hash->GetDigestByteCount();

	hash->BeginMAC();
	hash->Crunch(msg_data, msg_bytes);
	hash->End();
	hash->Generate(actual_data, actual_bytes);

	return actual_bytes == expected_bytes && SecureEqual(actual_data, expected_data, actual_bytes);
}

void Skein256OneRun()
{
	static const u8 key[] = { 0x06 };
	static const u8 msg[] = { 0xcc };
	const int len = 1;

	u8 out[32];

	Skein hash;
	hash.BeginKey(256);
	hash.Crunch(key, 1);
	hash.End();
	hash.BeginMAC();
	hash.Crunch(msg, len);
	hash.End();
	hash.Generate(out, sizeof(out));
}

void TestSkein256()
{
	Skein hash;

	// Test vectors from Skein paper
	hash.BeginKey(256);
	if (!VerifyCryptHash(&hash,
		"FF",
		"0B 98 DC D1 98 EA 0E 50 A7 A2 44 C4 44 E2 5C 23"
		"DA 30 C1 0F C9 A1 F2 70 A6 63 7F 1F 34 E6 7E D2"))
	{
		cout << "FAIL: Skein-256 output does not match test vector 1." << endl;
		return;
	}

	hash.BeginKey(256);
	if (!VerifyCryptHash(&hash,
		"FF FE FD FC FB FA F9 F8 F7 F6 F5 F4 F3 F2 F1 F0"
		"EF EE ED EC EB EA E9 E8 E7 E6 E5 E4 E3 E2 E1 E0",
		"8D 0F A4 EF 77 7F D7 59 DF D4 04 4E 6F 6A 5A C3"
		"C7 74 AE C9 43 DC FC 07 92 7B 72 3B 5D BF 40 8B"))
	{
		cout << "FAIL: Skein-256 output does not match test vector 2." << endl;
		return;
	}

	hash.BeginKey(256);
	if (!VerifyCryptHash(&hash,
		"FF FE FD FC FB FA F9 F8 F7 F6 F5 F4 F3 F2 F1 F0"
		"EF EE ED EC EB EA E9 E8 E7 E6 E5 E4 E3 E2 E1 E0"
		"DF DE DD DC DB DA D9 D8 D7 D6 D5 D4 D3 D2 D1 D0"
		"CF CE CD CC CB CA C9 C8 C7 C6 C5 C4 C3 C2 C1 C0",
		"DF 28 E9 16 63 0D 0B 44 C4 A8 49 DC 9A 02 F0 7A"
		"07 CB 30 F7 32 31 82 56 B1 5D 86 5A C4 AE 16 2F"))
	{
		cout << "FAIL: Skein-256 output does not match test vector 3." << endl;
		return;
	}

	cout << "SUCCESS: Skein-256 output matches all test vectors" << endl;

	cout << "Skein-256 ran in " << Clock::MeasureClocks(1000, Skein256OneRun) << " clock cycles (median of test data)" << endl;
}

void Skein512OneRun()
{
	static const u8 key[] = { 0x06 };
	static const u8 msg[] = { 0xcc };
	const int len = 1;

	u8 out[32];

	Skein hash;
	hash.BeginKey(512);
	hash.Crunch(key, 1);
	hash.End();
	hash.BeginMAC();
	hash.Crunch(msg, len);
	hash.End();
	hash.Generate(out, sizeof(out));
}

void TestSkein512()
{
	Skein hash;

	// Test vectors from Skein paper
	hash.BeginKey(512);
	if (!VerifyCryptHash(&hash,
		"FF",
		"71 B7 BC E6 FE 64 52 22 7B 9C ED 60 14 24 9E 5B"
		"F9 A9 75 4C 3A D6 18 CC C4 E0 AA E1 6B 31 6C C8"
		"CA 69 8D 86 43 07 ED 3E 80 B6 EF 15 70 81 2A C5"
		"27 2D C4 09 B5 A0 12 DF 2A 57 91 02 F3 40 61 7A"))
	{
		cout << "FAIL: Skein-512 output does not match test vector 1." << endl;
		return;
	}

	hash.BeginKey(512);
	if (!VerifyCryptHash(&hash,
		"FF FE FD FC FB FA F9 F8 F7 F6 F5 F4 F3 F2 F1 F0"
		"EF EE ED EC EB EA E9 E8 E7 E6 E5 E4 E3 E2 E1 E0"
		"DF DE DD DC DB DA D9 D8 D7 D6 D5 D4 D3 D2 D1 D0"
		"CF CE CD CC CB CA C9 C8 C7 C6 C5 C4 C3 C2 C1 C0",
		"45 86 3B A3 BE 0C 4D FC 27 E7 5D 35 84 96 F4 AC"
		"9A 73 6A 50 5D 93 13 B4 2B 2F 5E AD A7 9F C1 7F"
		"63 86 1E 94 7A FB 1D 05 6A A1 99 57 5A D3 F8 C9"
		"A3 CC 17 80 B5 E5 FA 4C AE 05 0E 98 98 76 62 5B"))
	{
		cout << "FAIL: Skein-512 output does not match test vector 2." << endl;
		return;
	}

	hash.BeginKey(512);
	if (!VerifyCryptHash(&hash,
		"FF FE FD FC FB FA F9 F8 F7 F6 F5 F4 F3 F2 F1 F0"
		"EF EE ED EC EB EA E9 E8 E7 E6 E5 E4 E3 E2 E1 E0"
		"DF DE DD DC DB DA D9 D8 D7 D6 D5 D4 D3 D2 D1 D0"
		"CF CE CD CC CB CA C9 C8 C7 C6 C5 C4 C3 C2 C1 C0"
		"BF BE BD BC BB BA B9 B8 B7 B6 B5 B4 B3 B2 B1 B0"
		"AF AE AD AC AB AA A9 A8 A7 A6 A5 A4 A3 A2 A1 A0"
		"9F 9E 9D 9C 9B 9A 99 98 97 96 95 94 93 92 91 90"
		"8F 8E 8D 8C 8B 8A 89 88 87 86 85 84 83 82 81 80",
		"91 CC A5 10 C2 63 C4 DD D0 10 53 0A 33 07 33 09"
		"62 86 31 F3 08 74 7E 1B CB AA 90 E4 51 CA B9 2E"
		"51 88 08 7A F4 18 87 73 A3 32 30 3E 66 67 A7 A2"
		"10 85 6F 74 21 39 00 00 71 F4 8E 8B A2 A5 AD B7"))
	{
		cout << "FAIL: Skein-512 output does not match test vector 3." << endl;
		return;
	}

	cout << "SUCCESS: Skein-512 output matches all test vectors." << endl;

	cout << "Skein-512 ran in " << m_clock->MeasureClocks(1000, Skein512OneRun) << " clock cycles (median of test data)" << endl;
}

static int cc_bytes;
static ChaChaKey cc_test_key;

void ChaChaOnce()
{
	char in[1500], out[1500];

	ChaChaOutput cc_test;
	cc_test.ReKey(cc_test_key, 0x0123456701234567LL);
	cc_test.Crypt(in, out, cc_bytes);
}

void TestChaCha()
{
	cout << "ChaCha timing results:" << endl;

	const char *key = "what is the key?";

	cc_test_key.Set(key, (int)strlen(key));

	static const int TIMING_BYTES[] = {
		16, 64, 128, 256, 512, 1024, 1500
	};

	for (int ii = 0; ii < 7; ++ii)
	{
		cc_bytes = TIMING_BYTES[ii];
		cout << cc_bytes << " bytes: " << m_clock->MeasureClocks(1000, ChaChaOnce)/(float)cc_bytes << " cycles/byte" << endl;
	}
}




BigTwistedEdwards *xt;
Leg *ptt;

Leg *rtt;

Leg *gtt;

Leg *utt;

Leg *stt;



void ECCSetup()
{
	FortunaOutput *output = FortunaFactory::ref()->Create();
	CAT_ENFORCE(output);

	xt = KeyAgreementCommon::InstantiateMath(256);
	ptt = xt->Get(0);
	rtt = xt->Get(4);
	gtt = xt->Get(5);
	utt = xt->Get(9);
	stt = xt->Get(13);

	xt->PtGenerate(output, gtt);
	xt->SaveAffineXY(gtt, stt, stt + xt->Legs());

	delete output;
}



void ECCSpeed()
{
	if (!xt->LoadVerifyAffineXY(stt, stt + xt->Legs(), utt) ||
		xt->IsAffineIdentity(utt))
	{
		cout << "Error!" << endl;
	}

	xt->PtDoubleZ1(utt, utt);
	xt->PtEDouble(utt, utt);

	xt->PtMultiply(utt, xt->GetCurveQ(), 0, ptt);
	xt->SaveAffineX(ptt, rtt);
}


void GenerateLottoTicketNumbers()
{
	FortunaOutput *output = FortunaFactory::ref()->Create();
	CAT_ENFORCE(output);

	cout << "Lotto Numbers: ";

	// 1 - 56 x 5 (without replacement)
	// 1 - 46 x 1 (separate)

	u32 white_balls[5], gold_ball;

	for (int ii = 0; ii < 5; ++ii)
	{
		bool retry;
		u32 number;

		do
		{
			retry = false;
			number = output->GenerateUnbiased(1, 56);

			for (int jj = 0; jj < ii; ++jj)
			{
				if (white_balls[jj] == number)
				{
					retry = true;
					break;
				}
			}
		} while (retry);

		white_balls[ii] = number;

		cout << number << " ";
	}

	gold_ball = output->GenerateUnbiased(1, 46);

	cout << "{ " << gold_ball << " }";

	cout << endl;

	delete output;
}



void GeneratePassword()
{
	FortunaOutput *output = FortunaFactory::ref()->Create();
	CAT_ENFORCE(output);

	cout << "Password: ";

	for (int ii = 0; ii < 16; ++ii)
	{
		// 0-25 = A-Z
		// 26-51 = a-z
		// 52-61 = 0-9
		char *symbols = "!@#$%^&*()_~`'"; // 14

		u32 offset = output->GenerateUnbiased(0, 26 + 26 + 10 + 14 - 1);
		char passch;

		if (offset < 26)
		{
			passch = 'A' + offset;
		}
		else if (offset < 52)
		{
			offset -= 26;
			passch = 'a' + offset;
		}
		else if (offset < 62)
		{
			offset -= 26 + 26;
			passch = '0' + offset;
		}
		else
		{
			offset -= 62;
			passch = symbols[offset];
		}

		cout << passch;
	}

	cout << endl;

	delete output;
}


int main()
{
	m_clock = Clock::ref();

	SlowTLS::ref()->Find(m_tls);

	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();
	GeneratePassword();

	//GenerateCurveParameterC();

	ECCSetup();

	cout << "EC-DH: " << Clock::MeasureClocks(1000, ECCSpeed) << " cycles" << endl;

	//return 0;

	CheckTatePairing();
	//return GenerateCurveParameterC();
	//GenerateWMOFTable();
	//return 0;
	//return TestDivide();
	//return TestModularInverse();
	//return TestSquareRoot();

	if (!TestCurveParameters())
		return 0;

/*
	cout << endl << "Atomic testing:" << endl;
	if (!Atomic::UnitTest())
	{
		cout << "FAILURE: Atomic test failed" << endl;
		return 1;
	}
	cout << "SUCCESS: Atomic functions work!" << endl;

/*
	cout << endl << "ECC testing:" << endl;
	ECCTest();
*/
	//return 0;
/*
	cout << endl << "Testing curve parameter d:" << endl;
	TestCurveParameterD();

	cout << "Candidate primes for ECC:" << endl;
	GenerateCandidatePrimes();
*/
	//cout << endl << "w-MOF table generation:" << endl;
	//GenerateMOFTable(4);
/*
	cout << endl << "Modulus code timing:" << endl;
	TimeModulusCode();
*/
	cout << endl << "Full handshake testing:" << endl;
	HandshakeTest();

	cout << endl << "IV reconstruction testing:" << endl;
	TestIVReconstruction();

	cout << endl << "Hash testing and timing:" << endl;
	TestSkein256();
	TestSkein512();

	cout << endl << "ChaCha testing and timing:" << endl;
	TestChaCha();

	GenerateLottoTicketNumbers();
	GenerateLottoTicketNumbers();

	cout << endl << "Press ENTER to quit." << endl;
	cin.get();

	return 0;
}
