#include "nasr.h"
#include "nasr_log.h"
#include <stdarg.h>
#include <stdio.h>

void NasrLog( const char * format, ... )
{
    #ifdef NASR_DEBUG
        va_list arg;
        va_start( arg, format );
        vfprintf( stderr, format, arg );
        va_end( arg );
        fprintf( stderr, "\n" );
    #endif
};