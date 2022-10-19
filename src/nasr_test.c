#include "nasr.h"
#include "nasr_test.h"

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
    NasrInit( "Nasringine 0.1", 520, 320, 1024, 1024, NASR_SAMPLING_NEAREST );
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
        canvas_rect1,
        NASR_DIR_DOWN,
        canvas_color_1,
        canvas_color_2
    );
    NasrGraphicsAddRectGradient
    (
        canvas_rect2,
        NASR_DIR_DOWN,
        canvas_color_2,
        canvas_color_3
    );
    NasrGraphicsAddRectGradient
    (
        canvas_rect3,
        NASR_DIR_DOWN,
        canvas_color_3,
        canvas_color_4
    );
    NasrGraphicsAddRectGradient
    (
        canvas_rect4,
        NASR_DIR_DOWN,
        canvas_color_4,
        canvas_color_5
    );
    NasrGraphicsAddRectGradient
    (
        canvas_rect5,
        NASR_DIR_DOWN,
        canvas_color_5,
        canvas_color_6
    );
    NasrGraphicsAddRectGradient
    (
        canvas_rect6,
        NASR_DIR_DOWN,
        canvas_color_6,
        canvas_color_1
    );


    int nasrin_texture1 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR );
    int nasrin_texture2 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR );
    int autumn_texture = NasrLoadFileAsTexture( "assets/autumn.png" );
    printf( "%d, %d, %d\n", nasrin_texture1, nasrin_texture2, autumn_texture );
    NasrClearTextures();
    
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
    const int texture = NasrAddTexture( texdata, texwidth, texheight );
    int blank_board = NasrAddTextureBlank( 256, 256 );
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
        blank_board,
        bsrc,
        bdest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );

    nasrin_texture1 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR );
    nasrin_texture2 = NasrLoadFileAsTextureEx( "assets/nasrin.png", NASR_SAMPLING_LINEAR );
    autumn_texture = NasrLoadFileAsTexture( "assets/autumn.png" );
    printf( "%d, %d, %d\n", nasrin_texture1, nasrin_texture2, autumn_texture );
    const NasrRect nasrin_src = { 0.0f, 0.0f, 1083.0f, 1881.0f };
    const NasrRect nasrin_dest = { 32.0f, 32.0f, 54.15f, 94.05f };
    const int nasrinid = NasrGraphicsAddSprite
    (
        nasrin_texture1,
        nasrin_src,
        nasrin_dest,
        0,
        0,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );

    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {
            NasrGraphicSprite * sprite = &NasrGraphicGet( nasrinid )->data.sprite;
            NasrRect * dest = &sprite->dest;
            if ( NasrHeld( INPUT_RIGHT ) )
            {
                dest->x += 1.0f;
            }
            else if ( NasrHeld( INPUT_LEFT ) )
            {
                dest->x -= 1.0f;
            }
            if ( NasrHeld( INPUT_DOWN ) )
            {
                dest->y += 1.0f;
            }
            else if ( NasrHeld( INPUT_UP ) )
            {
                dest->y -= 1.0f;
            }

            if ( NasrHeld( INPUT_X ) )
            {
                sprite->rotation_x += 4.0f;
            }
            if ( NasrHeld( INPUT_Z ) )
            {
                sprite->rotation_z += 4.0f;
            }
            if ( NasrHeld( INPUT_Y ) )
            {
                sprite->rotation_y += 4.0f;
            }
            if ( NasrHeld( INPUT_A ) )
            {
                dest->w += 1.0f;
                dest->h += 1.0f;
            }
            else if ( NasrHeld( INPUT_S ) )
            {
                dest->w -= 1.0f;
                dest->h -= 1.0f;
            }
            NasrAdjustCamera( dest, 1024.0f, 640.0f );
            NasrUpdate();
        }
    }
    NasrClose();
};