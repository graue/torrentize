#ifndef _SHA1LIB_H_
#define _SHA1LIB_H_

#ifndef  i386   /* For ALPHA  (SAK) */
#define MACHINE_IS_LITTLE_ENDIAN 
typedef          long int int64;
typedef unsigned long int uint64;
typedef          int int32;
typedef unsigned int uint32;
#else  /*i386*/
#define MACHINE_IS_LITTLE_ENDIAN 
typedef          long long int int64;
typedef unsigned long long int uint64;
typedef          long int int32;
typedef unsigned long int uint32;
#endif /*i386*/

typedef struct {
  uint32 state[5];
  uint32 count[2];
  unsigned char buffer[64];
} SHA1_CTX;

#define SHA1_DIGEST_LENGTH 20
#define SHA1_DIGEST_STRING_LENGTH (SHA1_DIGEST_LENGTH*2 + 1)

void SHA1Init(SHA1_CTX *context);
void SHA1Update(SHA1_CTX *context, const unsigned char *data, uint32 len);	/* JHB */
void SHA1Final(unsigned char digest[SHA1_DIGEST_LENGTH], SHA1_CTX *context);
char *SHA1End(SHA1_CTX *context, char *buf);
char *SHA1Data(const unsigned char *data, uint32 len, char *buf);

#endif
