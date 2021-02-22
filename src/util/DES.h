#ifndef _DES_H_
#define _DES_H_

#define CONSTANT_DATA
#define PERMUTATION

#define ALIGN_8(L) ((L)%8 == 0 ? (L) : ((L)/8 + 1)*8)

class DES
{
private:
    void spinit();
#ifdef PERMUTATION
    /* initialize a perm array */
    void perminit(char perm[16][16][8], char p[64]);
#endif /* PERMUTATION */
    int f(unsigned int r,unsigned char subkey[8]);
    void permute(char * inblock,char perm[16][16][8],char *outblock);
    void round(int num,unsigned int *block);
    unsigned int byteswap(unsigned int x);

public:
    DES() { kn = NULL; sp_ = NULL; fperm = NULL; iperm = NULL;};
    virtual ~DES() {};
	/* Allocate space and initialize DES lookup arrays
	 * mode == 0: standard Data Encryption Algorithm
	 * mode == 1: DEA without initial and final permutations for speed
	 * mode == 2: DEA without permutations and with 128-byte key (completely
	 *	      independent subkeys for each round)
	 */
	int Init(int mode);

	/* Set key (initialize key schedule array) */
	void SetKey(const char *key);

	/* In-place encryption of 64-bit block */
	void Encode(char *block);

	/* In-place decryption of 64-bit block */
	void Decode(char *block);

	/* Free up storage used by DES */
	void Done(void);

private:
    /* Lookup tables initialized once only at startup by desinit() */
    int (*sp_)[64]; 	/* Combined S and P boxes */

    #ifdef PERMUTATION
    char CONSTANT_DATA (*iperm)[16][8];	/* Initial and final permutations */
    char (*fperm)[16][8];
    #else
    #define iperm 0
    #define fperm 0
    #endif

    /* 8 6-bit subkeys for each of 16 rounds, initialized by setkey() */
    unsigned char (*kn)[8];
    int desmode;
};

#endif /* _DES_H_ */
