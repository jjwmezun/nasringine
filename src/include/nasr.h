#ifndef NASR_H
#define NASR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define NASR_DEBUG 1
#define NASR_SAFE 1

typedef struct NasrColor
{
    float r;
    float g;
    float b;
    float a;
} NasrColor;

typedef struct NasrRect
{
    float x;
    float y;
    float w;
    float h;
} NasrRect;

typedef struct NasrRectInt
{
    int x;
    int y;
    int w;
    int h;
} NasrRectInt;

typedef struct NasrTile
{
    unsigned char x;
    unsigned char y;
    unsigned char palette;
    unsigned char animation;
} NasrTile;

#define NASR_ALIGN_DEFAULT   0
#define NASR_ALIGN_LEFT      1
#define NASR_ALIGN_RIGHT     2
#define NASR_ALIGN_CENTER    3
#define NASR_ALIGN_JUSTIFIED 4

#define NASR_VALIGN_DEFAULT 0
#define NASR_VALIGN_TOP     1
#define NASR_VALIGN_MIDDLE  2
#define NASR_VALIGN_BOTTOM  3

typedef struct NasrText
{
    char * string;
    unsigned int charset;
    NasrRect coords;
    uint_fast8_t align;
    uint_fast8_t valign;
    float padding_left;
    float padding_right;
    float padding_top;
    float padding_bottom;
    float xoffset;
    float yoffset;
    float shadow;
} NasrText;

#define NASR_DIR_UP        0
#define NASR_DIR_UPRIGHT   1
#define NASR_DIR_RIGHT     2
#define NASR_DIR_DOWNRIGHT 3
#define NASR_DIR_DOWN      4
#define NASR_DIR_DOWNLEFT  5
#define NASR_DIR_LEFT      6
#define NASR_DIR_UPLEFT    7

#define NASR_SAMPLING_DEFAULT 0
#define NASR_SAMPLING_NEAREST 1
#define NASR_SAMPLING_LINEAR  2

#define NASR_INDEXED_DEFAULT 0
#define NASR_INDEXED_NO      1
#define NASR_INDEXED_YES     2

typedef void ( * input_handle_t )( void *, int, int, int, int );

// Init, Close, Update
int NasrInit
(
    const char * program_title,
    float canvas_width,
    float canvas_height,
    int init_max_states,
    int init_max_graphics,
    int init_max_textures,
    int init_max_gfx_layers,
    int sample_type,
    int default_indexed_type,
    uint_fast8_t vsync,
    int ticks_per_frame
);
void NasrClose( void );
void NasrUpdate( float dt );
void NasrHandleEvents( void );
int NasrHasClosed( void );

// Input
void NasrRegisterInputHandler( input_handle_t new_handler );

// Rect
float NasrRectRight( const NasrRect * r );
float NasrRectBottom( const NasrRect * r );
int NasrRectEqual( const NasrRect * a, const NasrRect * b );
struct NasrRectInt NasrRectToNasrRectInt( const NasrRect r );

// Palette
void NasrSetPalette( const char * filename );
void NasrSetGlobalPalette( uint_fast8_t palette );

// Charset
int NasrAddCharset( const char * texture, const char * chardata );
void NasrRemoveCharset( unsigned int charset );

// Time
double NasrGetTime( void );

// Camera
void NasrAdjustCamera( struct NasrRect * target, float max_w, float max_h );
void NasrMoveCamera( float x, float y, float max_w, float max_h );
void NasrResetCamera( void );

// Layers
void NasrGraphicChangeLayer( unsigned int id, unsigned int layer );
void NasrSendGraphicToFrontOLayer( unsigned int id );
void NasrSendGraphicToBackOLayer( unsigned int id );
void NasrRaiseGraphicForwardInLayer( unsigned int id );
void NasrRaiseGraphicBackwardInLayer( unsigned int id );
void NasrPlaceGraphicBelowPositionInLayer( unsigned int id, unsigned int pos );
void NasrPlaceGraphicAbovePositionInLayer( unsigned int id, unsigned int pos );
unsigned int NasrGetLayer( unsigned int id );
unsigned int NasrGetLayerPosition( unsigned int id );
unsigned int NasrNumOGraphicsInLayer( unsigned int state, unsigned int layer );

// Graphics
int NasrGraphicsAddCanvas
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    struct NasrColor color
);
int NasrGraphicsAddRect
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    struct NasrColor color
);
int NasrGraphicsAddRectGradient
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    int dir,
    struct NasrColor color1,
    struct NasrColor color2
);
int NasrGraphicsAddRectPalette
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    uint_fast8_t palette,
    uint_fast8_t color,
    uint_fast8_t useglobalpal
);
int NasrGraphicsAddRectGradientPalette
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    uint_fast8_t palette,
    uint_fast8_t dir,
    uint_fast8_t color1,
    uint_fast8_t color2,
    uint_fast8_t useglobalpal
);
int NasrGraphicsAddSprite
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    unsigned int texture,
    NasrRect src,
    NasrRect dest,
    int flip_x,
    int flip_y,
    float rotation_x,
    float rotation_y,
    float rotation_z,
    float opacity,
    uint_fast8_t palette,
    int_fast8_t useglobalpal
);
int NasrGraphicsAddTilemap
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    unsigned int texture,
    const NasrTile * tiles,
    unsigned int w,
    unsigned int h,
    int_fast8_t useglobalpal
);
int NasrGraphicAddText
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    NasrColor color
);
int NasrGraphicAddTextGradient
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    int_fast8_t dir,
    NasrColor color1,
    NasrColor color2
);
int NasrGraphicAddTextPalette
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    uint_fast8_t palette,
    uint_fast8_t useglobalpal,
    uint_fast8_t color
);
int NasrGraphicAddTextGradientPalette
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    uint_fast8_t palette,
    uint_fast8_t useglobalpal,
    int_fast8_t dir,
    uint_fast8_t color1,
    uint_fast8_t color2
);
int NasrGraphicsAddCounter
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    unsigned int charset,
    float num,
    unsigned int maxdigits,
    unsigned int maxdecimals,
    uint_fast8_t numpadding,
    uint_fast8_t decimalpadding,
    NasrColor color,
    float x,
    float y,
    float shadow
);
int NasrGraphicsAddCounterGradient
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    unsigned int charset,
    float num,
    unsigned int maxdigits,
    unsigned int maxdecimals,
    uint_fast8_t numpadding,
    uint_fast8_t decimalpadding,
    uint_fast8_t dir,
    NasrColor color1,
    NasrColor color2,
    float x,
    float y,
    float shadow
);
int NasrGraphicsAddCounterPalette
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    unsigned int charset,
    float num,
    unsigned int maxdigits,
    unsigned int maxdecimals,
    uint_fast8_t numpadding,
    uint_fast8_t decimalpadding,
    uint_fast8_t palette,
    uint_fast8_t color,
    uint_fast8_t useglobalpal,
    float x,
    float y,
    float shadow
);
int NasrGraphicsAddCounterPaletteGradient
(
    uint_fast8_t abs,
    unsigned int state,
    unsigned int layer,
    unsigned int charset,
    float num,
    unsigned int maxdigits,
    unsigned int maxdecimals,
    uint_fast8_t numpadding,
    uint_fast8_t decimalpadding,
    uint_fast8_t palette,
    uint_fast8_t dir,
    uint_fast8_t color1,
    uint_fast8_t color2,
    uint_fast8_t useglobalpal,
    float x,
    float y,
    float shadow
);
void NasrGraphicsRemove( unsigned int id );
void NasrGraphicsClearState( unsigned int state );
void NasrClearGraphics( void );

// NasrGraphicsSprite Manipulation
NasrRect NasrGraphicsSpriteGetDest( unsigned int id );
void NasrGraphicsSpriteSetDest( unsigned int id, NasrRect v );
float NasrGraphicsSpriteGetDestY( unsigned int id );
void NasrGraphicsSpriteSetDestY( unsigned int id, float v );
void NasrGraphicsSpriteAddToDestY( unsigned int id, float v );
float NasrGraphicsSpriteGetDestX( unsigned int id );
void NasrGraphicsSpriteSetDestX( unsigned int id, float v );
void NasrGraphicsSpriteAddToDestX( unsigned int id, float v );
float NasrGraphicsSpriteGetDestH( unsigned int id );
void NasrGraphicsSpriteSetDestH( unsigned int id, float v );
float NasrGraphicsSpriteGetDestW( unsigned int id );
void NasrGraphicsSpriteSetDestW( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcY( unsigned int id );
void NasrGraphicsSpriteSetSrcY( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcX( unsigned int id );
void NasrGraphicsSpriteSetSrcX( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcH( unsigned int id );
void NasrGraphicsSpriteSetSrcH( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcW( unsigned int id );
void NasrGraphicsSpriteSetSrcW( unsigned int id, float v );
float NasrGraphicsSpriteGetRotationX( unsigned int id );
void NasrGraphicsSpriteSetRotationX( unsigned int id, float v );
float NasrGraphicsSpriteGetRotationY( unsigned int id );
void NasrGraphicsSpriteSetRotationY( unsigned int id, float v );
float NasrGraphicsSpriteGetRotationZ( unsigned int id );
void NasrGraphicsSpriteSetRotationZ( unsigned int id, float v );
float NasrGraphicsSpriteGetOpacity( unsigned int id );
void NasrGraphicsSpriteSetOpacity( unsigned int id, float v );
uint_fast8_t NasrGraphicsSpriteGetFlipX( unsigned id );
void NasrGraphicsSpriteSetFlipX( unsigned id, int v );
void NasrGraphicsSpriteFlipX( unsigned id );
uint_fast8_t NasrGraphicsSpriteGetFlipY( unsigned id );
void NasrGraphicsSpriteSetFlipY( unsigned id, int v );
void NasrGraphicsSpriteFlipY( unsigned id );
uint_fast8_t NasrGraphicsSpriteGetPalette( unsigned int id );
void NasrGraphicsSpriteSetPalette( unsigned int id, uint_fast8_t v );

// RectGraphics Manipulation
float NasrGraphicsRectGetX( unsigned int id );
void NasrGraphicsRectSetX( unsigned int id, float v );
void NasrGraphicsRectAddToX( unsigned int id, float v );
float NasrGraphicsRectGetY( unsigned int id );
void NasrGraphicsRectSetY( unsigned int id, float v );
void NasrGraphicsRectAddToY( unsigned int id, float v );
float NasrGraphicsRectGetW( unsigned int id );
void NasrGraphicsRectSetW( unsigned int id, float v );
void NasrGraphicsRectAddToW( unsigned int id, float v );
float NasrGraphicsRectGetH( unsigned int id );
void NasrGraphicsRectSetH( unsigned int id, float v );
void NasrGraphicsRectAddToH( unsigned int id, float v );
void NasrGraphicRectSetColor( unsigned int id, NasrColor v );
void NasrGraphicRectSetColorR( unsigned int id, float v );
void NasrGraphicRectSetColorG( unsigned int id, float v );
void NasrGraphicRectSetColorB( unsigned int id, float v );
void NasrGraphicRectSetColorA( unsigned int id, float v );

// TilemapGraphics Manipulation
void NasrGraphicsTilemapSetX( unsigned int id, float v );
void NasrGraphicsTilemapSetY( unsigned int id, float v );

// TextGraphics Manipulation
float NasrGraphicsTextGetXOffset( unsigned int id );
void NasrGraphicsTextSetXOffset( unsigned int id, float v );
void NasrGraphicsTextAddToXOffset( unsigned int id, float v );
float NasrGraphicsTextGetYOffset( unsigned int id );
void NasrGraphicsTextSetYOffset( unsigned int id, float v );
void NasrGraphicsTextAddToYOffset( unsigned int id, float v );
void NasrGraphicsTextSetCount( unsigned int id, int count );
void NasrGraphicsTextIncrementCount( unsigned int id );

// CounterGraphics Manipulation
void NasrGraphicsCounterSetNumber( unsigned int id, float n );

// Texture
int NasrLoadFileAsTexture( const char * filename );
int NasrLoadFileAsTextureEx( const char * filename, int sampling, int indexed );
int NasrAddTexture( unsigned char * data, unsigned int width, unsigned int height );
int NasrAddTextureEx( unsigned char * data, unsigned int width, unsigned int height, int sampling, int indexed );
int NasrAddTextureBlank( unsigned int width, unsigned int height );
int NasrAddTextureBlankEx( unsigned int width, unsigned int height, int sampling, int indexed );
void NasrGetTexturePixels( unsigned int texture, void * pixels );
void NasrCopyTextureToTexture( unsigned int src, unsigned int dest, NasrRectInt srccoords, NasrRectInt destcoords );
void NasrApplyTextureToPixelData( unsigned int texture, unsigned char * dest, NasrRectInt srccoords, NasrRectInt destcoords );
void NasrCopyPixelData( unsigned char * src, unsigned char * dest, NasrRectInt srccoords, NasrRectInt destcoords, int maxsrcw, int maxsrch );
void NasrTileTexture( unsigned int texture, unsigned char * pixels, NasrRectInt srccoords, NasrRectInt destcoords );
void NasrSetTextureAsTarget( unsigned int texture );
void NasrReleaseTextureTarget( void );
void NasrClearTextures( void );

// Draw to Texture
void NasrDrawRectToTexture( NasrRect rect, NasrColor color );
void NasrDrawGradientRectToTexture( NasrRect rect, int dir, NasrColor color1, NasrColor color2 );
void NasrDrawSpriteToTexture
(
    unsigned int texture,
    NasrRect src,
    NasrRect dest,
    uint_fast8_t flip_x,
    uint_fast8_t flip_y,
    float rotation_x,
    float rotation_y,
    float rotation_z,
    float opacity,
    uint_fast8_t palette,
    int_fast8_t useglobalpal
);

// Debug
void NasrRectPrint( const NasrRect * r );
void NasrColorPrint( const NasrColor * c );
void NasrDebugGraphics( void );

#ifdef __cplusplus
}
#endif

#endif // NASR_H