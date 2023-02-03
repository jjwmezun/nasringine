#include <math.h>
#include "nasr_math.h"
#include <string.h>

int NasrGetDigit( int n, int d )
{
    double a = ( double )( n % ( int )( pow( 10, d ) ) );
    return d > 1 ? ( int )( floor( a / pow( 10, d - 1 ) ) ) : a;
};

int NasrGetNextPrime( int n )
{
    while ( !NasrIsPrime( n ) )
    {
        ++n;
    }
    return n;
};

int NasrGetNumberODigits( int n )
{
    int i = 1;
    while ( n >= pow( 10, i ) )
    {
        ++i;
    }
    return i;
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

int NasrIsPrime( int n )
{
    for ( int i = 2; i < n; ++i )
    {
        if ( n % i == 0 ) return 0;
    }
    return 1;
};