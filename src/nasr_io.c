#include "nasr_io.h"
#include <stdio.h>
#include <stdlib.h>

char * NasrReadFile( const char * filename )
{
    // Open file.
    FILE * file = fopen( filename, "rb" );

    if ( file == NULL )
    {
        return 0;
    }

    // Go thru file to find size.
    fseek( file, 0L, SEEK_END );
    const size_t size = ftell( file );

    // Go back to start o’ file.
    rewind( file );

    // Allocate buffer based on found size.
    char * buffer = ( char * )( malloc( size + 1 ) );

    if ( buffer == NULL )
    {
        return 0;
    }

    // Read file into buffer & get buffer length.
    const size_t bytes = fread( buffer, sizeof( char ), size, file );

    if ( bytes < size )
    {
        return 0;
    }

    // Make sure buffer ends with string terminator.
    buffer[ bytes ] = '\0';

    fclose( file );
    return buffer;
};