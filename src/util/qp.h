/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _QP_H_
#define _QP_H_

#include <stdio.h>

int __inline__ EncodeQuoted(const unsigned char* pSrc, int nSrcLen, char* pDst, int nMaxLineLen)
{
	int nDstLen;
	int nLineLen;
	nDstLen = 0;
	nLineLen = 0;
	for (int i = 0; i < nSrcLen; i++, pSrc++)
	{
		// ASCII 33-60, 62-126 is no need to process
		if ((*pSrc >= '!') && (*pSrc <= '~') && (*pSrc != '='))
		{
			*pDst++ = (char)*pSrc;
			nDstLen++;
			nLineLen++;
		}
		else
		{
			sprintf(pDst, "=%02X", *pSrc);
			pDst += 3;
			nDstLen += 3;
			nLineLen += 3;
		}
		if (nLineLen >= nMaxLineLen - 3)
		{
			sprintf(pDst, "=\r\n");
			pDst += 3;
			nDstLen += 3;
			nLineLen = 0;
		}
	}
	*pDst = '\0';
	return nDstLen;
}

int __inline__ DecodeQuoted(const char* pSrc, int nSrcLen, unsigned char* pDst)
{
	int nDstLen;
	int i;
	i = 0;
	nDstLen = 0;
	while (i < nSrcLen)
	{
		if (strncmp(pSrc, "=\r\n", 3) == 0) // ignore \r\n
		{
			pSrc += 3;
			i += 3;
		}
		else
		{
			if (*pSrc == '=')  // is encoded character
			{
				sscanf(pSrc, "=%02X", pDst);
				pDst++;
				pSrc += 3;
				i += 3;
			}
			else // no encoded character
			{
				*pDst++ = (unsigned char)*pSrc++;
				i++;
			}
			nDstLen++;
		}
	}
	*pDst = '\0';
	return nDstLen;
}


#endif /* _QP_H_ */

