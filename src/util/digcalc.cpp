/*
The code comes from RFC2617. http://www.faqs.org/rfcs/rfc2617.html
Brian Sheng modify it. uplusware@gmail.com
*/
#include "md5.h"

#include <string.h>
#include "digcalc.h"

void CvtHex(
    IN HASH Bin,
    OUT HASHHEX Hex
    )
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[HASHHEXLEN] = '\0';
};

/* calculate H(A1) as per spec */
void DigestCalcHA1(
    IN char * pszAlg,
    IN char * pszUserName,
    IN char * pszRealm,
    IN char * pszPassword,
    IN char * pszNonce,
    IN char * pszCNonce,
    IN char * pszAuthzid,
    OUT HASHHEX SessionKey
    )
{
      MD5_CTX_OBJ Md5Ctx;
      HASH HA1;
	  Md5Ctx.MD5Init();
      Md5Ctx.MD5Update((unsigned char*) pszUserName, strlen(pszUserName));
      Md5Ctx.MD5Update((unsigned char*) ":", 1);
      Md5Ctx.MD5Update((unsigned char*) pszRealm, strlen(pszRealm));
      Md5Ctx.MD5Update((unsigned char*) ":", 1);
      Md5Ctx.MD5Update((unsigned char*) pszPassword, strlen(pszPassword));
      Md5Ctx.MD5Final((unsigned char*)HA1);
      
      
      if (strcasecmp(pszAlg, "md5-sess") == 0) {

            Md5Ctx.MD5Init();
            Md5Ctx.MD5Update((unsigned char*) HA1, HASHLEN);
            Md5Ctx.MD5Update((unsigned char*) ":", 1);
            Md5Ctx.MD5Update((unsigned char*) pszNonce, strlen(pszNonce));
            Md5Ctx.MD5Update((unsigned char*) ":", 1);
            Md5Ctx.MD5Update((unsigned char*) pszCNonce, strlen(pszCNonce));
            if(pszAuthzid && *pszAuthzid)
            {
                Md5Ctx.MD5Update((unsigned char*) ":", 1);
                Md5Ctx.MD5Update((unsigned char*) pszAuthzid, strlen(pszAuthzid));
            }
            Md5Ctx.MD5Final((unsigned char*)HA1);
      };
      CvtHex(HA1, SessionKey);
};

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    IN HASHHEX HA1,           /* H(A1) */
    IN char * pszNonce,       /* nonce from server */
    IN char * pszNonceCount,  /* 8 hex digits */
    IN char * pszCNonce,      /* client nonce */
    IN char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    IN char * pszMethod,      /* method from the request */
    IN char * pszDigestUri,   /* requested URL */
    IN HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    OUT HASHHEX Response      /* request-digest or response-digest */
    )
{
      MD5_CTX_OBJ Md5Ctx;
      HASH HA2;
      HASH RespHash;
      HASHHEX HA2Hex;

      
      
      // calculate H(A2)
      Md5Ctx.MD5Init();
      Md5Ctx.MD5Update((unsigned char*) pszMethod, strlen(pszMethod));
      Md5Ctx.MD5Update((unsigned char*) ":", 1);
      Md5Ctx.MD5Update((unsigned char*) pszDigestUri, strlen(pszDigestUri));
      if (pszQop && strcasecmp(pszQop, "auth-int") == 0) {
            Md5Ctx.MD5Update((unsigned char*) ":", 1);
            Md5Ctx.MD5Update((unsigned char*) HEntity, HASHHEXLEN);
      };
      Md5Ctx.MD5Final((unsigned char*)HA2);
      CvtHex(HA2, HA2Hex);

      
      
      // calculate response
      Md5Ctx.MD5Init();
      Md5Ctx.MD5Update((unsigned char*) HA1, HASHHEXLEN);
      Md5Ctx.MD5Update((unsigned char*) ":", 1);
      Md5Ctx.MD5Update((unsigned char*) pszNonce, strlen(pszNonce));
      Md5Ctx.MD5Update((unsigned char*) ":", 1);
      if (pszQop && *pszQop) {

          Md5Ctx.MD5Update((unsigned char*) pszNonceCount, strlen(pszNonceCount));
          Md5Ctx.MD5Update((unsigned char*) ":", 1);
          Md5Ctx.MD5Update((unsigned char*) pszCNonce, strlen(pszCNonce));
          Md5Ctx.MD5Update((unsigned char*) ":", 1);
          Md5Ctx.MD5Update((unsigned char*) pszQop, strlen(pszQop));
          Md5Ctx.MD5Update((unsigned char*) ":", 1);
      };
      Md5Ctx.MD5Update((unsigned char*) HA2Hex, HASHHEXLEN);
      Md5Ctx.MD5Final((unsigned char*)RespHash);
      CvtHex(RespHash, Response);
};


/*
TEST CODE
*/

/*
#include <stdio.h>
void main(int argc, char ** argv) {

      char * pszNonce = "dcd98b7102dd2f0e8b11d0f600bfb0c093";
      char * pszCNonce = "0a4f113b";
      char * pszUser = "Mufasa";
      char * pszRealm = "testrealm@host.com";
      char * pszPass = "Circle Of Life";
      char * pszAlg = "md5";
      char szNonceCount[9] = "00000001";
      char * pszMethod = "GET";
      char * pszQop = "auth";
      char * pszURI = "/dir/index.html";
      HASHHEX HA1;
      HASHHEX HA2 = "";
      HASHHEX Response;

      DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce, pszCNonce, HA1);
      DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop, pszMethod, pszURI, HA2, Response);
      printf("Response = %s\n", Response);
};

*/