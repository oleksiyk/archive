#include "sha1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main()
{
    SHA1 sha1;
    SHA1::Digest digest;
    unsigned i, n;

    static char foo[1000001];
    static char *testcases[]={"abc", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", foo};

    memset(foo, 'a', 1000000);
    for (n=0; n<sizeof(testcases)/sizeof(testcases[0]); n++){
        i=strlen(testcases[n]);
        sha1.digest(testcases[n], i, digest);
        printf( (i < 200 ? "SHA1(%s)=": "SHA1(%-1.20s...)="), testcases[n]);

        for (i=0; i<20; i++){
            if (i && (i & 3) == 0)  putchar(' ');
            printf("%02X", digest[i]);
        }
        printf("\n");
    }
    
    /* RFC2202 HMAC test vectors */
    sha1.hmac((const unsigned char*)"Jefe", 4, (const unsigned char*)"what do ya want for nothing?", 28, digest);
    for (size_t j=0; j<sizeof(digest); j++){
        printf("%02x", digest[j]);
    }
    printf("\n");

    unsigned char key[80];
    memset(key, 0xaa, 80);    
    const char * data = "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data";
    sha1.hmac(key, 80, (const unsigned char*)data, strlen(data), digest);
    for (size_t j=0; j<sizeof(digest); j++){
        printf("%02x", digest[j]);
    }
    printf("\n");
    
    return (0);     
}
