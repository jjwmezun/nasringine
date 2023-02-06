#ifndef NASR_MATH_H
#define NASR_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint32_t hash_t;
typedef struct NasrHashKey
{
    char * string;
    hash_t hash;
} NasrHashKey;

#define NASR_MATH_MAX( a, b ) (((a) > (b)) ? (a) : (b))
#define NASR_MATH_MIN( a, b ) (((a) < (b)) ? (a) : (b))

int NasrGetDigit( int n, int d );
int NasrGetNextPrime( int n );
int NasrGetNumberODigits( int n );
uint32_t NasrHashString( const char * key, int max );
int NasrIsPrime( int n );

#ifdef __cplusplus
}
#endif

#endif // NASR_MATH_H