#include "application.h"

extern "C" {
#define MD5HASHLEN 16
typedef unsigned char MD5HASH[MD5HASHLEN];
#define MD5HASHHEXLEN 32
typedef char MD5HASHHEX[MD5HASHHEXLEN+1];
#define IN
#define OUT

/* calculate H(A1) as per HTTP Digest spec */
void DigestCalcHA1(
    IN String pszAlg,
    IN String pszUserName,
    IN String pszRealm,
    IN String pszPassword,
    IN String pszNonce,
    IN String pszCNonce,
    OUT MD5HASHHEX SessionKey
    );

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    IN MD5HASHHEX HA1,           /* H(A1) */
    IN String pszNonce,       /* nonce from server */
    IN String pszNonceCount,  /* 8 hex digits */
    IN String pszCNonce,      /* client nonce */
    IN String pszQop,         /* qop-value: "", "auth", "auth-int" */
    IN String pszMethod,      /* method from the request */
    IN String pszDigestUri,   /* requested URL */
    IN MD5HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    OUT MD5HASHHEX Response      /* request-digest or response-digest */
    );
}
