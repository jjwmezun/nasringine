#include "nasr.h"
#include "nasr_localization.h"
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
    NasrSetPalette( "assets/palette2.png" );
    const int charset1 = NasrAddCharset( "assets/latin1.png", "assets/latin1.json" );
    const int charset2 = NasrAddCharset( "assets/latin2.png", "assets/latin2.json" );
    NasrSetLanguage( "assets/es.json", "nasringine" );

    printf( "%s\n", Nasr__( "Hello", "nasringine" ) );
    printf( "%s\n", Nasr_x( "cook", "imperative verb", "nasringine" ) );
    printf( "%s\n", Nasr_x( "cook", "person", "nasringine" ) );
    printf( "%s\n", Nasr__( "cook", "nasringine" ) );
    printf( "%s\n", Nasr_n( "Give me %d cat.", "Give me %d cats.", 1, "nasringine" ) );
    printf( "%s\n", Nasr_n( "Give me %d cat.", "Give me %d cats.", 3, "nasringine" ) );
    
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

    NasrRect backbox = { 0.0f, 0.0f, 520.0f, 320.0f };
    NasrGraphicsAddRectGradientPalette
    (
        1,
        0,
        0,
        backbox,
        60,
        NASR_DIR_DOWN,
        255,
        1,
        1
    );

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
    NasrRect dest = { 200.0f, 100.0f, 54.15f, 94.05f };
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
        0,
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
    const int tilemap1 = NasrGraphicsAddTilemap
    (
        1,
        0,
        2,
        tilestext,
        tiles,
        tilesw,
        tilesh,
        1
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
    const int tilemap2 = NasrGraphicsAddTilemap
    (
        1,
        0,
        2,
        tilestext,
        tiles2,
        tilesw,
        tilesh,
        1
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
        1,
        0,
        3,
        tilestext,
        tiles3,
        tilesw,
        tilesh,
        0
    );

    const board = NasrAddTextureBlankEx( 400, 300, NASR_SAMPLING_NEAREST, NASR_INDEXED_YES );
    NasrSetTextureAsTarget( board );
    const NasrRect boardrect = { 0.0f, 0.0f, 400.0f, 300.0f };
    const NasrRect boardrect2 = { 16.0f, 16.0f, 400.0f - 32.0f, 300.0f - 32.0f };
    const NasrColor boardcolor = { 32.0f, 0.0f, 16.0f, 255.0f };
    const NasrColor boardcolor2 = { 255.0f, 180.0f, 16.0f, 255.0f };
    const NasrColor boardcolor3 = { 0.0f, 180.0f, 255.0f, 255.0f };
    NasrDrawRectToTexture( boardrect, boardcolor );
    NasrDrawGradientRectToTexture( boardrect2, NASR_DIR_DOWNRIGHT, boardcolor2, boardcolor3 );

    const NasrRect boardtilesrc = { 32.0f, 16.0f, 16.0f, 16.0f };
    NasrRect boardtiledest = { 0.0f, 0.0f, 16.0f, 16.0f };
    NasrGraphicSprite boardsprite =
    {
        tilestext,
        boardtilesrc,
        boardtiledest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    };
    for ( int y = 0; y < 8; ++y )
    {
        boardsprite.dest.y = 16.0f + y * 16.0f;
        for ( int x = 0; x < 8; ++x )
        {
            boardsprite.dest.x = 16.0f + x * 16.0f;
            NasrDrawSpriteToTexture( boardsprite );
        }
    }

    NasrReleaseTextureTarget();

    const NasrRect boarddest = { 32.0f, 32.0f, 400.0f, 300.0f };
    /*NasrGraphicsAddSprite
    (
        1,
        0,
        0,
        board,
        boardrect,
        boarddest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0,
        1
    );*/

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
            pal[ i ],
            1
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

    #define TILEACCVAL 0.001f
    static float tiley = 32.0f;
    static float tileaccy = TILEACCVAL;
    static float tilevy = 0.0f;

    NasrText text1 =
    {
        "野 As A. awoke 1 morn from uneasy dreams she found herself a target o’ a gang. Every day @ 8 in the morn she was brought home breakfast by Edgar Winters before heading off for his friend’s, but today he didn’t come. That had ne’er happened before. But A. didn’t let it bother her: ’haps he woke late & didn’t want to be late for whate’er excursion he had planned with his friend, or ’haps he had woken earlier than she did & didn’t want to wake her. She was not all that hungry, anyway, & didn’t waste any mo’ time thinking ’bout breakfast but immediately set to work on her laptop, only to be interrupted by a knock on her door.",
        ( unsigned int )( charset1 ),
        { 32.0f, 32.0f, 520.0f - 64.0f, 320.0f - 64.0f - 128.0f },
        NASR_ALIGN_JUSTIFIED,
        NASR_VALIGN_DEFAULT,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,
        0.5f
    };

    NasrText text2 =
    {
        "As A. awoke 1 default morn from uneasy dreams she found herself a target o’ a gang. Every day @ 8 in the morn she was brought home breakfast by Edgar Winters before heading off for his friend’s, but today he didn’t come. That had ne’er happened before. But A. didn’t let it bother her: ’haps he woke late & didn’t want to be late for whate’er excursion he had planned with his friend, or ’haps he had woken earlier than she did & didn’t want to wake her. She was not all that hungry, anyway, & didn’t waste any mo’ time thinking ’bout breakfast but immediately set to work on her laptop, only to be interrupted by a knock on her door.\n\nAs A. awoke 1 morn from uneasy dreams she found herself a target o’ a gang. Every day @ 8 in the morn she was brought home breakfast by Edgar Winters before heading off for his friend’s, but today he didn’t come. That had ne’er happened before. But A. didn’t let it bother her: ’haps he woke late & didn’t want to be late for whate’er excursion he had planned with his friend, or ’haps he had woken earlier than she did & didn’t want to wake her. She was not all that hungry, anyway, & didn’t waste any mo’ time thinking ’bout breakfast but immediately set to work on her laptop, only to be interrupted by a knock on her door.",
        ( unsigned int )( charset2 ),
        { 32.0f, 128.0f, 520.0f - 64.0f, 320.0f - 64.0f - 128.0f },
        NASR_ALIGN_JUSTIFIED,
        NASR_VALIGN_DEFAULT,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,
        0.75f
    };

    NasrText text3 =
    {
        "thru frostgrass\ntwigs & crisp leaves jogs\nlone black crow\n\ntatata\nmarch the frosty streets\ndry warm leaves",
        ( unsigned int )( charset1 ),
        { 32.0f, 256.0f + 128.0f, 520.0f - 64.0f, 320.0f - 64.0f - 128.0f },
        NASR_ALIGN_CENTER,
        NASR_VALIGN_DEFAULT,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,
        0.5f
    };

    NasrText text4 =
    {
        "thru frostgrass\ntwigs & crisp leaves jogs\nlone black crow\n\ntatata\nmarch the frosty streets\ndry warm leaves",
        ( unsigned int )( charset2 ),
        { 32.0f, 512.0f, 520.0f - 64.0f, 320.0f - 64.0f - 128.0f },
        NASR_ALIGN_CENTER,
        NASR_VALIGN_DEFAULT,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,
        0.0f
    };

    NasrColor texcolor1 = { 255.0f, 16.0f, 64.0f, 255.0f };
    NasrColor texcolor2 = { 64.0f, 16.0f, 255.0f, 255.0f };
    const int textid = NasrGraphicAddTextGradientPalette(
        1,
        2,
        0,
        text1,
        200,
        0,
        NASR_DIR_DOWN,
        255,
        1
    );
    const int textid2 = NasrGraphicAddTextGradient(
        0,
        2,
        0,
        text2,
        NASR_DIR_DOWN,
        texcolor1,
        texcolor2
    );
    NasrGraphicTextSetCount( textid2, 0 );
    NasrGraphicAddTextPalette(
        0,
        2,
        0,
        text3,
        60,
        1,
        255
    );
    NasrGraphicAddTextGradient(
        0,
        2,
        0,
        text4,
        NASR_DIR_DOWN,
        texcolor1,
        texcolor2
    );

    static unsigned char globalpal;
    NasrSetGlobalPalette( globalpal );
    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {

            NasrGraphicTextIncrementCount( textid2 );

            tilevy += tileaccy;
            if ( tilevy > 0.1f )
            {
                tilevy = 0.1f;
            }
            else if ( tilevy < -0.1f )
            {
                tilevy = -0.1f;
            }
            tiley += tilevy;

            if ( tiley >= 34.0f )
            {
                tileaccy = -TILEACCVAL;
            }
            else if ( tiley <= 30.0f )
            {
                tileaccy = TILEACCVAL;
            }

            NasrGraphicsTilemapSetY( tilemap1, tiley );
            NasrGraphicsTilemapSetY( tilemap2, tiley );
            NasrGraphicsTilemapSetX( tilemap1, tiley );

            NasrSetGlobalPalette( globalpal );

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
                NasrGraphicRectSetColor( ids[ i ], c1[ i ] );
                if ( paltimer == 31 || paltimer == 7 )
                {
                    NasrGraphicsSpriteSetPalette( ids[ i ], pal[ i ] + paloffset );
                }*/

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


            if ( globalpal == 255 )
            {
                globalpal = 0;
            }
            else
            {
                ++globalpal;
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
            const NasrRect d = NasrGraphicsSpriteGetDest( nasrinid );
            NasrAdjustCamera( &d, 800.0f, 800.0f );
            NasrUpdate();
        }
    }
    NasrClose();
};