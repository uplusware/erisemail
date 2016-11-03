/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_

#include "base64.h"
#include "DES.h"

#define DEF_COOKIE_DES_KEY "THJ$%gty"

class Security
{
public:
	//Source -> DES -> Base64
	static int Encrypt(const char* srcBuf, int srcLen, string & strOut, const char* key = NULL)
	{
		int ntmp1 = ALIGN_8(srcLen);
		char* ptmp1 = (char*)malloc(ntmp1);
		memset(ptmp1, 0, ntmp1);
		memcpy(ptmp1, srcBuf, srcLen);
        
        DES des;
		des.Init(1);
		des.SetKey(key == NULL ? DEF_COOKIE_DES_KEY : key);
		for(int x = 0; x < ntmp1/8; x++)
			des.Encode(ptmp1 + x*8);
		des.Done();

		int ntmp2 = BASE64_ENCODE_OUTPUT_MAX_LEN(ntmp1);
		char* ptmp2 = (char*)malloc(ntmp2);
		memset(ptmp2, 0, ntmp2);

		if(CBase64::Encode(ptmp1, ntmp1, ptmp2, &ntmp2) == -1)
		{
			free(ptmp2);
			free(ptmp1);
			return -1;
		}
		strOut = ptmp2;
		free(ptmp2);
		free(ptmp1);
		return 0;
	}

	//Source -> Base64 -> DES
	static int Decrypt(const char* srcBuf, int srcLen, string & strOut, const char* key = NULL)
	{
		int ntmp1 = BASE64_DECODE_OUTPUT_MAX_LEN(srcLen);
		char* ptmp1 = (char*)malloc(ntmp1);
		memset(ptmp1, 0, ntmp1);
		if(CBase64::Decode((char*)srcBuf, srcLen, ptmp1, &ntmp1) == -1)
		{
			free(ptmp1);
			return -1;
		}

		if(ntmp1%8 != 0)
			return -1;
		else
		{
            DES des;
			des.Init(1);
			des.SetKey(key == NULL ? DEF_COOKIE_DES_KEY : key);
			for(int x = 0; x < ntmp1/8; x++)
				des.Decode(ptmp1 + x*8);
			des.Done();
		}
		strOut = ptmp1;
		free(ptmp1);
		return 0;
	}
};

#endif /* _ENCRYPT_H_ */
