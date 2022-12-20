#include "nasr_math.h"

int NasrMathIsPrime( int n )
{
    for ( int i = 2; i < n; ++i )
    {
        if ( n % i == 0 ) return 0;
    }
    return 1;
};

uint32_t NasrHashString( const char * key, int max )
{
    uint32_t hash = 2166136261u;
    const int length = strlen( key );
    for ( int i = 0; i < length; i++ )
    {
        hash ^= ( uint8_t )( key[ i ] );
        hash *= 16777619;
    }
    return hash % max;
};

int NasrGetNextPrime( int n )
{
    while ( !NasrMathIsPrime( n ) )
    {
        ++n;
    }
    return n;
};