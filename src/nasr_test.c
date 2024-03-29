#include "nasr.h"
#include "nasr_audio.h"
#include "nasr_input.h"
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
    INPUT_G,
    INPUT_1,
    INPUT_2,
    INPUT_3,
    INPUT_M
} Input;

static float total_dt = 0.0f;
static int dtcount = 0;

static int othernas[ 100 ];

void NasrTestRun( void )
{
    NasrInit( "Nasringine 0.1", 520, 320, 5, 128, 128, 18, NASR_SAMPLING_NEAREST, NASR_INDEXED_YES, 0, 8 );
    NasrAudioInit( 256, 16, 16 );
    NasrSetPalette( "assets/palette2.png" );
    const int charset1 = NasrAddCharset( "assets/latin1.png", "assets/latin1.json" );
    const int charset2 = NasrAddCharset( "assets/latin2.png", "assets/latin2.json" );
    NasrSetLanguage( "assets/es.json", "nasringine" );
    
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
        { INPUT_G, NASR_KEY_G },
        { INPUT_1, NASR_KEY_1 },
        { INPUT_2, NASR_KEY_2 },
        { INPUT_3, NASR_KEY_3 },
        { INPUT_M, NASR_KEY_M }
    };
    NasrRegisterInputs( inputs, 19 );

    NasrRect backbox = { 0.0f, 0.0f, 520.0f, 320.0f };
    NasrGraphicsAddRectGradientPalette
    (
        1.0f,
        0.0f,
        0,
        0,
        backbox,
        60,
        NASR_DIR_DOWN,
        255,
        1,
        1,
        0.5f
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

    /*
    const NasrRect s = { 0.0f, 0.0f, 16.0f, 25.0f };
    for ( int i = 1; i <= 500; ++i )
    {
        char f[24];
        for ( int j = 0; j < 24; ++j )
        {
            f[ j ] = '\0';
        }
        sprintf( f, "assets/autumn%d.png", i );
        NasrLoadFileAsTexture( f );
    }
    for ( int i = 1; i <= 500; ++i )
    {
        char f[24];
        for ( int j = 0; j < 24; ++j )
        {
            f[ j ] = '\0';
        }
        sprintf( f, "assets/autumn%d.png", i );
        const int t = NasrLoadFileAsTexture( f );
        const NasrRect d = { ( i % 50 ) * 20.0f, ( i / 50 ) * 30.0f, 16.0f, 25.0f };
        NasrGraphicsAddSprite
        (
            0,
            0,
            3,
            t,
            s,
            d,
            0,
            0,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0,
            0
        );
    }*/

    NasrRect src = { 0.0f, 0.0f, 1083.0f, 1881.0f };
    NasrRect dest = { 200.0f, 100.0f, 54.15f, 94.05f };
    int nasrinid = NasrGraphicsAddSprite
    (
        0.0f,
        0.0f,
        0,
        3,
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
        0,
        1.0f,
        1.0f
    );


    int window_text = NasrLoadFileAsTextureEx( "assets/window.png", NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    NasrRect window_src = { 0.0f, 0.0f, 48.0f, 48.0f };
    NasrRect window_dest = { 0.0f, 64.0f, 96.0f, 96.0f };
    NasrGraphicsAddSprite
    (
        0.5f,
        1.0f,
        0,
        2,
        window_text,
        window_src,
        window_dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0,
        0,
        2.0f,
        2.0f
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
        tiles[ i ].palette = 100;
        tiles[ i ].animation = 0;
    }
    for ( int i = tilesw * ( tilesh - 2 ); i < tilesw * ( tilesh - 1 ); ++i )
    {
        tiles[ i ].x = 0;
        tiles[ i ].y = 0;
        tiles[ i ].palette = 200;
        tiles[ i ].animation = 0;
    }
    const int tilemap1 = NasrGraphicsAddTilemap
    (
        0.0f,
        0.0f,
        0,
        2,
        tilestext,
        tiles,
        tilesw,
        tilesh,
        0,
        0.5f,
        1.0f,
        1.0f
    );

    NasrTile till = { 5, 1, 150, 2 };
    NasrGraphicsTilemapSetTile( tilemap1, 5, 1, till );
    NasrGraphicsTilemapClearTile( tilemap1, 5, tilesh - 1 );

    NasrTile tiles2[ 8 ] =
    {
        { 10, 0, 0, 0 },
        { 11, 0, 0, 0 },
        { 10, 0, 0, 0 },
        { 10, 0, 0, 0 },
        { 10, 1, 0, 0 },
        { 11, 1, 0, 0 },
        { 10, 1, 0, 0 },
        { 10, 1, 0, 0 }
    };
    const int tilemap2 = NasrGraphicsAddTilemap
    (
        0.5f,
        0.0f,
        0,
        2,
        tilestext,
        tiles2,
        4,
        2,
        1,
        1.0f,
        2.0f,
        1.0f
    );
    NasrGraphicsTilemapSetY( tilemap2, 192.0f );

    NasrTile tiles3[ tilesw * tilesh ];
    for ( int i = 0; i < tilesw * tilesh; ++i )
    {
        tiles3[ i ].animation = 255;
    }
    for ( int i = tilesw * ( tilesh - 4 ) + 5; i < tilesw * ( tilesh - 3 ) - 28; ++i )
    {
        tiles3[ i ].x = 0;
        tiles3[ i ].y = 0;
        tiles3[ i ].palette = 0;
        tiles3[ i ].animation = 6;
    }
    NasrGraphicsAddTilemap
    (
        1.0f,
        0.0f,
        0,
        3,
        NasrLoadFileAsTexture( "assets/universal.png" ),
        tiles3,
        tilesw,
        tilesh,
        0,
        1.0f,
        1.0f,
        1.0f
    );

    const board = NasrAddTextureBlankEx( 400, 200, NASR_SAMPLING_NEAREST, NASR_INDEXED_YES );
    NasrSetTextureAsTarget( board );
    const NasrRect boardrect = { 0.0f, 0.0f, 400.0f, 200.0f };
    const NasrRect boardrect2 = { 16.0f, 16.0f, 400.0f - 32.0f, 200.0f - 32.0f };
    const NasrColor boardcolor = { 32.0f, 0.0f, 16.0f, 255.0f };
    const NasrColor boardcolor2 = { 255.0f, 180.0f, 16.0f, 255.0f };
    const NasrColor boardcolor3 = { 0.0f, 180.0f, 255.0f, 255.0f };
    NasrDrawRectToTexture( boardrect, boardcolor );
    NasrDrawGradientRectToTexture( boardrect2, NASR_DIR_DOWN, boardcolor2, boardcolor3 );

    const NasrRect boardtilesrc = { 32.0f, 16.0f, 16.0f, 16.0f };
    NasrRect boardtiledest = { 0.0f, 0.0f, 16.0f, 16.0f };
    for ( int y = 0; y < 8; ++y )
    {
        boardtiledest.y = 16.0f + y * 16.0f;
        for ( int x = 0; x < 8; ++x )
        {
            boardtiledest.x = 16.0f + x * 16.0f;
            NasrDrawSpriteToTexture
            (
                tilestext,
                boardtilesrc,
                boardtiledest,
                0,
                0,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                0,
                0,
                1.0f,
                1.0f
            );
        }
    }

    NasrReleaseTextureTarget();

    const NasrRect boarddest = { 32.0f, 32.0f, 400.0f, 200.0f };
    /*
    NasrGraphicsAddSprite
    (
        1.0f,
        0.0f,
        4,
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
        1,
        1.0f,
        1.0f
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
            1.0f,
            0.0f,
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
            1,
            1.0f,
            1.0f
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
        0.5f,
        1.0f
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
        0.75f,
        1.0f
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
        0.5f,
        1.0f
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
        0.0f,
        1.0f
    };

    NasrColor texcolor1 = { 255.0f, 16.0f, 64.0f, 255.0f };
    NasrColor texcolor2 = { 64.0f, 16.0f, 255.0f, 255.0f };
    const int textid = NasrGraphicsAddTextGradientPalette(
        1.0f,
        0.0f,
        2,
        0,
        text1,
        200,
        0,
        NASR_DIR_DOWN,
        255,
        1
    );
    const int textid2 = NasrGraphicsAddTextGradient(
        1.0f,
        0.0f,
        2,
        0,
        text2,
        NASR_DIR_DOWN,
        texcolor1,
        texcolor2
    );
    NasrGraphicsTextSetCount( textid2, 0 );
    NasrGraphicsAddTextPalette(
        0.0f,
        0.0f,
        2,
        0,
        text3,
        60,
        1,
        255
    );
    NasrGraphicsAddTextGradient(
        0.0f,
        0.0f,
        2,
        0,
        text4,
        NASR_DIR_DOWN,
        texcolor1,
        texcolor2
    );

    static unsigned char globalpal;
    NasrSetGlobalPalette( globalpal );

    /*
    const NasrRect digit_src = { 9.0f, 0.0f, 7.0f, 8.0f };
    const NasrRect digit_dest[ 3 ] =
    {
        { 8.0f, 8.0f, 7.0f, 8.0f },
        { 16.0f, 8.0f, 7.0f, 8.0f },
        { 24.0f, 8.0f, 7.0f, 8.0f }
    };
    int digits[ 3 ];
    for ( int i = 0; i < 3; ++i )
    {
        digits[ i ] = NasrGraphicsAddSprite
        (
            1,
            4,
            0,
            NasrLoadFileAsTexture( "assets/latin1.png" ),
            digit_src,
            digit_dest[ i ],
            0,
            0,
            0.0f,
            0.0f,
            0.0f,
            255.0f,
            128,
            0
        );
    }*/

    const NasrColor digitcolor1 = { 255.0f, 0.0f, 255.0f, 255.0f };
    const NasrColor digitcolor2 = { 80.0f, 0.0f, 128.0f, 255.0f };

    int citysong = NasrLoadSong( "assets/district4.wav" );
    int sewersong = NasrLoadSong( "assets/retrofuture.wav" );
    int jumpsound = NasrLoadSong( "assets/jump.wav" );
    int gemsound = NasrLoadSong( "assets/gem.wav" );

    int queueid = NasrAddPermanentSoundtoQueue( citysong, 1 );
    NasrPlaySong( queueid );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );

    NasrAudioClear();
    citysong = NasrLoadSong( "assets/retrofuture.wav" );
    sewersong = NasrLoadSong( "assets/retrofuture.wav" );
    jumpsound = NasrLoadSong( "assets/jump.wav" );
    queueid = NasrAddPermanentSoundtoQueue( citysong, 1 );
    //NasrPlaySong( queueid );
    gemsound = NasrLoadSong( "assets/gem.wav" );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );
    NasrAddTemporarySoundtoQueue( gemsound, 0 );
    int diamondsound = NasrLoadSong( "assets/diamond.wav" );

    float naccx = 0.0f;
    float nvx = 0.0f;

    double prev_time = NasrGetTime();
    double current_time = 0;

    int digits = NasrGraphicsAddCounterPaletteGradient
    (
        1.0f,
        0.0f,
        3,
        0,
        charset1,
        88,
        3,
        3,
        0,
        0,
        128,
        NASR_DIR_DOWN,
        200,
        32,
        0,
        16.0f,
        16.0f,
        1.0f,
        1.0f
    );

    digits = NasrGraphicsAddCounterPaletteGradient
    (
        1.0f,
        0.0f,
        3,
        0,
        charset1,
        7456.2368,
        3,
        3,
        0,
        0,
        128,
        NASR_DIR_DOWN,
        200,
        32,
        0,
        16.0f,
        16.0f,
        1.0f,
        1.0f
    );

    NasrGraphicsCounterSetXOffset( digits, 100.0f );
    NasrGraphicsCounterSetYOffset( digits, 20.0f );

    nasrinid = NasrGraphicsAddSprite
    (
        0.0f,
        0.0f,
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
        0,
        1.0f,
        1.0f
    );

    for ( int i = 0; i < 100; ++i )
    {
        othernas[ i ] = NasrGraphicsAddSprite
        (
            0.0f,
            0.0f,
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
            0,
            1.0f,
            1.0f
        );
    }

    int boxi = -1;

    const NasrRect ranr = { 200.0f, 16.0f, 200.0f, 100.0f };
    float rani = 0.1f;
    const int ran = NasrGraphicsAddRectPalette
    (
        1.0f,
        0.0f,
        4,
        12,
        ranr,
        128,
        64,
        0,
        0.1f
    );

    NasrRect clockrect = { 0.0f, 0.0f, 9.0f, 9.0f };
    NasrText clocktext =
    {
        "🕑",
        charset2,
        clockrect,
        NASR_ALIGN_LEFT,
        NASR_VALIGN_TOP,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.5f,
        1.0f
    };
    NasrColor clockcolor = { 128.0f, 32.0f, 255.0f, 255.0f };
    NasrGraphicsAddText
    (
        1.0f,
        0.0f,
        4,
        12,
        clocktext,
        clockcolor
    );

    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {
            current_time = NasrGetTime();
            double timechange = current_time - prev_time;
            double fps = 1.0 / timechange;
            float dt = 60.0f / ( float )( fps );

            total_dt += ( 60.0f / dt );
            ++dtcount;

            NasrHandleEvents();

            /*
            static float digitxes[ 10 ] =
            {
                9.0f,
                17.0f,
                25.0f,
                32.0f,
                41.0f,
                49.0f,
                56.0f,
                65.0f,
                73.0f,
                81.0f
            };

            const int onesd = ( int )( floor( fps ) ) % 10;
            NasrGraphicsSpriteSetSrcX( digits[ 2 ], digitxes[ onesd ] );
            const int tensd = ( int )( floor( ( double )( ( int )( floor( fps ) ) % 100 ) / 10.0 ) );
            NasrGraphicsSpriteSetSrcX( digits[ 1 ], digitxes[ tensd ] );
            const int hunsd = ( int )( floor( ( double )( ( int )( floor( fps ) ) % 1000 ) / 100.0 ) );
            NasrGraphicsSpriteSetSrcX( digits[ 0 ], digitxes[ hunsd ] );

            */

            NasrGraphicsRectPaletteAddToOpacity( ran, 0.1f );

            if ( NasrHeld( INPUT_Z ) )
            {
                /*
                uint_fast8_t d = NasrGraphicsRectGradientGetDir( ran ) + 1;
                if ( d >= 8 )
                {
                    d = 0;
                }*/
                NasrGraphicsRectPaletteSetColor( ran, 3 );
            }
            else if ( NasrPressed( INPUT_Y ) )
            {
                NasrGraphicsRectPaletteSetColor( ran, 244 );
            }

            if ( boxi < 0 && NasrHeld( INPUT_UP ) )
            {
                const boxo = NasrAddTextureBlankEx( 200, 200, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
                NasrSetTextureAsTarget( boxo );
                NasrRect boxor = { 0.0f, 0.0f, 200.0f, 200.0f };
                NasrColor boxoc = { 0.0f, 0.0f, 255.0f, 128.0f };
                NasrDrawRectToTexture( boxor, boxoc );
                NasrRect boxosrc = { 0.0f, 0.0f, 1083.0f, 1881.0f };
                NasrRect boxodest = { 8.0f, 8.0f, 54.15f, 94.05f };
                NasrDrawSpriteToTexture
                (
                    texture,
                    boxosrc,
                    boxodest,
                    0,
                    0,
                    0.0f,
                    0.0f,
                    0.0f,
                    1.0f,
                    0,
                    0,
                    1.0f,
                    1.0f
                );
                NasrReleaseTextureTarget();
                NasrRect boxd = { 0.0f, 0.0f, 200.0f, 200.0f };
                boxi = NasrGraphicsAddSprite
                (
                    1.0f,
                    0.0f,
                    4,
                    0,
                    boxo,
                    boxd,
                    boxd,
                    0,
                    0,
                    0.0f,
                    0.0f,
                    0.0f,
                    1.0f,
                    0,
                    0,
                    1.0f,
                    1.0f
                );

            }
            else if ( NasrHeld( INPUT_DOWN ) )
            {
                dest.w -= 5.0f;
                dest.h -= 5.0f;
            }

            if ( NasrHeld( INPUT_LEFT ) )
            {
                naccx = -0.5f;
            }
            else if ( NasrHeld( INPUT_RIGHT ) )
            {
                naccx = 0.5f;
            }
            else
            {
                naccx = 0.0f;
            }

            nvx += naccx * dt;
            if ( nvx > 4.0f )
            {
                nvx = 4.0f;
            }
            else if ( nvx < -4.0f )
            {
                nvx = -4.0f;
            }

            if ( naccx == 0.0f )
            {
                nvx /= ( 1.0f + 0.2f * dt );
            }

            dest.x += nvx * dt;

            NasrGraphicsSpriteSetDest( nasrinid, dest );
            for ( int i = 0; i < 100; ++i )
            {
                NasrGraphicsSpriteSetDest( othernas[ i ], dest );
            }

            const NasrRect d = NasrGraphicsSpriteGetDest( nasrinid );
            NasrAdjustCamera( &d, 800.0f, 800.0f );

            NasrUpdate( dt );
            NasrInputUpdate();
            prev_time = current_time;
        }
    }

    printf( "%f\n", total_dt / ( float )( dtcount ) );

    NasrCloseLanguage();
    NasrAudioClose();
    NasrInputClose();
    NasrClose();
};