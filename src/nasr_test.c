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
    INPUT_Q,
    INPUT_O,
    INPUT_PLUS,
    INPUT_MINUS,
    INPUT_F,
    INPUT_G
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
        { INPUT_Q, NASR_KEY_Q },
        { INPUT_O, NASR_KEY_O },
        { INPUT_PLUS, NASR_KEY_RIGHT_BRACKET },
        { INPUT_MINUS, NASR_KEY_LEFT_BRACKET },
        { INPUT_F, NASR_KEY_F },
        { INPUT_G, NASR_KEY_G }
    };
    NasrRegisterInputs( inputs, 15 );

    #define RECTCOUNT 128
    #define SPEED 1.0f
    #define NASRSPEED 4.0f

    time_t t;
    srand( ( unsigned )( time( &t ) ) );
    int counter = 0;
    int paltimer = 0;
    int paloffset = 0;
    int pald = 0;
    int pal[ RECTCOUNT ];
    int ids[ RECTCOUNT ];
    float accx[ RECTCOUNT ];
    float accy[ RECTCOUNT ];
    NasrColor c1[ RECTCOUNT ];
    int cdr[ RECTCOUNT ];
    int cdg[ RECTCOUNT ];
    int cdb[ RECTCOUNT ];

    int texture = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR, NASR_INDEXED_NO );
    int tilestext = NasrLoadFileAsTexture( "assets/tilemap.png" );

    NasrRect src = { 0.0f, 0.0f, 1083.0f, 1881.0f };
    NasrRect dest = { 200.0f, 100.0f, 108.3f, 188.1f };
    int nasrinid = NasrGraphicsAddSprite
    (
        0,
        0,
        4,
        texture,
        src,
        dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    );


    const unsigned int tilesw = 48;
    const unsigned int tilesh = 16;
    NasrTile tiles[ tilesw * tilesh ];
    for ( int i = 0; i < tilesw * tilesh; ++i )
    {
        tiles[ i ].animation = 255;
    }
    for ( int i = tilesw * ( tilesh - 1 ); i < tilesw * tilesh; ++i )
    {
        tiles[ i ].x = 0;
        tiles[ i ].y = 1;
        tiles[ i ].palette = 5;
        tiles[ i ].animation = 0;
    }
    for ( int i = tilesw * ( tilesh - 2 ); i < tilesw * ( tilesh - 1 ); ++i )
    {
        tiles[ i ].x = 0;
        tiles[ i ].y = 0;
        tiles[ i ].palette = 5;
        tiles[ i ].animation = 0;
    }
    NasrGraphicsAddTilemap
    (
        0,
        0,
        2,
        tilestext,
        tiles,
        tilesw,
        tilesh
    );

    NasrTile tiles2[ tilesw * tilesh ];
    for ( int i = 0; i < tilesw * tilesh; ++i )
    {
        tiles2[ i ].animation = 255;
    }
    for ( int i = tilesw * ( tilesh - 3 ); i < tilesw * ( tilesh - 2 ); ++i )
    {
        tiles2[ i ].x = i % 3 == 0 ? 11 : 10;
        tiles2[ i ].y = 1;
        tiles2[ i ].palette = 0;
        tiles2[ i ].animation = 0;
    }
    for ( int i = tilesw * ( tilesh - 4 ); i < tilesw * ( tilesh - 3 ); ++i )
    {
        tiles2[ i ].x = i % 3 == 0 ? 11 : 10;
        tiles2[ i ].y = 0;
        tiles2[ i ].palette = 0;
        tiles2[ i ].animation = 0;
    }
    NasrGraphicsAddTilemap
    (
        0,
        0,
        2,
        tilestext,
        tiles2,
        tilesw,
        tilesh
    );

    NasrTile tiles3[ tilesw * tilesh ];
    for ( int i = 0; i < tilesw * tilesh; ++i )
    {
        tiles3[ i ].animation = 255;
    }
    for ( int i = tilesw * ( tilesh - 4 ) + 5; i < tilesw * ( tilesh - 3 ) - 28; ++i )
    {
        tiles3[ i ].x = 5;
        tiles3[ i ].y = 0;
        tiles3[ i ].palette = 0;
        tiles3[ i ].animation = 0;
    }
    NasrGraphicsAddTilemap
    (
        0,
        0,
        3,
        tilestext,
        tiles3,
        tilesw,
        tilesh
    );

    for ( int i = 0; i < RECTCOUNT; ++i )
    {
        NasrRect tilesdest = { ( i % 16 ) * 32.0f + 16.0f, floor( i / 16 ) * 32.0f + 16.0f, 16.0f, 16.0f };
        NasrRect tilessrc = { 16.0f * ( rand() % 12 ), 16.0f * ( rand() % 2 ), 16.0f, 16.0f };
        c1[ i ].r = ( i % 8 ) * 32.0f;
        c1[ i ].g = floor( i / 8 ) * 32.0f;
        c1[ i ].b = 255.0f;
        c1[ i ].a = 255.0f;
        //NasrColor c2 = {  255.0f, ( i % 8 ) * 32.0f, floor( i / 8 ) * 32.0f, 255.0f };
        int d = rand() % 8;
        pal[ i ] = ( rand() % 5 ) * 5;
        ids[ i ] = NasrGraphicsAddSprite
        (
            0,
            0,
            0,
            tilestext,
            tilessrc,
            tilesdest,
            0,
            0,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            pal[ i ]
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
                NasrGraphicsSpriteAddToDestX( ids[ i ], accx[ i ] );
                NasrGraphicsSpriteAddToDestY( ids[ i ], accy[ i ] );
                /*
                const float rchange = cdr[ i ] ? 1.0f : -1.0f;
                c1[ i ].r += rchange;
                if ( c1[ i ].r > 255.0f ) {
                    cdr[ i ] = 0;
                }
                else if ( c1[ i ].r < 0.0f ) {
                    cdr[ i ] = 1;
                }
                const float gchange = cdg[ i ] ? 2.0f : -2.0f;
                c1[ i ].g += gchange;
                if ( c1[ i ].g > 255.0f ) {
                    cdg[ i ] = 0;
                }
                else if ( c1[ i ].g < 0.0f ) {
                    cdg[ i ] = 1;
                }
                const float bchange = cdb[ i ] ? 0.5f : -0.5f;
                c1[ i ].b += bchange;
                if ( c1[ i ].b > 255.0f ) {
                    cdb[ i ] = 0;
                }
                else if ( c1[ i ].b < 0.0f ) {
                    cdb[ i ] = 1;
                }
                NasrGraphicRectSetColor( ids[ i ], c1[ i ] );*/
                if ( paltimer == 31 || paltimer == 7 )
                {
                    NasrGraphicsSpriteSetPalette( ids[ i ], pal[ i ] + paloffset );
                }

                if ( NasrGraphicsSpriteGetDestX( ids[ i ] ) > 520.0f )
                {
                    accx[ i ] = -SPEED;
                }
                else if ( NasrGraphicsSpriteGetDestX( ids[ i ] ) < 0.0f )
                {
                    accx[ i ] = SPEED;
                }
                if ( NasrGraphicsSpriteGetDestY( ids[ i ] ) > 320.0f )
                {
                    accy[ i ] = -SPEED;
                }
                else if ( NasrGraphicsSpriteGetDestY( ids[ i ] ) < 0.0f )
                {
                    accy[ i ] = SPEED;
                }
            }

            if ( NasrHeld( INPUT_UP ) )
            {
                NasrGraphicsSpriteSetDestY( nasrinid, NasrGraphicsSpriteGetDestY( nasrinid ) - NASRSPEED );
            }
            else if ( NasrHeld( INPUT_DOWN ) )
            {
                NasrGraphicsSpriteSetDestY( nasrinid, NasrGraphicsSpriteGetDestY( nasrinid ) + NASRSPEED );
            }

            if ( NasrHeld( INPUT_LEFT ) )
            {
                NasrGraphicsSpriteSetDestX( nasrinid, NasrGraphicsSpriteGetDestX( nasrinid ) - NASRSPEED );
            }
            else if ( NasrHeld( INPUT_RIGHT ) )
            {
                NasrGraphicsSpriteSetDestX( nasrinid, NasrGraphicsSpriteGetDestX( nasrinid ) + NASRSPEED );
            }

            if ( NasrHeld( INPUT_X ) )
            {
                NasrGraphicsSpriteSetRotationX( nasrinid, NasrGraphicsSpriteGetRotationX( nasrinid ) + NASRSPEED );
            }

            if ( NasrHeld( INPUT_Z ) )
            {
                NasrGraphicsSpriteSetRotationZ( nasrinid, NasrGraphicsSpriteGetRotationZ( nasrinid ) + NASRSPEED );
            }

            if ( NasrHeld( INPUT_Y ) )
            {
                NasrGraphicsSpriteSetRotationY( nasrinid, NasrGraphicsSpriteGetRotationY( nasrinid ) + NASRSPEED );
            }

            if ( NasrHeld( INPUT_F ) )
            {
                if ( counter == 0 )
                {
                    NasrGraphicsSpriteFlipX( nasrinid );
                    counter = 32;
                }
            }

            if ( NasrHeld( INPUT_G ) )
            {
                if ( counter == 0 )
                {
                    NasrGraphicsSpriteFlipY( nasrinid );
                    counter = 32;
                }
            }

            if ( NasrHeld( INPUT_O ) )
            {
                if ( NasrHeld( INPUT_PLUS ) )
                {
                    NasrGraphicsSpriteSetOpacity( nasrinid, NasrGraphicsSpriteGetOpacity( nasrinid ) + 0.005f );
                }
                else if ( NasrHeld( INPUT_MINUS ) )
                {
                    NasrGraphicsSpriteSetOpacity( nasrinid, NasrGraphicsSpriteGetOpacity( nasrinid ) - 0.005f );
                }
            }

            if ( counter > 0 )
            {
                --counter;
            }

            if ( paltimer == 0 )
            {
                if ( pald )
                {
                    --paloffset;
                    if ( paloffset == 0 )
                    {
                        pald = 0;
                        paltimer = 31;
                    }
                    else
                    {
                        paltimer = 7;
                    }
                }
                else
                {
                    ++paloffset;
                    if ( paloffset == 4 )
                    {
                        pald = 1;
                        paltimer = 31;
                    }
                    else
                    {
                        paltimer = 7;
                    }
                }
            }
            else
            {
                --paltimer;
            }
            NasrUpdate();
        }
    }
    NasrClose();
};