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
    float opacity;
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
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    struct NasrColor color
);
int NasrGraphicsAddRect
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    struct NasrColor color
);
int NasrGraphicsAddRectGradient
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    int dir,
    struct NasrColor color1,
    struct NasrColor color2
);
int NasrGraphicsAddRectPalette
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    uint_fast8_t palette,
    uint_fast8_t color,
    uint_fast8_t useglobalpal,
    float opacity
);
int NasrGraphicsAddRectGradientPalette
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    uint_fast8_t palette,
    uint_fast8_t dir,
    uint_fast8_t color1,
    uint_fast8_t color2,
    uint_fast8_t useglobalpal,
    float opacity
);
int NasrGraphicsAddSprite
(
    float scrollx,
	float scrolly,
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
    int_fast8_t useglobalpal,
    float tilingx,
    float tilingy
);
int NasrGraphicsAddTilemap
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    unsigned int texture,
    const NasrTile * tiles,
    unsigned int w,
    unsigned int h,
    int_fast8_t useglobalpal,
    float opacity,
    float tilingx,
    float tilingy
);
int NasrGraphicsAddText
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    NasrColor color
);
int NasrGraphicsAddTextGradient
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    int_fast8_t dir,
    NasrColor color1,
    NasrColor color2
);
int NasrGraphicsAddTextPalette
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    uint_fast8_t palette,
    uint_fast8_t useglobalpal,
    uint_fast8_t color
);
int NasrGraphicsAddTextGradientPalette
(
    float scrollx,
	float scrolly,
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
    float scrollx,
	float scrolly,
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
    float shadow,
    float opacity
);
int NasrGraphicsAddCounterGradient
(
    float scrollx,
	float scrolly,
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
    float shadow,
    float opacity
);
int NasrGraphicsAddCounterPalette
(
    float scrollx,
	float scrolly,
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
    float shadow,
    float opacity
);
int NasrGraphicsAddCounterPaletteGradient
(
    float scrollx,
	float scrolly,
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
    float shadow,
    float opacity
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
float NasrGraphicsSpriteGetDestW( unsigned int id );
void NasrGraphicsSpriteSetDestW( unsigned int id, float v );
void NasrGraphicsSpriteAddToDestW( unsigned int id, float v );
float NasrGraphicsSpriteGetDestH( unsigned int id );
void NasrGraphicsSpriteSetDestH( unsigned int id, float v );
void NasrGraphicsSpriteAddToDestH( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcX( unsigned int id );
void NasrGraphicsSpriteSetSrcX( unsigned int id, float v );
void NasrGraphicsSpriteAddToSrcX( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcY( unsigned int id );
void NasrGraphicsSpriteSetSrcY( unsigned int id, float v );
void NasrGraphicsSpriteAddToSrcY( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcW( unsigned int id );
void NasrGraphicsSpriteSetSrcW( unsigned int id, float v );
void NasrGraphicsSpriteAddToSrcW( unsigned int id, float v );
float NasrGraphicsSpriteGetSrcH( unsigned int id );
void NasrGraphicsSpriteSetSrcH( unsigned int id, float v );
void NasrGraphicsSpriteAddToSrcH( unsigned int id, float v );
float NasrGraphicsSpriteGetRotationX( unsigned int id );
void NasrGraphicsSpriteSetRotationX( unsigned int id, float v );
void NasrGraphicsSpriteAddToRotationX( unsigned int id, float v );
float NasrGraphicsSpriteGetRotationY( unsigned int id );
void NasrGraphicsSpriteSetRotationY( unsigned int id, float v );
void NasrGraphicsSpriteAddToRotationY( unsigned int id, float v );
float NasrGraphicsSpriteGetRotationZ( unsigned int id );
void NasrGraphicsSpriteSetRotationZ( unsigned int id, float v );
void NasrGraphicsSpriteAddToRotationZ( unsigned int id, float v );
uint_fast8_t NasrGraphicsSpriteGetPalette( unsigned int id );
void NasrGraphicsSpriteSetPalette( unsigned int id, uint_fast8_t v );
void NasrGraphicsSpriteIncrementPalette( unsigned int id );
void NasrGraphicsSpriteDecrementPalette( unsigned int id );
float NasrGraphicsSpriteGetOpacity( unsigned int id );
void NasrGraphicsSpriteSetOpacity( unsigned int id, float v );
void NasrGraphicsSpriteAddToOpacity( unsigned int id, float v );
uint_fast8_t NasrGraphicsSpriteGetFlipX( unsigned id );
void NasrGraphicsSpriteSetFlipX( unsigned id, int v );
void NasrGraphicsSpriteFlipX( unsigned id );
uint_fast8_t NasrGraphicsSpriteGetFlipY( unsigned id );
void NasrGraphicsSpriteSetFlipY( unsigned id, int v );
void NasrGraphicsSpriteFlipY( unsigned id );
unsigned int NasrGraphicsSpriteGetTexture( unsigned id );
void NasrGraphicsSpriteSetTexture( unsigned id, unsigned int v );

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
void NasrGraphicsRectSetColor( unsigned int id, NasrColor v );
void NasrGraphicsRectSetColorR( unsigned int id, float v );
void NasrGraphicsRectSetColorG( unsigned int id, float v );
void NasrGraphicsRectSetColorB( unsigned int id, float v );
void NasrGraphicsRectSetColorA( unsigned int id, float v );

// RectGraphicsGradient Manipulation
float NasrGraphicsRectGradientGetX( unsigned int id );
void NasrGraphicsRectGradientSetX( unsigned int id, float v );
void NasrGraphicsRectGradientAddToX( unsigned int id, float v );
float NasrGraphicsRectGradientGetY( unsigned int id );
void NasrGraphicsRectGradientSetY( unsigned int id, float v );
void NasrGraphicsRectGradientAddToY( unsigned int id, float v );
float NasrGraphicsRectGradientGetW( unsigned int id );
void NasrGraphicsRectGradientSetW( unsigned int id, float v );
void NasrGraphicsRectGradientAddToW( unsigned int id, float v );
float NasrGraphicsRectGradientGetH( unsigned int id );
void NasrGraphicsRectGradientSetH( unsigned int id, float v );
void NasrGraphicsRectGradientAddToH( unsigned int id, float v );
uint_fast8_t NasrGraphicsRectGradientGetDir( unsigned int id );
void NasrGraphicsRectGradientSetDir( unsigned int id, uint_fast8_t dir );
void NasrGraphicsRectGradientSetColor1( unsigned int id, NasrColor color );
void NasrGraphicsRectGradientSetColor1R( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor1G( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor1B( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor1A( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor2( unsigned int id, NasrColor color );
void NasrGraphicsRectGradientSetColor2R( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor2G( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor2B( unsigned int id, float v );
void NasrGraphicsRectGradientSetColor2A( unsigned int id, float v );

// RectGraphicsPalette Manipulation
float NasrGraphicsRectPaletteGetX( unsigned int id );
void NasrGraphicsRectPaletteSetX( unsigned int id, float v );
void NasrGraphicsRectPaletteAddToX( unsigned int id, float v );
float NasrGraphicsRectPaletteGetY( unsigned int id );
void NasrGraphicsRectPaletteSetY( unsigned int id, float v );
void NasrGraphicsRectPaletteAddToY( unsigned int id, float v );
float NasrGraphicsRectPaletteGetW( unsigned int id );
void NasrGraphicsRectPaletteSetW( unsigned int id, float v );
void NasrGraphicsRectPaletteAddToW( unsigned int id, float v );
float NasrGraphicsRectPaletteGetH( unsigned int id );
void NasrGraphicsRectPaletteSetH( unsigned int id, float v );
void NasrGraphicsRectPaletteAddToH( unsigned int id, float v );
uint_fast8_t NasrGraphicsRectPaletteGetPalette( unsigned int id );
void NasrGraphicsRectPaletteSetPalette( unsigned int id, uint_fast8_t v );
void NasrGraphicsRectPaletteIncrementPalette( unsigned int id );
void NasrGraphicsRectPaletteDecrementPalette( unsigned int id );
uint_fast8_t NasrGraphicsRectPaletteGetColor( unsigned int id );
void NasrGraphicsRectPaletteSetColor( unsigned int id, uint_fast8_t v );
void NasrGraphicsRectPaletteIncrementColor( unsigned int id );
void NasrGraphicsRectPaletteDecrementColor( unsigned int id );
float NasrGraphicsRectPaletteGetOpacity( unsigned int id );
void NasrGraphicsRectPaletteSetOpacity( unsigned int id, float v );
void NasrGraphicsRectPaletteAddToOpacity( unsigned int id, float v );

// RectGraphicsGradientPalette Manipulation
float NasrGraphicsRectGradientPaletteGetX( unsigned int id );
void NasrGraphicsRectGradientPaletteSetX( unsigned int id, float v );
void NasrGraphicsRectGradientPaletteAddToX( unsigned int id, float v );
float NasrGraphicsRectGradientPaletteGetY( unsigned int id );
void NasrGraphicsRectGradientPaletteSetY( unsigned int id, float v );
void NasrGraphicsRectGradientPaletteAddToY( unsigned int id, float v );
float NasrGraphicsRectGradientPaletteGetW( unsigned int id );
void NasrGraphicsRectGradientPaletteSetW( unsigned int id, float v );
void NasrGraphicsRectGradientPaletteAddToW( unsigned int id, float v );
float NasrGraphicsRectGradientPaletteGetH( unsigned int id );
void NasrGraphicsRectGradientPaletteSetH( unsigned int id, float v );
void NasrGraphicsRectGradientPaletteAddToH( unsigned int id, float v );
uint_fast8_t NasrGraphicsRectGradientPaletteGetDir( unsigned int id );
void NasrGraphicsRectGradientPaletteSetDir( unsigned int id, uint_fast8_t v );
uint_fast8_t NasrGraphicsRectGradientPaletteGetPalette( unsigned int id );
void NasrGraphicsRectGradientPaletteSetPalette( unsigned int id, uint_fast8_t v );
void NasrGraphicsRectGradientPaletteIncrementPalette( unsigned int id );
void NasrGraphicsRectGradientPaletteDecrementPalette( unsigned int id );
uint_fast8_t NasrGraphicsRectGradientPaletteGetColor1( unsigned int id );
void NasrGraphicsRectGradientPaletteSetColor1( unsigned int id, uint_fast8_t v );
void NasrGraphicsRectGradientPaletteIncrementColor1( unsigned int id );
void NasrGraphicsRectGradientPaletteDecrementColor1( unsigned int id );
uint_fast8_t NasrGraphicsRectGradientPaletteGetColor2( unsigned int id );
void NasrGraphicsRectGradientPaletteSetColor2( unsigned int id, uint_fast8_t v );
void NasrGraphicsRectGradientPaletteIncrementColor2( unsigned int id );
void NasrGraphicsRectGradientPaletteDecrementColor2( unsigned int id );

// TilemapGraphics Manipulation
void NasrGraphicsTilemapSetX( unsigned int id, float v );
void NasrGraphicsTilemapSetY( unsigned int id, float v );
unsigned int NasrGraphicsTilemapGetWidth( unsigned int id );
unsigned int NasrGraphicsTilemapGetHeight( unsigned int id );
void NasrGraphicsTilemapSetTileX( unsigned int id, unsigned int x, unsigned int y, unsigned char v );
void NasrGraphicsTilemapSetTileY( unsigned int id, unsigned int x, unsigned int y, unsigned char v );
void NasrGraphicsTilemapSetTilePalette( unsigned int id, unsigned int x, unsigned int y, unsigned char v );
void NasrGraphicsTilemapSetTileAnimation( unsigned int id, unsigned int x, unsigned int y, unsigned char v );
void NasrGraphicsTilemapSetTile( unsigned int id, unsigned int x, unsigned int y, NasrTile tile );
void NasrGraphicsTilemapClearTile( unsigned int id, unsigned int x, unsigned int y );
float NasrGraphicsTilemapGetOpacity( unsigned int id );
void NasrGraphicsTilemapSetOpacity( unsigned int id, float opacity );

// TextGraphics Manipulation
float NasrGraphicsTextGetXOffset( unsigned int id );
void NasrGraphicsTextSetXOffset( unsigned int id, float v );
void NasrGraphicsTextAddToXOffset( unsigned int id, float v );
float NasrGraphicsTextGetYOffset( unsigned int id );
void NasrGraphicsTextSetYOffset( unsigned int id, float v );
void NasrGraphicsTextAddToYOffset( unsigned int id, float v );
void NasrGraphicsTextSetCount( unsigned int id, int count );
void NasrGraphicsTextIncrementCount( unsigned int id );
void NasrSetTextOpacity( unsigned int id, float v );

// CounterGraphics Manipulation
void NasrGraphicsCounterSetNumber( unsigned int id, float n );
void NasrGraphicsCounterSetOpacity( unsigned int id, float v );
void NasrGraphicsCounterSetXOffset( unsigned id, float v );
void NasrGraphicsCounterSetYOffset( unsigned id, float v );

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
unsigned int NasrTextureGetWidth( unsigned int texture );
unsigned int NasrTextureGetHeight( unsigned int texture );

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
    int_fast8_t useglobalpal,
    float tilingx,
    float tilingy
);

// Debug
void NasrRectPrint( const NasrRect * r );
void NasrColorPrint( const NasrColor * c );
void NasrDebugGraphics( void );

#ifdef __cplusplus
}
#endif

#endif // NASR_H