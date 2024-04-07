#ifndef __YAPI_MD5_H__
#define __YAPI_MD5_H__

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];   
} yapi_MD5_CTX;


#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))

#define FF(a,b,c,d,x,s,ac) \
{ \
    a += F(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}
#define GG(a,b,c,d,x,s,ac) \
{ \
    a += G(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}
#define HH(a,b,c,d,x,s,ac) \
{ \
    a += H(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}
#define II(a,b,c,d,x,s,ac) \
{ \
    a += I(b,c,d) + x + ac; \
    a = ROTATE_LEFT(a,s); \
    a += b; \
}                                            
void yapi_MD5Init(yapi_MD5_CTX *context);
void yapi_MD5Update(yapi_MD5_CTX *context, unsigned char *input, unsigned int inputlen);
void yapi_MD5Final(yapi_MD5_CTX *context, unsigned char digest[16]);
void yapi_MD5Transform(unsigned int state[4], unsigned char block[64]);
void yapi_MD5Encode(unsigned char *output, unsigned int *input, unsigned int len);
void yapi_MD5Decode(unsigned int *output, unsigned char *input, unsigned int len);

#endif /* __YAPI_MD5_H__ */
