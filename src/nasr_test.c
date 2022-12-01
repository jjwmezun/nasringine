#include "nasr.h"
#include "nasr_test.h"

static int running = 1;
static int lock = 0;

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

    const NasrRect rainbow_rect = { 0.0f, 0.0f, 520.0f, 300.0f };
    //int rainbow = NasrAddTextureBlank( rainbow_rect.w, rainbow_rect.h );
    //NasrSetTextureAsTarget( rainbow );
    const NasrRect canvas_rect1 = { 0.0f, 0.0f, 520.0f, 50.0f };
    const NasrRect canvas_rect2 = { 0.0f, 50.0f, 520.0f, 50.0f };
    const NasrRect canvas_rect3 = { 0.0f, 100.0f, 520.0f, 50.0f };
    const NasrRect canvas_rect4 = { 0.0f, 150.0f, 520.0f, 50.0f };
    const NasrRect canvas_rect5 = { 0.0f, 200.0f, 520.0f, 50.0f };
    const NasrRect canvas_rect6 = { 0.0f, 250.0f, 520.0f, 50.0f };
    const NasrColor canvas_color_1 = { 255.0f, 0.0f, 0.0f, 255.0f };
    const NasrColor canvas_color_2 = { 255.0f, 255.0f, 0.0f, 255.0f };
    const NasrColor canvas_color_3 = { 0.0f, 255.0f, 0.0f, 255.0f };
    const NasrColor canvas_color_4 = { 0.0f, 255.0f, 255.0f, 255.0f };
    const NasrColor canvas_color_5 = { 0.0f, 0.0f, 255.0f, 255.0f };
    const NasrColor canvas_color_6 = { 255.0f, 0.0f, 255.0f, 255.0f };
    /*
    NasrDrawGradientRectToTexture( canvas_rect1, NASR_DIR_DOWN, canvas_color_1, canvas_color_2 );
    NasrDrawGradientRectToTexture( canvas_rect2, NASR_DIR_DOWN, canvas_color_2, canvas_color_3 );
    NasrDrawGradientRectToTexture( canvas_rect3, NASR_DIR_DOWN, canvas_color_3, canvas_color_4 );
    NasrDrawGradientRectToTexture( canvas_rect4, NASR_DIR_DOWN, canvas_color_4, canvas_color_5 );
    NasrDrawGradientRectToTexture( canvas_rect5, NASR_DIR_DOWN, canvas_color_5, canvas_color_6 );
    NasrDrawGradientRectToTexture( canvas_rect6, NASR_DIR_DOWN, canvas_color_6, canvas_color_1 );
    */
    //NasrReleaseTextureTarget();
/*
    NasrGraphicsAddSprite
    (
        0,
        0,
        rainbow,
        rainbow_rect,
        rainbow_rect,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );*/

    NasrGraphicsAddRectGradient
    (
        0,
        0,
        0,
        canvas_rect1,
        NASR_DIR_DOWN,
        canvas_color_1,
        canvas_color_2
    );
    NasrGraphicsAddRectGradient
    (
        0,
        0,
        0,
        canvas_rect2,
        NASR_DIR_DOWN,
        canvas_color_2,
        canvas_color_3
    );
    NasrGraphicsAddRectGradient
    (
        0,
        0,
        0,
        canvas_rect3,
        NASR_DIR_DOWN,
        canvas_color_3,
        canvas_color_4
    );
    NasrGraphicsAddRectGradient
    (
        0,
        0,
        0,
        canvas_rect4,
        NASR_DIR_DOWN,
        canvas_color_4,
        canvas_color_5
    );
    NasrGraphicsAddRectGradient
    (
        0,
        0,
        0,
        canvas_rect5,
        NASR_DIR_DOWN,
        canvas_color_5,
        canvas_color_6
    );
    NasrGraphicsAddRectGradient
    (
        0,
        0,
        0,
        canvas_rect6,
        NASR_DIR_DOWN,
        canvas_color_6,
        canvas_color_1
    );


    int tilemap_texture = NasrLoadFileAsTextureEx( "assets/tilemap.png", NASR_SAMPLING_NEAREST, NASR_INDEXED_YES );
    /*
    NasrRect tiledest = { 0.0f, 0.0f, 256.0f, 48.0f };
    NasrGraphicsAddSprite
    (
        0,
        1,
        3,
        tilemap_texture,
        tiledest,
        tiledest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        128
    );*/
    unsigned int tilesw = 70;
    unsigned int tilesh = 16;
    NasrTile * tiles = calloc( tilesw * tilesh, sizeof( NasrTile ) );
    for ( int i = 0; i < tilesw * tilesh; ++i ) {
        tiles[ i ].animation = 255;
    }
    const int i1 = tilesw * ( tilesh - 1 );
    for ( int i = 0; i < tilesw; ++i )
    {
        tiles[ i1 + i ].y = 1;
        tiles[ i1 + i ].x = 0;
        tiles[ i1 + i ].palette = 128;
        tiles[ i1 + i ].animation = 0;
    }
    const int i2 = tilesw * ( tilesh - 2 );
    for ( int i = 0; i < tilesw; ++i )
    {
        tiles[ i2 + i ].y = 0;
        tiles[ i2 + i ].x = 0;
        tiles[ i2 + i ].palette = 128;
        tiles[ i2 + i ].animation = 0;
    }
    int tilemap_id = NasrGraphicsAddTilemap( 0, 1, 3, tilemap_texture, tiles, tilesw, tilesh );

    /*
    NasrRect tilemaptestdest = { 0.0f, 0.0f, 70.0f, 32.0f };
    NasrGraphicsAddSprite
    (
        1,
        3,
        0,
        1,
        tilemaptestdest,
        tilemaptestdest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    );*/

    int nasrin_texture1 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR, NASR_INDEXED_NO );
    int nasrin_texture2 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR, NASR_INDEXED_NO );
    int autumn_texture = NasrLoadFileAsTexture( "assets/autumn.png" );
    //printf( "%d, %d, %d\n", nasrin_texture1, nasrin_texture2, autumn_texture );
    
    const unsigned int texwidth = 8;
    const unsigned int texheight = 8;
    const int bricksize = 256;
    uint32_t texdata[ texwidth * texheight ];
    for ( int i = 0; i < texwidth * texheight; ++i )
    {
        const int x = i % texwidth;
        const int y = floor( i / texwidth );
        if ( y % 4 == 0 || ( y < 4 && x == 0 ) || ( y >= 4 && x == 4 ) )
        {
            texdata[ i ] = 0xFF000000;
        }
        else {
            texdata[ i ] = 0xFF0c4cc8;
        }
    }
    const int texture = NasrAddTextureEx( texdata, texwidth, texheight, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    int blank_board = NasrAddTextureBlankEx( 256, 256, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    NasrSetTextureAsTarget( blank_board );
    NasrRect rect = { 0.0f, 0.0f, 256.0f, 256.0f };
    NasrColor color = { 255.0f, 0.0f, 255.0f, 255.0f };
    NasrDrawRectToTexture( rect, color );

    NasrRect src = { 0.0f, 0.0f, 8.0f, 8.0f };
    NasrRect dest = { 0.0f, 0.0f, 8.0f, 8.0f };
    for ( float x = 0.0f; x < 256.0f; x += 8.0f )
    {
        dest.x = x;
        for ( float y = 0.0f; y < 256.0f; y += 8.0f )
        {
            dest.y = y;
            NasrDrawSpriteToTexture(
                texture,
                src,
                dest,
                0,
                0,
                0.0,
                0.0,
                0.0,
                1.0
            );
        }
    }
    NasrReleaseTextureTarget();

    const NasrRect bsrc = { 0.0f, 0.0f, ( float )( bricksize ), ( float )( bricksize ) };
    const NasrRect bdest = { 0.0f, 0.0f, ( float )( bricksize ), ( float )( bricksize ) };
    const int bid = NasrGraphicsAddSprite
    (
        0,
        0,
        16,
        blank_board,
        bsrc,
        bdest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    );

    nasrin_texture1 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR, NASR_INDEXED_NO );
    nasrin_texture2 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR, NASR_INDEXED_NO );
    autumn_texture = NasrLoadFileAsTexture( "assets/autumn.png" );
    const NasrRect nasrin_src = { 0.0f, 0.0f, 1083.0f, 1881.0f };
    NasrRect nasrin_dest = { 200.0f, 32.0f, 54.15f, 94.05f };
    const int nasrinid = NasrGraphicsAddSprite
    (
        0,
        0,
        14,
        nasrin_texture1,
        nasrin_src,
        nasrin_dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    );

    nasrin_dest.x = 300.0f;
    nasrin_dest.y = 180.0f;

    const int nasrinid2 = NasrGraphicsAddSprite
    (
        0,
        0,
        14,
        nasrin_texture1,
        nasrin_src,
        nasrin_dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    );

    const int dawn_texture = NasrLoadFileAsTextureEx( "assets/dawn-summers-2021.png", NASR_SAMPLING_LINEAR, NASR_INDEXED_NO );
    const NasrRect dawn_src = { 0.0f, 0.0f, 306.f, 535.0f };
    const NasrRect dawn_dest = { 300.0f, 32.0f, dawn_src.w / 5.0f, dawn_src.h / 5.0f };
    const int dawnid = NasrGraphicsAddSprite
    (
        0,
        0,
        14,
        dawn_texture,
        dawn_src,
        dawn_dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0
    );  

    const NasrRect autumn_src = { 0.0f, 0.0f, 16.0f, 25.0f };
    const NasrRect autumn_dest = { 200.0f, 32.0f, 16.0f, 25.0f };
    const int autumnid = NasrGraphicsAddSprite
    (
        0,
        1,
        2,
        autumn_texture,
        autumn_src,
        autumn_dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        128
    );

    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {
            NasrGraphic * gfx = NasrGraphicGet( nasrinid );
            NasrGraphicSprite * sprite = &gfx->data.sprite;
            NasrRect * dest = &sprite->dest;
            if ( NasrHeld( INPUT_RIGHT ) )
            {
                dest->x += 1.0f;
                if ( NasrGetLayer( nasrinid ) == NasrGetLayer( nasrinid2 ) )
                {
                    NasrPlaceGraphicAbovePositionInLayer( nasrinid, NasrGetLayerPosition( nasrinid2 ) );
                }
            }
            else if ( NasrHeld( INPUT_LEFT ) )
            {
                dest->x -= 1.0f;
                if ( NasrGetLayer( nasrinid ) == NasrGetLayer( dawnid ) )
                {
                    NasrPlaceGraphicBelowPositionInLayer( nasrinid, NasrGetLayerPosition( nasrinid2 ) );
                }
            }
            if ( NasrHeld( INPUT_DOWN ) )
            {
                dest->y += 1.0f;
                NasrGraphicChangeLayer( nasrinid, 14 );
            }
            else if ( NasrHeld( INPUT_UP ) )
            {
                dest->y -= 1.0f;
                NasrGraphicChangeLayer( nasrinid, 17 );
            }

            if ( NasrHeld( INPUT_X ) )
            {
                sprite->rotation_x += 4.0f;
            }
            if ( NasrHeld( INPUT_Z ) )
            {
                NasrSendGraphicToBackOLayer( nasrinid );
            }
            if ( NasrHeld( INPUT_Y ) )
            {
                NasrSendGraphicToFrontOLayer( nasrinid );
            }
            if ( NasrHeld( INPUT_A ) )
            {
                if ( lock == 0 )
                {
                    NasrRaiseGraphicBackwardInLayer( nasrinid );
                    lock = 16;
                }
            }
            else if ( NasrHeld( INPUT_S ) )
            {
                if ( lock == 0 )
                {
                    NasrRaiseGraphicForwardInLayer( nasrinid );
                    lock = 16;
                }
            }
            gfx = NasrGraphicGet( nasrinid );
            sprite = &gfx->data.sprite;
            dest = &sprite->dest;

            if ( lock > 0 )
            {
                --lock;
            }

            NasrAdjustCamera( dest, 6000.0f, 640.0f );
            NasrUpdate();
        }
    }
    NasrClose();
};