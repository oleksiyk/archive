#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

#include "md5.h"

static const char * const teststr[]={
"",
"a",
"abc",
"message digest",
"abcdefghijklmnopqrstuvwxyz",
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
"12345678901234567890123456789012345678901234567890123456789012345678901234567890"
};

char *salts[4] = {"abcdef","01234567","76543210","QWERTY"};
char *passwds[4]={"rosebud","trust noone", "trust, but verify", "for the world is hollow, and I have touched the sky"};

void * routine(void *)
{
    MD5 md5;
    MD5::Digest digest;

    for(int k = 0; k<100; k++){
        for (size_t i=0; i<sizeof(teststr)/sizeof(teststr[0]); i++){
        
            md5.digest(teststr[i], strlen(teststr[i]), digest);

            printf("MD5 (\"%s\") = ", teststr[i]);
            for (size_t j=0; j<sizeof(digest); j++){
                printf("%02x", digest[j]);
            }
            printf("\n");
        }
    }
    
    for (size_t i=0; i<sizeof(salts)/sizeof(salts[0]); i++){
        printf("Salt: %s\nPassword: %s\nHash:%s\n\n",salts[i], passwds[i],
            md5.crypt(passwds[i], salts[i]));
    }

    /* RFC2202 HMAC test vectors */
    md5.hmac((const unsigned char*)"Jefe", 4, (const unsigned char*)"what do ya want for nothing?", 28, digest);
    for (size_t j=0; j<sizeof(digest); j++){
        printf("%02x", digest[j]);
    }
    printf("\n");

    unsigned char key[80];
    memset(key, 0xaa, 80);    
    const char * data = "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data";
    md5.hmac(key, 80, (const unsigned char*)data, strlen(data), digest);
    for (size_t j=0; j<sizeof(digest); j++){
        printf("%02x", digest[j]);
    }
    printf("\n");

    // from CRAM-MD5 authentication example
    md5.hmac((const unsigned char*)"tanstaaftanstaaf", strlen("tanstaaftanstaaf"), (const unsigned char*)"<1896.697170952@postoffice.reston.mci.net>", strlen("<1896.697170952@postoffice.reston.mci.net>"), digest);
    for (size_t j=0; j<sizeof(digest); j++){
        printf("%02x", digest[j]);
    }
    printf("\n");
    
    return NULL;
}

int main()
{
    struct timeval tv1, tv2;
    
    gettimeofday(&tv1, NULL);

    pthread_t threads[25];
    for(int i = 0; i<=0; i++){
        pthread_create(&threads[i], NULL, routine, NULL);
    }
    
    for(int i = 0; i<=0; i++){
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&tv2, NULL);

    printf("execution time - %d sec\n", (int)(tv2.tv_sec-tv1.tv_sec));

    return 0;
}
