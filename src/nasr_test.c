#include "nasr.h"
#include "nasr_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int running = 1;

typedef enum Input {
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_DOWN,
    INPUT_UP,
    INPUT_Z,
    INPUT_X,
    INPUT_Y,
    INPUT_A,
    INPUT_S,
    INPUT_Q
} Input;

void NasrTestRun( void )
{
    NasrInit( "Nasringine 0.1", 520, 320, 5, 1024, 1024, 18, NASR_SAMPLING_NEAREST, NASR_INDEXED_YES );
    NasrSetPalette( "assets/palette.png" );
    NasrInput inputs[] =
    {
        { INPUT_RIGHT, NASR_KEY_RIGHT },
        { INPUT_LEFT, NASR_KEY_LEFT },
        { INPUT_DOWN, NASR_KEY_DOWN },
        { INPUT_UP, NASR_KEY_UP },
        { INPUT_Z, NASR_KEY_Z },
        { INPUT_Y, NASR_KEY_Y },
        { INPUT_X, NASR_KEY_X },
        { INPUT_A, NASR_KEY_A },
        { INPUT_S, NASR_KEY_S },
        { INPUT_Q, NASR_KEY_Q }
    };
    NasrRegisterInputs( inputs, 10 );

    #define RECTCOUNT 128
    #define SPEED 1.0f

    time_t t;
    srand( ( unsigned )( time( &t ) ) );
    int ids[ RECTCOUNT ];
    float accx[ RECTCOUNT ];
    float accy[ RECTCOUNT ];

    for ( int i = 0; i < RECTCOUNT; ++i )
    {
        NasrRect r = { ( i % 16 ) * 32.0f + 16.0f, floor( i / 16 ) * 32.0f + 16.0f, 16.0f, 16.0f };
        NasrColor c1 = { ( i % 8 ) * 32.0f, floor( i / 8 ) * 32.0f, 255.0f, 255.0f };
        NasrColor c2 = {  255.0f, ( i % 8 ) * 32.0f, floor( i / 8 ) * 32.0f, 255.0f };
        int d = rand() % 8;
        ids[ i ] = NasrGraphicsAddRectGradient
        (
            0,
            0,
            0,
            r,
            d,
            c1,
            c2
        );
        switch ( d )
        {
            case ( NASR_DIR_UP ):
            {
                accy[ i ] = -SPEED;
                accx[ i ] = 0.0f;
            }
            break;
            case ( NASR_DIR_UPRIGHT ):
            {
                accy[ i ] = -SPEED;
                accx[ i ] = SPEED;
            }
            break;
            case ( NASR_DIR_RIGHT ):
            {
                accy[ i ] = 0.0f;
                accx[ i ] = SPEED;
            }
            break;
            case ( NASR_DIR_DOWNRIGHT ):
            {
                accy[ i ] = SPEED;
                accx[ i ] = SPEED;
            }
            break;
            case ( NASR_DIR_DOWN ):
            {
                accy[ i ] = SPEED;
                accx[ i ] = 0.0f;
            }
            break;
            case ( NASR_DIR_DOWNLEFT ):
            {
                accy[ i ] = SPEED;
                accx[ i ] = -SPEED;
            }
            break;
            case ( NASR_DIR_LEFT ):
            {
                accy[ i ] = 0.0f;
                accx[ i ] = -SPEED;
            }
            break;
            case ( NASR_DIR_UPLEFT ):
            {
                accy[ i ] = -SPEED;
                accx[ i ] = -SPEED;
            }
            break;
        }
    }

    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {
            for ( int i = 0; i < RECTCOUNT; ++i )
            {
                NasrGraphic * g = NasrGraphicGet( ids[ i ] );
                g->data.rect.rect.x += accx[ i ];
                g->data.rect.rect.y += accy[ i ];
                if ( g->data.rect.rect.x > 520.0f )
                {
                    accx[ i ] = -SPEED;
                }
                else if ( g->data.rect.rect.x < 0.0f )
                {
                    accx[ i ] = SPEED;
                }
                if ( g->data.rect.rect.y > 320.0f )
                {
                    accy[ i ] = -SPEED;
                }
                else if ( g->data.rect.rect.y < 0.0f )
                {
                    accy[ i ] = SPEED;
                }
            }
            NasrUpdate();
        }
    }
    NasrClose();
};