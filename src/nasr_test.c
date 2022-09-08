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
    NasrInit( "Nasringine 0.1", 520, 320, 1024, 1024 );
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

    const unsigned int texwidth = 8;
    const unsigned int texheight = 8;
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

    const int bricksize = 16;

    /*
    const unsigned int d3[ bricksize * bricksize ];
    const NasrRectInt src3 = { 0, 0, 8, 8 };
    const NasrRectInt dest6 = { 0, 0, bricksize, bricksize };
    NasrTileTexture( texture, d3, src3, dest6 );
    const int blank_board = NasrAddTexture( d3, bricksize, bricksize );
    */

    NasrRectInt texsrc = { 0, 0, 8, 8 };
    NasrRectInt texdest = { 0, 0, 16, 16 };
    const int blank_board = NasrAddTextureBlank( 16, 16 );
    NasrCopyTextureToTexture( texture, blank_board, texsrc, texdest );
    texdest.x = 8;
    texdest.y = 8;
    NasrCopyTextureToTexture( texture, blank_board, texsrc, texdest );

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

    while ( running )
    {
        if ( NasrHasClosed() )
        {
            running = 0;
        }
        else
        {
            NasrGraphicSprite * sprite = &NasrGraphicGet( bid )->data.sprite;
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