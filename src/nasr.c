#include <cglm/call.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "json/json.h"
#include "nasr.h"
#include "nasr_io.h"
#include "nasr_log.h"
#include "nasr_math.h"
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define DEGREES_TO_RADIANS( n ) ( ( n ) * 3.14159f / 180.0f )

#define VERTEX_SIZE 8
#define VERTEX_RECT_SIZE VERTEX_SIZE * 4
#define INDICES_SIZE 6

#define BASE_MATRIX {\
    { 1.0f, 0.0f, 0.0f, 0.0f },\
    { 0.0f, 1.0f, 0.0f, 0.0f },\
    { 0.0f, 0.0f, 1.0f, 0.0f },\
    { 0.0f, 0.0f, 0.0f, 1.0f }\
}

#define NASR_CHAR_NORMAL     0
#define NASR_CHAR_WHITESPACE 1
#define NASR_CHAR_NEWLINE    2

typedef struct NasrChar
{
    NasrRect src;
    NasrRect dest;
    uint_fast8_t type;
} NasrChar;

#define NASR_SHADER_VERTEX   0
#define NASR_SHADER_FRAGMENT 1

typedef struct NasrShader
{
    int type;
    const char * code;
} NasrShader;

#define NASR_GRAPHIC_NONE          0
#define NASR_GRAPHIC_RECT          1
#define NASR_GRAPHIC_RECT_GRADIENT 2
#define NASR_GRAPHIC_RECT_PAL      3
#define NASR_GRAPHIC_SPRITE        4
#define NASR_GRAPHIC_TILEMAP       5
#define NASR_GRAPHIC_TEXT          6
#define NASR_GRAPHIC_COUNTER       7

#define NASR_PALETTE_NONE    0
#define NASR_PALETTE_SET     1
#define NASR_PALETTE_DEFAULT 2

typedef struct NasrGraphicRect
{
    NasrRect rect;
    NasrColor color;
} NasrGraphicRect;

typedef struct NasrGraphicRectPalette
{
    NasrRect rect;
    uint_fast8_t palette;
    uint_fast8_t dir;
    uint_fast8_t color1;
    uint_fast8_t color2;
    uint_fast8_t useglobalpal;
    float opacity;
} NasrGraphicRectPalette;

typedef struct NasrGraphicRectGradient
{
    NasrRect rect;
    NasrColor color1;
    NasrColor color2;
    NasrColor color3;
    NasrColor color4;
    uint_fast8_t dir;
} NasrGraphicRectGradient;

typedef struct NasrGraphicSprite
{
    mat4 model;
    NasrRect src;
    NasrRect dest;
    float rotation_x;
    float rotation_y;
    float rotation_z;
    float opacity;
    float tilingx;
    float tilingy;
    unsigned int texture;
    uint_fast8_t flip_x;
    uint_fast8_t flip_y;
    uint_fast8_t palette;
    int_fast8_t useglobalpal;
} NasrGraphicSprite;

typedef struct NasrGraphicTilemap
{
    unsigned int texture;
    unsigned int tilemap;
    NasrRect src;
    NasrRect dest;
    int_fast8_t useglobalpal;
    unsigned char * data;
    float opacity;
    float tilingx;
    float tilingy;
} NasrGraphicTilemap;

typedef struct NasrGraphicText
{
    unsigned int capacity;
    unsigned int count;
    NasrChar * chars;
    float * vertices;
    unsigned int * vaos;
    unsigned int * vbos;
    uint_fast8_t palette;
    uint_fast8_t palette_type;
    unsigned int charset;
    float xoffset;
    float yoffset;
    float shadow;
    float opacity;
} NasrGraphicText;

typedef struct NasrGraphicCounter
{
    unsigned int count;
    unsigned int maxdigits;
    unsigned int maxdecimals;
    uint_fast8_t numpadding;
    uint_fast8_t decimalpadding;
    NasrChar * chars;
    float maxnum;
    float * vertices;
    unsigned int * vaos;
    unsigned int * vbos;
    uint_fast8_t palette;
    uint_fast8_t palette_type;
    unsigned int charset;
    float xoffset;
    float yoffset;
    float shadow;
    float opacity;
    NasrColor colors[ 4 ];
} NasrGraphicCounter;

typedef union NasrGraphicData
{
    NasrGraphicRect         rect;
    NasrGraphicRectGradient gradient;
    NasrGraphicRectPalette  rectpal;
    NasrGraphicSprite       sprite;
    NasrGraphicTilemap      tilemap;
    NasrGraphicText         text;
    NasrGraphicCounter *    counter;
} NasrGraphicData;

typedef struct NasrGraphic
{
    uint_fast8_t type;
    float scrollx;
    float scrolly;
    NasrGraphicData data;
} NasrGraphic;

typedef struct Texture
{
    unsigned int width;
    unsigned int height;
    unsigned int indexed;
} Texture;

static const float vertices_base[] =
{
    // Vertices     // Texture coords   // Color
    0.5f,  0.5f,   1.0f, 1.0f,         1.0f, 1.0f, 1.0f, 1.0f,// top right
    0.5f, -0.5f,   1.0f, 0.0f,         1.0f, 1.0f, 1.0f, 1.0f, // bottom right
    -0.5f, -0.5f,   0.0f, 0.0f,         1.0f, 1.0f, 1.0f, 1.0f, // bottom left
    -0.5f,  0.5f,   0.0f, 1.0f,         1.0f, 1.0f, 1.0f, 1.0f,  // top left 
};

#define MAX_ANIMATION_FRAME 2 * 3 * 4 * 5 * 6 * 7 * 8
#define NUMBER_O_BASE_SHADERS 8

typedef struct TextureMapEntry { NasrHashKey key; unsigned int value; struct TextureMapEntry * next; } TextureMapEntry;

typedef struct CharTemplate
{
    NasrRect src;
    uint_fast8_t chartype;
} CharTemplate;

typedef struct CharNum
{
    NasrRect src;
    float xoffset;
    float yoffset;
} CharNum;

typedef struct { NasrHashKey key; CharTemplate value; } CharMapEntry;
typedef struct CharMap
{
    unsigned int capacity;
    unsigned int hashmax;
    CharMapEntry * list;
    unsigned int texture_id;
    Texture texture;
    CharNum nums[ 11 ];
    float numwidth;
    float numheight;
} CharMap;
typedef struct CharMapList
{
    unsigned int capacity;
    CharMap * list;
} CharMapList;

typedef struct SpriteUniforms
{
    GLint model;
    GLint palette_id;
    GLint opacity;
    GLint texture_data;
    GLint palette_data;
    GLint tiling;
} SpriteUniforms;

typedef struct RectUniforms
{
    GLint model;
} RectUniforms;

typedef struct RectPalUniforms
{
    GLint model;
    GLint palette_id;
    GLint palette_data;
    GLint opacity;
} RectPalUniforms;

typedef struct TilemapUniforms
{
    GLint model;
    GLint mapw;
    GLint maph;
    GLint tilesetw;
    GLint tileseth;
    GLint animation;
    GLint opacity;
    GLint texture;
    GLint palette;
    GLint mapdata;
    GLint globalpal;
    GLint tiling;
} TilemapUniforms;

typedef struct TextUniforms
{
    GLint texture;
    GLint shadow;
    GLint opacity;
    GLint palette_id;
    GLint palette_data;
    GLint model;
} TextUniforms;

// Static Data
static int magnification = 1;
static GLFWwindow * window;
static unsigned int * vaos;
static unsigned int * vbos;
static float * vertices;
static unsigned int ebo;
static unsigned int current_shader = ( unsigned int )( -1 );
static unsigned int rect_shader;
static unsigned int sprite_shader;
static unsigned int indexed_sprite_shader;
static unsigned int tilemap_shader;
static unsigned int tilemap_mono_shader;
static unsigned int text_shader;
static unsigned int text_pal_shader;
static unsigned int rect_pal_shader;
static unsigned int * base_shaders[ NUMBER_O_BASE_SHADERS ] =
{
    &rect_shader,
	&sprite_shader,
	&indexed_sprite_shader,
	&tilemap_shader,
	&tilemap_mono_shader,
	&text_shader,
	&text_pal_shader,
    &rect_pal_shader
};
static SpriteUniforms sprite_uniforms;
static SpriteUniforms indexed_sprite_uniforms;
static RectUniforms rect_uniforms;
static RectPalUniforms rect_pal_uniforms;
static TilemapUniforms tilemap_uniforms;
static TilemapUniforms tilemap_mono_uniforms;
static TextUniforms text_uniforms;
static TextUniforms text_pal_uniforms;
static NasrGraphic * graphics;
static unsigned int max_graphics;
static unsigned int num_o_graphics;
static NasrRect camera = { 0.0f, 0.0f, 0.0f, 0.0f };
static NasrRect prev_camera = { 0.0f, 0.0f, 0.0f, 0.0f };
static NasrRect canvas = { 0.0f, 0.0f, 0.0f, 0.0f };
static int max_textures;
static int texture_map_size;
static unsigned int * texture_ids;
static Texture * textures;
static int texture_count;
static GLuint framebuffer;
static GLint magnified_canvas_width;
static GLint magnified_canvas_height;
static GLint magnified_canvas_x;
static GLint magnified_canvas_y;
static int selected_texture = -1;
static GLint default_sample_type = GL_LINEAR;
static TextureMapEntry * texture_map;
static GLint default_indexed_mode = GL_RGBA;
static unsigned int palette_texture_id;
static Texture palette_texture;
static int max_states;
static int max_gfx_layers;
static int * layer_pos;
static int * gfx_ptrs_id_to_pos;
static int * gfx_ptrs_pos_to_id;
static int * state_for_gfx;
static int * layer_for_gfx;
static unsigned int animation_frame;
static float animation_timer;
static uint_fast8_t global_palette;
static CharMapList charmaps = { 0, 0 };
static float animation_ticks_per_frame;



// Static Functions
static int AddGraphic
(
    unsigned int state,
    unsigned int layer,
    struct NasrGraphic graphic
);
static void AddTexture( Texture * texture, unsigned int texture_id, const unsigned char * data, unsigned int width, unsigned int height, int sampling, int indexed );
static void BindBuffers( unsigned int id );
static void BufferDefault( float * vptr );
static void BufferVertices( float * vptr );
static CharMapEntry * CharMapGenEntry( unsigned int id, const char * key );
static CharMapEntry * CharMapHashFindEntry( unsigned int id, const char * needle_string, hash_t needle_hash );
static uint32_t CharMapHashString( unsigned int id, const char * key );
static void CharsetMalformedError( const char * msg, const char * file );
static void ClearBufferBindings( void );
static void DestroyGraphic( NasrGraphic * graphic );
static void DrawBox( unsigned int vao, const NasrRect * rect, float scrollx, float scrolly );
static void FramebufferSizeCallback( GLFWwindow * window, int width, int height );
static unsigned int GenerateShaderProgram( const NasrShader * shaders, int shadersnum );
static int GetCharacterSize( const char * s );
static GLint GetGLRGBA( int indexed );
static GLint GetGLSamplingType( int sampling );
static NasrGraphic * GetGraphic( unsigned int id );
static unsigned int GetStateLayerIndex( unsigned int state, unsigned int layer );
static float * GetVertices( unsigned int id );
static int GraphicsAddCounter
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    unsigned int charset,
    float num,
    unsigned int maxdigits,
    unsigned int maxdecimals,
    float x,
    float y,
    float shadow,
    float opacity,
    uint_fast8_t numpadding,
    uint_fast8_t decimalpadding,
    NasrColor ** colors,
    uint_fast8_t palette,
    uint_fast8_t palette_type
);
static int GraphicAddText
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    NasrColor * top_left_color,
    NasrColor * top_right_color,
    NasrColor * bottom_left_color,
    NasrColor * bottom_right_color,
    uint_fast8_t palette,
    uint_fast8_t palette_type
);
static void GraphicsRectGradientPaletteUpdateColors( unsigned int id, uint_fast8_t * c );
static void GraphicsUpdateRectPalette( unsigned int id, uint_fast8_t color );
static int GrowGraphics( void );
static unsigned char * LoadTextureFileData( const char * filename, unsigned int * width, unsigned int * height, int sampling, int indexed );
static void ResetVertices( float * vptr );
static void SetShader( unsigned int shader );
static void SetVerticesColors( unsigned int id, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesColorValues( float * vptr, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesView( float x, float y, float scrollx, float scrolly );
static void SetupVertices( unsigned int vao );
static uint32_t TextureMapHashString( const char * key );
static void UpdateShaderOrtho( float x, float y, float w, float h );
static void UpdateShaderOrthoToCamera( void );
static void UpdateSpriteModel( unsigned int id );
static void UpdateSpriteVertices( unsigned int id );
static void UpdateSpriteVerticesValues( float * vptr, const NasrGraphicSprite * sprite );
static void UpdateSpriteX( unsigned int id );
static void UpdateSpriteY( unsigned int id );



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
)
{
    animation_ticks_per_frame = ( float )( ticks_per_frame );

    default_sample_type = GetGLSamplingType( sample_type );
    default_indexed_mode = GetGLRGBA( default_indexed_type );
    glfwInit();

    // Init window
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );

    // Update canvas
    canvas.w = canvas_width;
    canvas.h = canvas_height;

    // Set up window
    window = glfwCreateWindow( canvas.w * magnification, canvas.h * magnification, program_title, NULL, NULL );
    if ( window == NULL )
    {
        NasrLog( "Failed to create GLFW window." );
        return -1;
    }

    glfwMakeContextCurrent( window );

    // Set up GLAD
    if ( !gladLoadGLLoader( ( GLADloadproc )( glfwGetProcAddress ) ) )
    {
        NasrLog( "Failed to initialize GLAD." );
        return -1;
    }

    // Update viewport on window resize.
    glfwSetFramebufferSizeCallback( window, FramebufferSizeCallback );

    // Turn on blending.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Init textures list
    max_textures = texture_map_size = init_max_textures;
    textures = calloc( max_textures, sizeof( Texture ) );
    texture_ids = calloc( max_textures, sizeof( unsigned int ) );
    glGenTextures( max_textures, texture_ids );
    glGenTextures( 1, &palette_texture_id );

    max_graphics = init_max_graphics;
    vaos = calloc( ( max_graphics + 1 ), sizeof( unsigned int ) );
    vbos = calloc( ( max_graphics + 1 ), sizeof( unsigned int ) );
    vertices = calloc( ( max_graphics + 1 ) * VERTEX_RECT_SIZE, sizeof( float ) );

    const unsigned int indices[] =
    {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };
    
    glGenVertexArrays( max_graphics + 1, vaos );
    glGenBuffers( max_graphics + 1, vbos );

    // Setup EBO
    glGenBuffers( 1, &ebo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

    for ( int i = 0; i < max_graphics + 1; ++i )
    {
        float * vptr = GetVertices( i );
        ResetVertices( vptr );
        BindBuffers( i );
        BufferDefault( vptr );
    }
    ClearBufferBindings();

    // Set up shaders
    NasrShader vertex_shader =
    {
        NASR_SHADER_VERTEX,
        "#version 330 core\n layout ( location = 0 ) in vec2 in_position;\n layout ( location = 1 ) in vec2 in_texture_coords;\n layout ( location = 2 ) in vec4 in_color;\n \n out vec2 texture_coords;\n out vec4 out_color;\n out vec2 out_position;\n \n uniform mat4 model;\n uniform mat4 view;\n uniform mat4 ortho;\n \n void main()\n {\n out_position = in_position;\n gl_Position = ortho * view * model * vec4( in_position, 0.0, 1.0 );\n texture_coords = in_texture_coords;\n out_color = in_color;\n }"
    };

    NasrShader rect_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\nin vec2 out_position;\n\nvoid main()\n{\n    final_color = out_color;\n}"
        }
    };

    NasrShader sprite_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform float opacity;\nuniform vec2 tiling;\n  \nvoid main()\n{\n    final_color = texture( texture_data, texture_coords * tiling );\n    final_color.a *= opacity;\n}"
        }
    };

    NasrShader indexed_sprite_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform float palette_id;\nuniform float opacity;\nuniform vec2 tiling;\n\nvoid main()\n{\n    vec4 index = texture( texture_data, texture_coords * tiling );\n    float palette = palette_id / 256.0;\n    final_color = texture( palette_data, vec2( ( 255.0 / 256.0 ) * index.r, palette ) );\n    final_color.a *= opacity;\n}"
        }
    };

    NasrShader tilemap_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform sampler2D map_data;\nuniform float map_width;\nuniform float map_height;\nuniform float tileset_width;\nuniform float tileset_height;\nuniform float opacity;\nuniform uint animation;\nuniform vec2 tiling;\n  \nvoid main()\n{\n    vec2 tc = texture_coords * tiling;\n    vec4 tile = texture( map_data, tc );\n    if ( tile.a > 0.0 && tile.a < 1.0 )\n    {\n        float frames = floor( tile.a * 255.0 );\n        float frame = mod( float( animation ), frames );\n        // I don’t know why mod sometimes doesn’t work right & still sometimes says 6 is the mod o’ 6 / 6 ’stead o’ 0;\n        // This fixes it.\n        while ( frame >= frames )\n        {\n            frame -= frames;\n        }\n        tile.x += frame / 255.0;\n    }\n    float xrel = mod( tc.x * 256.0, ( 256.0 / map_width ) ) / ( 4096.0 / map_width );\n    float yrel = mod( tc.y * 256.0, ( 256.0 / map_height ) ) / ( 4096.0 / map_height );\n    float xoffset = tile.x * 255.0 * ( 16 / tileset_width );\n    float yoffset = tile.y * 255.0 * ( 16 / tileset_height );\n    float palette = tile.z;\n    vec4 index = texture( texture_data, vec2( xoffset + ( xrel / ( tileset_width / 256.0 ) ), yoffset + ( yrel / ( tileset_height / 256.0 ) ) ) );\n    final_color = ( tile.a < 1.0 ) ? texture( palette_data, vec2( ( 255.0 / 256.0 ) * index.r, palette ) ) : vec4( 0.0, 0.0, 0.0, 0.0 );\n    final_color.a *= opacity;\n}"
        }
    };

    NasrShader tilemap_mono_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform sampler2D map_data;\nuniform float map_width;\nuniform float map_height;\nuniform float tileset_width;\nuniform float tileset_height;\nuniform float opacity;\nuniform uint animation;\nuniform uint global_palette;\nuniform vec2 tiling;\n  \nvoid main()\n{\n    vec2 tc = texture_coords * tiling;\n    vec4 tile = texture( map_data, tc );\n    if ( tile.a < 1.0 || opacity > 0.0 )\n    {\n        if ( tile.a > 0.0 && tile.a < 1.0 )\n        {\n            float frames = floor( tile.a * 255.0 );\n            float frame = mod( float( animation ), frames );\n            // I don’t know why mod sometimes doesn’t work right & still sometimes says 6 is the mod o’ 6 / 6 ’stead o’ 0;\n            // This fixes it.\n            while ( frame >= frames )\n            {\n                frame -= frames;\n            }\n            tile.x += frame / 255.0;\n        }\n        float xrel = mod( tc.x * 256.0, ( 256.0 / map_width ) ) / ( 4096.0 / map_width );\n        float yrel = mod( tc.y * 256.0, ( 256.0 / map_height ) ) / ( 4096.0 / map_height );\n        float xoffset = tile.x * 255.0 * ( 16 / tileset_width );\n        float yoffset = tile.y * 255.0 * ( 16 / tileset_height );\n        float palette = float( global_palette ) / 256.0;\n        vec4 index = texture( texture_data, vec2( xoffset + ( xrel / ( tileset_width / 256.0 ) ), yoffset + ( yrel / ( tileset_height / 256.0 ) ) ) );\n        final_color = ( tile.a < 1.0 ) ? texture( palette_data, vec2( ( 255.0 / 256.0 ) * index.r, palette ) ) : vec4( 0.0, 0.0, 0.0, 0.0 );\n        final_color.a *= opacity;\n    }\n}"
        }
    };

    NasrShader text_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform float opacity;\nuniform float shadow;\n  \nvoid main()\n{\n    vec4 texture_color = texture( texture_data, texture_coords );\n    if ( texture_color.r < 1.0 )\n    {\n        final_color = vec4( vec3( out_color.rgb * texture_color.rgb ), texture_color.a * shadow );\n    }\n    else\n    {\n        final_color = out_color * texture_color * opacity;\n    }\n}"
        }
    };

    NasrShader text_pal_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform float palette_id;\nuniform float opacity;\nuniform float shadow;\n\nvoid main()\n{\n    float palette = palette_id / 256.0;\n    vec4 texture_color = texture( texture_data, texture_coords );\n    final_color = texture_color * texture( palette_data, vec2( ( 255.0 / 256.0 ) * out_color.r, palette ) );\n    final_color.a *= texture_color.a * opacity;\n    if ( texture_color.r < 1.0 )\n    {\n        final_color.a *= shadow;\n    }\n}"
        }
    };

    NasrShader rect_pal_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\nin vec4 out_color;\n\nuniform sampler2D palette_data;\nuniform float palette_id;\nuniform float opacity;\n\nvoid main()\n{\n    float palette = palette_id / 256.0;\n    final_color = texture( palette_data, vec2( ( 255.0 / 256.0 ) * out_color.r, palette ) );\n    final_color.a *= opacity;\n}"
        }
    };
    
    rect_shader = GenerateShaderProgram( rect_shaders, 2 );
    sprite_shader = GenerateShaderProgram( sprite_shaders, 2 );
    indexed_sprite_shader = GenerateShaderProgram( indexed_sprite_shaders, 2 );
    tilemap_shader = GenerateShaderProgram( tilemap_shaders, 2 );
    tilemap_mono_shader = GenerateShaderProgram( tilemap_mono_shaders, 2 );
    text_shader = GenerateShaderProgram( text_shaders, 2 );
    text_pal_shader = GenerateShaderProgram( text_pal_shaders, 2 );
    rect_pal_shader = GenerateShaderProgram( rect_pal_shaders, 2 );

    // Store uniforms for use during rendering.
    sprite_uniforms.model        = glGetUniformLocation( sprite_shader, "model" );
    sprite_uniforms.opacity      = glGetUniformLocation( sprite_shader, "opacity" );
    sprite_uniforms.texture_data = glGetUniformLocation( sprite_shader, "texture_data" );
    sprite_uniforms.tiling       = glGetUniformLocation( sprite_shader, "tiling" );
    indexed_sprite_uniforms.model        = glGetUniformLocation( indexed_sprite_shader, "model" );
    indexed_sprite_uniforms.palette_id   = glGetUniformLocation( indexed_sprite_shader, "palette_id" );
    indexed_sprite_uniforms.opacity      = glGetUniformLocation( indexed_sprite_shader, "opacity" );
    indexed_sprite_uniforms.texture_data = glGetUniformLocation( indexed_sprite_shader, "texture_data" );
    indexed_sprite_uniforms.palette_data = glGetUniformLocation( indexed_sprite_shader, "palette_data" );
    indexed_sprite_uniforms.tiling       = glGetUniformLocation( indexed_sprite_shader, "tiling" );
    rect_uniforms.model = glGetUniformLocation( rect_shader, "model" );
    rect_pal_uniforms.model        = glGetUniformLocation( rect_pal_shader, "model" );
    rect_pal_uniforms.palette_id   = glGetUniformLocation( rect_pal_shader, "palette_id" );
    rect_pal_uniforms.palette_data = glGetUniformLocation( rect_pal_shader, "palette_data" );
    rect_pal_uniforms.opacity      = glGetUniformLocation( rect_pal_shader, "opacity" );
    tilemap_uniforms.model     = glGetUniformLocation( tilemap_shader, "model" );
    tilemap_uniforms.mapw      = glGetUniformLocation( tilemap_shader, "map_width" );
    tilemap_uniforms.maph      = glGetUniformLocation( tilemap_shader, "map_height" );
    tilemap_uniforms.tilesetw  = glGetUniformLocation( tilemap_shader, "tileset_width" );
    tilemap_uniforms.tileseth  = glGetUniformLocation( tilemap_shader, "tileset_height" );
    tilemap_uniforms.animation = glGetUniformLocation( tilemap_shader, "animation" );
    tilemap_uniforms.opacity   = glGetUniformLocation( tilemap_shader, "opacity" );
    tilemap_uniforms.texture   = glGetUniformLocation( tilemap_shader, "texture_data" );
    tilemap_uniforms.palette   = glGetUniformLocation( tilemap_shader, "palette_data" );
    tilemap_uniforms.mapdata   = glGetUniformLocation( tilemap_shader, "map_data" );
    tilemap_uniforms.tiling   = glGetUniformLocation( tilemap_shader, "tiling" );
    tilemap_mono_uniforms.model     = glGetUniformLocation( tilemap_mono_shader, "model" );
    tilemap_mono_uniforms.mapw      = glGetUniformLocation( tilemap_mono_shader, "map_width" );
    tilemap_mono_uniforms.maph      = glGetUniformLocation( tilemap_mono_shader, "map_height" );
    tilemap_mono_uniforms.tilesetw  = glGetUniformLocation( tilemap_mono_shader, "tileset_width" );
    tilemap_mono_uniforms.tileseth  = glGetUniformLocation( tilemap_mono_shader, "tileset_height" );
    tilemap_mono_uniforms.animation = glGetUniformLocation( tilemap_mono_shader, "animation" );
    tilemap_mono_uniforms.opacity   = glGetUniformLocation( tilemap_mono_shader, "opacity" );
    tilemap_mono_uniforms.texture   = glGetUniformLocation( tilemap_mono_shader, "texture_data" );
    tilemap_mono_uniforms.palette   = glGetUniformLocation( tilemap_mono_shader, "palette_data" );
    tilemap_mono_uniforms.mapdata   = glGetUniformLocation( tilemap_mono_shader, "map_data" );
    tilemap_mono_uniforms.globalpal = glGetUniformLocation( tilemap_mono_shader, "global_palette" );
    tilemap_mono_uniforms.tiling    = glGetUniformLocation( tilemap_mono_shader, "tiling" );
    text_uniforms.texture = glGetUniformLocation( text_shader, "texture_data" );
    text_uniforms.shadow = glGetUniformLocation( text_shader, "shadow" );
    text_uniforms.opacity = glGetUniformLocation( text_shader, "opacity" );
    text_uniforms.palette_id = glGetUniformLocation( text_shader, "palette_id" );
    text_uniforms.palette_data = glGetUniformLocation( text_shader, "palette_data" );
    text_uniforms.model = glGetUniformLocation( text_shader, "model" );
    text_pal_uniforms.texture      = glGetUniformLocation( text_pal_shader, "texture_data" );
    text_pal_uniforms.shadow       = glGetUniformLocation( text_pal_shader, "shadow" );
    text_pal_uniforms.opacity      = glGetUniformLocation( text_pal_shader, "opacity" );
    text_pal_uniforms.palette_id   = glGetUniformLocation( text_pal_shader, "palette_id" );
    text_pal_uniforms.palette_data = glGetUniformLocation( text_pal_shader, "palette_data" );
    text_pal_uniforms.model        = glGetUniformLocation( text_pal_shader, "model" );

    // Init camera
    NasrResetCamera();
    UpdateShaderOrthoToCamera();

    // Init graphics list
    max_states = init_max_states;
    max_gfx_layers = init_max_gfx_layers;
    graphics = calloc( max_graphics, sizeof( NasrGraphic ) );
    gfx_ptrs_id_to_pos = calloc( max_graphics, sizeof( int ) );
    gfx_ptrs_pos_to_id = calloc( max_graphics, sizeof( int ) );
    state_for_gfx = calloc( max_graphics, sizeof( int ) );
    layer_for_gfx = calloc( max_graphics, sizeof( int ) );
    layer_pos = calloc( max_states * max_gfx_layers, sizeof( int ) );

    // Initialize these to null values ( since 0 is a valid value, we use -1 ).
    for ( int i = 0; i < max_graphics; ++i )
    {
        gfx_ptrs_id_to_pos[ i ] = gfx_ptrs_pos_to_id[ i ] = state_for_gfx[ i ] = layer_for_gfx[ i ] = -1;
    }

    // Init framebuffer.
    glGenFramebuffers( 1, &framebuffer );

    magnified_canvas_width = canvas.w * magnification;
    magnified_canvas_height = canvas.h * magnification;
    magnified_canvas_x = ( int )( floor( ( double )( magnified_canvas_width - magnified_canvas_width ) / 2.0 ) );
    magnified_canvas_y = ( int )( floor( ( double )( magnified_canvas_height - magnified_canvas_height ) / 2.0 ) );
    glViewport( magnified_canvas_x, magnified_canvas_y, magnified_canvas_width, magnified_canvas_height );

    // Init texture map.
    texture_map = calloc( texture_map_size, sizeof( TextureMapEntry ) );

    // Set framerate
    glfwSwapInterval( vsync );

    return 0;
};

void NasrClose( void )
{
    #ifdef NASR_DEBUG
        // Close charmaps
        if ( charmaps.list )
        {
            for ( int i = 0; i < charmaps.capacity; ++i )
            {
                NasrRemoveCharset( i );
            }
            free( charmaps.list );
        }

        for ( int i = 0; i < num_o_graphics; ++i )
        {
            DestroyGraphic( &graphics[ i ] );
        }
        glDeleteBuffers( 1, &ebo );
        if ( vaos )
        {
            glDeleteVertexArrays( max_graphics + 1, vaos );
            free( vaos );
        }
        if ( vbos )
        {
            glDeleteBuffers( max_graphics + 1, vbos );
            free( vbos );
        }
        free( vertices );
        glDeleteFramebuffers( 1, &framebuffer );
        NasrClearTextures();
        free( texture_map );
        free( textures );
        glDeleteTextures( 1, &palette_texture_id );
        if ( texture_ids )
        {
            glDeleteTextures( max_textures, texture_ids );
            free( texture_ids );
        }
        free( graphics );
        free( gfx_ptrs_id_to_pos );
        free( gfx_ptrs_pos_to_id );
        free( layer_pos );
        free( state_for_gfx );
        free( layer_for_gfx );
        glfwTerminate();
    #endif
};

void NasrUpdate( float dt )
{
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Only update ortho if camera has moved.
    if ( camera.x != prev_camera.x || camera.y != prev_camera.y )
    {
        UpdateShaderOrthoToCamera();
    }

    for ( int i = 0; i < num_o_graphics; ++i )
    {
        const unsigned int id = ( unsigned int )( gfx_ptrs_pos_to_id[ i ] );
        const unsigned vao = vaos[ id ];
        BindBuffers( id );
        switch ( graphics[ i ].type )
        {
            case ( NASR_GRAPHIC_RECT ):
            {
                DrawBox
                (
                    vao,
                    &graphics[ i ].data.rect.rect,
                    graphics[ i ].scrollx,
                    graphics[ i ].scrolly
                );
                
                SetupVertices( vao );
            }
            break;
            case ( NASR_GRAPHIC_RECT_GRADIENT ):
            {
                DrawBox
                (
                    vao,
                    &graphics[ i ].data.gradient.rect,
                    graphics[ i ].scrollx,
                    graphics[ i ].scrolly
                );
                
                SetupVertices( vao );
            }
            break;
            case ( NASR_GRAPHIC_RECT_PAL ):
            {
                #define RECT graphics[ i ].data.rectpal.rect

                SetShader( rect_pal_shader );

                SetVerticesView
                (
                    RECT.x + ( RECT.w / 2.0f ),
                    RECT.y + ( RECT.h / 2.0f ),
                    graphics[ i ].scrollx,
                    graphics[ i ].scrolly
                );

                // Set scale.
                mat4 model = BASE_MATRIX;
                vec3 scale = { RECT.w, RECT.h, 0.0 };
                glm_scale( model, scale );
                glUniformMatrix4fv( rect_pal_uniforms.model, 1, GL_FALSE, ( float * )( model ) );

                // Set palette ID.
                const float palette = ( float )
                ( 
                    graphics[ i ].data.rectpal.useglobalpal
                        ? global_palette
                        : graphics[ i ].data.rectpal.palette
                );
                glUniform1f( rect_pal_uniforms.palette_id, palette );
                glUniform1f( rect_pal_uniforms.opacity, graphics[ i ].data.rectpal.opacity );

                // Set palette texture.
                glActiveTexture( GL_TEXTURE1 );
                glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                glUniform1i( rect_pal_uniforms.palette_data, 1 );
                
                SetupVertices( vao );

                #undef RECT
            }
            break;
            case ( NASR_GRAPHIC_SPRITE ):
            {
                #define SPRITE graphics[ i ].data.sprite
                #define SRC SPRITE.src
                #define DEST SPRITE.dest

                // Ignore if offscreen.
                if
                (
                    graphics[ i ].scrollx == 0.0f && graphics[ i ].scrolly == 0.0f &&
                    (
                        DEST.x + DEST.w < camera.x ||
                        DEST.y + DEST.h < camera.y ||
                        DEST.x > camera.x + camera.w ||
                        DEST.y > camera.y + camera.h
                    )
                )
                {
                    break;
                }

                unsigned int texture_id = SPRITE.texture;

                if ( texture_id >= max_textures )
                {
                    NasrLog( "NasrUpdate Error: Invalid texture #%u beyond limit.", texture_id );
                    continue;
                }

                // Set shader.
                const SpriteUniforms * shader_uniforms = textures[ texture_id ].indexed
                    ? &indexed_sprite_uniforms
                    : &sprite_uniforms;
                const unsigned int shader = textures[ texture_id ].indexed ? indexed_sprite_shader : sprite_shader;
                SetShader( shader );

                // Set view.
                SetVerticesView
                (
                    DEST.x + ( DEST.w / 2.0f ),
                    DEST.y + ( DEST.h / 2.0f ),
                    graphics[ i ].scrollx,
                    graphics[ i ].scrolly
                );

                // Set scale.
                glUniformMatrix4fv( shader_uniforms->model, 1, GL_FALSE, ( float * )( SPRITE.model ) );

                // Set tiling.
                glUniform2f( shader_uniforms->tiling, SPRITE.tilingx, SPRITE.tilingy );

                // Set opacity.
                glUniform1f( shader_uniforms->opacity, ( float )( SPRITE.opacity ) );

                // Set texture.
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, texture_ids[ texture_id ] );
                glUniform1i( shader_uniforms->texture_data, 0 );

                // Set palette ID & texture if set to indexed.
                if ( textures[ texture_id ].indexed )
                {
                    const float palette = ( float )( SPRITE.useglobalpal ? global_palette : SPRITE.palette );
                    glUniform1f( shader_uniforms->palette_id, palette );

                    glActiveTexture( GL_TEXTURE1);
                    glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                    glUniform1i( shader_uniforms->palette_data, 1 );
                }
                
                SetupVertices( vao );

                #undef SPRITE
                #undef SRC
                #undef DEST
            }
            break;
            case ( NASR_GRAPHIC_TILEMAP ):
            {
                #define TG graphics[ i ].data.tilemap

                if ( TG.texture >= max_textures )
                {
                    NasrLog( "NasrUpdate Error: Invalid texture #%u beyond limit.", TG.texture );
                    continue;
                }

                // Set shader.
                const unsigned int shader = TG.useglobalpal ? tilemap_mono_shader : tilemap_shader;
                const TilemapUniforms * uniforms = TG.useglobalpal ? &tilemap_mono_uniforms : &tilemap_uniforms;
                SetShader( shader );

                // Set view.
                SetVerticesView
                (
                    TG.dest.x + ( TG.dest.w / 2.0f ),
                    TG.dest.y + ( TG.dest.h / 2.0f ),
                    graphics[ i ].scrollx,
                    graphics[ i ].scrolly
                );

                // Set scale.
                mat4 model = BASE_MATRIX;
                vec3 scale = { TG.dest.w * TG.tilingx, TG.dest.h * TG.tilingy, 0.0 };
                glm_scale( model, scale );
                glUniformMatrix4fv( uniforms->model, 1, GL_FALSE, ( float * )( model ) );

                // Set tiling.
                glUniform2f( uniforms->tiling, TG.tilingx, TG.tilingy );

                // Set map width.
                const float mapw = ( float )( textures[ TG.tilemap ].width );
                glUniform1f( uniforms->mapw, mapw );

                // Set map height.
                const float maph = ( float )( textures[ TG.tilemap ].height );
                glUniform1f( uniforms->maph, maph );

                // Set tileset width.
                glUniform1f( uniforms->tilesetw, ( float )( textures[ TG.texture ].width ) );

                // Set tileset height.
                glUniform1f( uniforms->tileseth, ( float )( textures[ TG.texture ].height ) );

                // Set animation counter.
                glUniform1ui( uniforms->animation, animation_frame );

                // Set opacity.
                glUniform1f( uniforms->opacity, TG.opacity );

                // Set tileset texture.
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, texture_ids[ TG.texture ] );
                glUniform1i( uniforms->texture, 0 );

                // Set palette texture.
                glActiveTexture( GL_TEXTURE1 );
                glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                glUniform1i( uniforms->palette, 1 );

                // Set tilemap texture.
                glActiveTexture( GL_TEXTURE2 );
                glBindTexture( GL_TEXTURE_2D, texture_ids[ TG.tilemap ] );
                glUniform1i( uniforms->mapdata, 2 );

                // If using global palette, set its ID.
                if ( TG.useglobalpal )
                {
                    glUniform1ui( uniforms->globalpal, ( GLuint )( global_palette ) );
                }

                SetupVertices( vao );

                #undef TG
            }
            break;
            case ( NASR_GRAPHIC_TEXT ):
            {
                // Set shader.
                const unsigned int shader = graphics[ i ].data.text.palette_type ? text_pal_shader : text_shader;
                const TextUniforms * uniforms = graphics[ i ].data.text.palette_type
                    ? &text_pal_uniforms
                    : &text_uniforms;
                SetShader( shader );

                // Set texture.
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, charmaps.list[ graphics[ i ].data.text.charset ].texture_id );
                glUniform1i( uniforms->texture, 0 );

                // Set shadow.
                glUniform1f( uniforms->shadow, graphics[ i ].data.text.shadow );

                // Set opacity.
                glUniform1f( uniforms->opacity, graphics[ i ].data.text.opacity );

                // If using palette, set palette.
                if ( graphics[ i ].data.text.palette_type )
                {
                    const float palette = ( float )
                    (
                        graphics[ i ].data.text.palette_type == NASR_PALETTE_DEFAULT
                            ? global_palette
                            : graphics[ i ].data.text.palette
                    );
                    glUniform1f( uniforms->palette_id, palette );

                    glActiveTexture( GL_TEXTURE1 );
                    glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                    glUniform1i( uniforms->palette_data, 1 );
                }

                for ( int j = 0; j < graphics[ i ].data.text.count; ++j )
                {
                    #define CHAR graphics[ i ].data.text.chars[ j ]
                    
                    // Set buffers.
                    glBindVertexArray( graphics[ i ].data.text.vaos[ j ] );
                    glBindBuffer( GL_ARRAY_BUFFER, graphics[ i ].data.text.vbos[ j ] );

                    // Set view.
                    SetVerticesView
                    (
                        CHAR.dest.x + ( CHAR.dest.w / 2.0f ) + graphics[ i ].data.text.xoffset,
                        CHAR.dest.y + ( CHAR.dest.h / 2.0f ) + graphics[ i ].data.text.yoffset,
                        graphics[ i ].scrollx,
                        graphics[ i ].scrolly
                    );

                    // Set scale.
                    mat4 model = BASE_MATRIX;
                    vec3 scale = { CHAR.dest.w, CHAR.dest.h, 0.0 };
                    glm_scale( model, scale );
                    glUniformMatrix4fv( uniforms->model, 1, GL_FALSE, ( float * )( model ) );

                    SetupVertices( graphics[ i ].data.text.vaos[ j ] );
                    ClearBufferBindings();

                    #undef CHAR
                }
            }
            break;
            case ( NASR_GRAPHIC_COUNTER ):
            {
                // Set shader.
                const unsigned int shader = graphics[ i ].data.counter->palette_type ? text_pal_shader : text_shader;
                const TextUniforms * uniforms = graphics[ i ].data.counter->palette_type
                    ? &text_pal_uniforms
                    : &text_uniforms;
                SetShader( shader );

                // Set texture.
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, charmaps.list[ graphics[ i ].data.counter->charset ].texture_id );
                glUniform1i( uniforms->texture, 0 );

                // Set shadow.
                glUniform1f( uniforms->shadow, graphics[ i ].data.counter->shadow );

                // Set opacity.
                glUniform1f( uniforms->opacity, graphics[ i ].data.counter->opacity );

                // If using palette, set palette.
                if ( graphics[ i ].data.counter->palette_type )
                {
                    const float palette = ( float )
                    (
                        graphics[ i ].data.counter->palette_type == NASR_PALETTE_DEFAULT
                            ? global_palette
                            : graphics[ i ].data.counter->palette
                    );
                    glUniform1f( uniforms->palette_id, palette );

                    glActiveTexture( GL_TEXTURE1 );
                    glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                    glUniform1i( uniforms->palette_data, 1 );
                }

                for ( int j = 0; j < graphics[ i ].data.counter->count; ++j )
                {
                    #define CHAR graphics[ i ].data.counter->chars[ j ]

                    // Bind buffers.
                    glBindVertexArray( graphics[ i ].data.counter->vaos[ j ] );
                    glBindBuffer( GL_ARRAY_BUFFER, graphics[ i ].data.counter->vbos[ j ] );

                    // Set view.
                    SetVerticesView
                    (
                        CHAR.dest.x + ( CHAR.dest.w / 2.0f ) + graphics[ i ].data.counter->xoffset,
                        CHAR.dest.y + ( CHAR.dest.h / 2.0f ) + graphics[ i ].data.counter->yoffset,
                        graphics[ i ].scrollx,
                        graphics[ i ].scrolly
                    );

                    // Set scale.
                    mat4 model = BASE_MATRIX;
                    vec3 scale = { CHAR.dest.w, CHAR.dest.h, 0.0 };
                    glm_scale( model, scale );
                    glUniformMatrix4fv( uniforms->model, 1, GL_FALSE, ( float * )( model ) );

                    SetupVertices( graphics[ i ].data.counter->vaos[ j ] );
                    ClearBufferBindings();

                    #undef CHAR
                }
            }
            break;
            default:
            {
                NasrLog( "¡Trying to render invalid graphic type #%d!\n", graphics[ i ].type );
            }
            break;
        }
        ClearBufferBindings();
    }

    glfwSwapBuffers( window );
    prev_camera = camera;

    animation_timer += dt;
    if ( animation_timer >= animation_ticks_per_frame )
    {
        animation_timer -= animation_ticks_per_frame;
        ++animation_frame;
        if ( animation_frame == MAX_ANIMATION_FRAME )
        {
            animation_frame = 0;
        }
    }
};

void NasrHandleEvents( void )
{
    glfwPollEvents();
};

int NasrHasClosed( void )
{
    return glfwWindowShouldClose( window );
};



// Input
void NasrRegisterInputHandler( input_handle_t handler )
{
    glfwSetKeyCallback( window, handler );
};



// Rect
float NasrRectRight( const NasrRect * r )
{
    return r->x + r->w;
};

float NasrRectBottom( const NasrRect * r )
{
    return r->y + r->h;
};

int NasrRectEqual( const NasrRect * a, const NasrRect * b )
{
    return a->x == b->x
        && a->y == b->y
        && a->w == b->w
        && a->h == b->h;
};

NasrRectInt NasrRectToNasrRectInt( const NasrRect r )
{
    NasrRectInt r2;
    r2.x = ( int )( r.x );
    r2.y = ( int )( r.y );
    r2.w = ( int )( r.w );
    r2.h = ( int )( r.h );
    return r2;
};



// Palette
void NasrSetPalette( const char * filename )
{
    unsigned int width;
    unsigned int height;
    unsigned char * data = LoadTextureFileData( filename, &width, &height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    AddTexture( &palette_texture, palette_texture_id, data, width, height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    free( data );
};

void NasrSetGlobalPalette( uint_fast8_t palette )
{
    global_palette = palette;
};



// Charset
int NasrAddCharset( const char * texture, const char * chardata )
{
    char * text = NasrReadFile( chardata );
    if ( !text )
    {
        NasrLog( "NasrAddCharset Error: Could not load file “%s”.", chardata );
        return -1;
    }
    json_char * json = ( json_char * )( text );
    json_value * root = json_parse( json, strlen( text ) + 1 );
    free( text );
    if ( !root || root->type != json_object || !root->u.object.length )
    {
        NasrLog( "NasrAddCharset Error: Could not parse JSON from “%s”.", chardata );
        return -1;
    }

    int id = -1;
    for ( unsigned int i = 0; i < root->u.object.length; ++i )
    {
        const json_object_entry root_entry = root->u.object.values[ i ];
        if ( strcmp( "characters", root_entry.name ) == 0 )
        {
            if ( root_entry.value->type != json_array )
            {
                CharsetMalformedError( "characters is not an array.", chardata );
                return -1;
            }

            CharTemplate chars[ root_entry.value->u.array.length ];
            char * keys[ root_entry.value->u.array.length ];

            for ( unsigned int j = 0; j < root_entry.value->u.array.length; ++j )
            {
                const json_value * char_item = root_entry.value->u.array.values[ j ];
                if ( char_item->type != json_object )
                {
                    CharsetMalformedError( "character entry not an object.", chardata );
                    return -1;
                }

                // Setup defaults.
                keys[ j ] = 0;
                chars[ j ].src.x = chars[ j ].src.y = 0.0f;
                chars[ j ].src.w = chars[ j ].src.h = 8.0f;
                chars[ j ].chartype = NASR_CHAR_NORMAL;

                for ( unsigned int k = 0; k < char_item->u.object.length; ++k )
                {
                    const json_object_entry char_entry = char_item->u.object.values[ k ];
                    if ( strcmp( "key", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            CharsetMalformedError( "character key is not a string.", chardata );
                            return -1;
                        }
                        keys[ j ] = char_entry.value->u.string.ptr;
                    }
                    else if ( strcmp( "type", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            CharsetMalformedError( "character type is not a string.", chardata );
                            return -1;
                        }
                        if ( strcmp( "whitespace", char_entry.value->u.string.ptr ) == 0 )
                        {
                            chars[ j ].chartype = NASR_CHAR_WHITESPACE;
                        }
                    }
                    else if ( strcmp( "x", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type == json_integer )
                        {
                            chars[ j ].src.x = ( float )( char_entry.value->u.integer );
                        }
                        else if ( char_entry.value->type == json_double )
                        {
                            chars[ j ].src.x = ( float )( char_entry.value->u.dbl );
                        }
                        else
                        {
                            CharsetMalformedError( "x is not an int.", chardata );
                            return -1;
                        }
                    }
                    else if ( strcmp( "y", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type == json_integer )
                        {
                            chars[ j ].src.y = ( float )( char_entry.value->u.integer );
                        }
                        else if ( char_entry.value->type == json_double )
                        {
                            chars[ j ].src.y = ( float )( char_entry.value->u.dbl );
                        }
                        else
                        {
                            CharsetMalformedError( "y is not an int.", chardata );
                            return -1;
                        }
                    }
                    else if ( strcmp( "w", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type == json_integer )
                        {
                            chars[ j ].src.w = ( float )( char_entry.value->u.integer );
                        }
                        else if ( char_entry.value->type == json_double )
                        {
                            chars[ j ].src.w = ( float )( char_entry.value->u.dbl );
                        }
                        else
                        {
                            CharsetMalformedError( "w is not an int.", chardata );
                            return -1;
                        }
                    }
                    else if ( strcmp( "h", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type == json_integer )
                        {
                            chars[ j ].src.h = ( float )( char_entry.value->u.integer );
                        }
                        else if ( char_entry.value->type == json_double )
                        {
                            chars[ j ].src.h = ( float )( char_entry.value->u.dbl );
                        }
                        else
                        {
                            CharsetMalformedError( "h is not an int.", chardata );
                            return -1;
                        }
                    }
                }

                if ( !keys[ j ] )
                {
                    CharsetMalformedError( "key missing from characters entry", chardata );
                    return -1;
                }
            }

            if ( !charmaps.list )
            {
                charmaps.capacity = 5;
                charmaps.list = calloc( charmaps.capacity, sizeof( CharMap ) );
            }

            // Find 1st unused charmaps slot.
            for ( int i = 0; i < charmaps.capacity; ++i )
            {
                if ( !charmaps.list[ i ].list )
                {
                    id = i;
                    break;
                }
            }

            // If all available slots used, add mo’ slots.
            if ( id == -1 )
            {
                size_t newcapacity = charmaps.capacity == 0 ? 5 : charmaps.capacity * 2;
                CharMap * newlist = calloc( newcapacity, sizeof( CharMap ) );
                for ( int l = 0; l < charmaps.capacity; ++l )
                {
                    newlist[ l ] = charmaps.list[ l ];
                }
                free( charmaps.list );
                charmaps.list = newlist;
                id = charmaps.capacity;
                charmaps.capacity = newcapacity;
            }

            charmaps.list[ id ].capacity = NasrGetNextPrime( root_entry.value->u.array.length + 3 );
            charmaps.list[ id ].hashmax = charmaps.list[ id ].capacity;
            charmaps.list[ id ].list = calloc( charmaps.list[ id ].capacity, sizeof( CharMapEntry ) );

            unsigned int width;
            unsigned int height;
            unsigned char * data = LoadTextureFileData( texture, &width, &height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
            glGenTextures( 1, &charmaps.list[ id ].texture_id );
            AddTexture( &charmaps.list[ id ].texture, charmaps.list[ id ].texture_id, data, width, height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
            free( data );

            // Autogenerate newline & whitespace characters.
            CharMapEntry * entry = CharMapGenEntry( ( unsigned int )( id ), "\n" );
            entry->value.src.w = entry->value.src.h = 0.0f;
            entry->value.chartype = NASR_CHAR_NEWLINE;
            entry = CharMapGenEntry( ( unsigned int )( id ), " " );
            entry->value.src.w = 5.0f;
            entry->value.src.h = 8.0f;
            entry->value.chartype = NASR_CHAR_WHITESPACE;

            for ( unsigned int c = 0; c < root_entry.value->u.array.length; ++c )
            {
                entry = CharMapGenEntry( id, keys[ c ] );
                entry->value = chars[ c ];
            }

            // Generate list of digit characters for counters.
            char digit[ 1 ] = "0";
            charmaps.list[ id ].numwidth = 0.0f;
            charmaps.list[ id ].numheight = 0.0f;
            for ( int i = 0; i < 11; ++i )
            {
                // Include period for floating point #s.
                digit[ 0 ] = i == 10 ? '.' : '0' + i;
                hash_t digithash = CharMapHashString( ( unsigned int )( id ), digit );
                CharMapEntry * digitentry = CharMapHashFindEntry( ( unsigned int )( id ), digit, digithash );
                if ( digitentry->key.string != NULL )
                {
                    charmaps.list[ id ].nums[ i ].src = digitentry->value.src;
                    if ( charmaps.list[ id ].nums[ i ].src.w > charmaps.list[ id ].numwidth )
                    {
                        charmaps.list[ id ].numwidth = charmaps.list[ id ].nums[ i ].src.w;
                    }
                    if ( charmaps.list[ id ].nums[ i ].src.h > charmaps.list[ id ].numheight )
                    {
                        charmaps.list[ id ].numheight = charmaps.list[ id ].nums[ i ].src.h;
                    }
                }
            }

            for ( int i = 0; i < 11; ++i )
            {
                charmaps.list[ id ].nums[ i ].xoffset = ( charmaps.list[ id ].numwidth - charmaps.list[ id ].nums[ i ].src.w ) / 2.0f;
                charmaps.list[ id ].nums[ i ].yoffset = ( charmaps.list[ id ].numheight - charmaps.list[ id ].nums[ i ].src.h ) / 2.0f;
            }
        }
    }

    if ( id <= -1 )
    {
        NasrLog( "NasrAddCharset Error: Could not find characters object in “%s”.", chardata );
    }

    json_value_free( root );
    return id;
};

void NasrRemoveCharset( unsigned int charset )
{
    if ( charmaps.list[ charset ].list )
    {
        for ( int j = 0; j < charmaps.list[ charset ].capacity; ++j )
        {
            if ( charmaps.list[ charset ].list[ j ].key.string )
            {
                free( charmaps.list[ charset ].list[ j ].key.string );
            }
        }
        glDeleteTextures( 1, &charmaps.list[ charset ].texture_id );
        free( charmaps.list[ charset ].list );
        charmaps.list[ charset ].list = 0;
    }
};



// Time
double NasrGetTime( void )
{
    return glfwGetTime();
};



// Camera
void NasrAdjustCamera( NasrRect * target, float max_w, float max_h )
{
    float x_adjust = 0.0f;
    const float right_edge = canvas.w * 0.75 + camera.x;
    const float left_edge = canvas.w * 0.25 + camera.x;
    const float target_right = NasrRectRight( target );
    
    if ( target_right > right_edge )
    {
        x_adjust = target_right - right_edge;
    }
    else if ( target->x < left_edge )
    {
        x_adjust = target->x - left_edge;
    }
    camera.x += x_adjust;
    camera.w += x_adjust;
    if ( camera.x < 0.0f )
    {
        camera.w = canvas.w;
        camera.x = 0.0f;
    }
    else if ( camera.w > max_w )
    {
        camera.x = max_w - canvas.w;
        camera.w = max_w;
    }

    float y_adjust = 0.0f;
    const float bottom_edge = canvas.h * 0.75 + camera.y;
    const float top_edge = canvas.h * 0.25 + camera.y;
    const float target_bottom = NasrRectBottom( target );
    
    if ( target_bottom > bottom_edge )
    {
        y_adjust = target_bottom - bottom_edge;
    }
    else if ( target->y < top_edge )
    {
        y_adjust = target->y - top_edge;
    }
    camera.y += y_adjust;
    camera.h += y_adjust;
    if ( camera.y < 0.0f )
    {
        camera.h = canvas.h;
        camera.y = 0.0f;
    }
    else if ( camera.h > max_h )
    {
        camera.y = max_h - canvas.h;
        camera.h = max_h;
    }
};

void NasrMoveCamera( float x, float y, float max_w, float max_h )
{
    camera.x += x;
    camera.y += y;
    if ( NasrRectRight( &camera ) > max_w )
    {
        camera.x = max_w - camera.w;
    }
    if ( NasrRectBottom( &camera ) > max_h )
    {
        camera.y = max_h - camera.h;
    }
};

void NasrResetCamera( void )
{
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.w = canvas.w;
    camera.h = canvas.h;
};



// Layers
void NasrGraphicChangeLayer( unsigned int id, unsigned int layer )
{
    // Skip if already on layer.
    if ( layer_for_gfx[ id ] == layer )
    {
        return;
    }

    // Make copy o’ graphic.
    NasrGraphic gfx = graphics[ gfx_ptrs_id_to_pos[ id ] ];

    const unsigned int state = state_for_gfx[ id ];
    const unsigned int current_layer_index = GetStateLayerIndex( state, layer_for_gfx[ id ] );
    const unsigned int target_layer_index = GetStateLayerIndex( state, layer );
    const unsigned int target_layer_pos = layer_pos[ target_layer_index ];
    if ( layer < layer_for_gfx[ id ] )
    {
        // If already lowest graphic, then there is nothing below, so we don’t need to move graphic downward.
        if ( gfx_ptrs_id_to_pos[ id ] == 0 )
        {
            layer_for_gfx[ id ] = layer;
            return;
        }

        // Push forward all following graphics.
        for ( unsigned int i = gfx_ptrs_id_to_pos[ id ]; i > target_layer_pos; --i )
        {
            graphics[ i ] = graphics[ i - 1 ];

            // Update pointers so they still point to correct graphics.
            const unsigned int t = gfx_ptrs_pos_to_id[ i - 1 ];
            ++gfx_ptrs_id_to_pos[ t ];            
            gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
        }

        graphics[ target_layer_pos ] = gfx;
        gfx_ptrs_id_to_pos[ id ] = target_layer_pos;
        gfx_ptrs_pos_to_id[ target_layer_pos ] = id;

        // Push up all layers from original point to selected layer.
        for ( unsigned int i = target_layer_index; i < current_layer_index; ++i )
        {
            ++layer_pos[ i ];
        }
    }
    else
    {
        // Push down all graphics between them.
        for ( unsigned int i = gfx_ptrs_id_to_pos[ id ]; i < target_layer_pos - 1; ++i )
        {
            graphics[ i ] = graphics[ i + 1 ];

            // Update pointers so they still point to correct graphics.
            const unsigned int t = gfx_ptrs_pos_to_id[ i + 1 ];
            --gfx_ptrs_id_to_pos[ t ];
            gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
        }

        // Move copied graphic to end of target layer & update pointers.
        graphics[ target_layer_pos - 1 ] = gfx;
        gfx_ptrs_id_to_pos[ id ] = target_layer_pos - 1;
        gfx_ptrs_pos_to_id[ target_layer_pos - 1 ] = id;

        // Push down all layers from original point to selected layer.
        for ( unsigned int i = current_layer_index; i < target_layer_index; ++i )
        {
            --layer_pos[ i ];
        }
    }
    layer_for_gfx[ id ] = layer;
};

void NasrSendGraphicToFrontOLayer( unsigned int id )
{
    const unsigned int end = NasrNumOGraphicsInLayer( state_for_gfx[ id ], layer_for_gfx[ id ] );
    NasrPlaceGraphicAbovePositionInLayer( id, end > 0 ? end : 0 );
};

void NasrSendGraphicToBackOLayer( unsigned int id )
{
    NasrPlaceGraphicBelowPositionInLayer( id, 0 );
};

void NasrRaiseGraphicForwardInLayer( unsigned int id )
{
    const unsigned int state = state_for_gfx[ id ];
    const unsigned int layer = layer_for_gfx[ id ];
    const unsigned int rel_pos = NasrGetLayerPosition( id );

    // Skip if already @ front.
    if ( rel_pos >= NasrNumOGraphicsInLayer( state, layer ) - 1 )
    {
        return;
    }

    // Make copy o’ graphic & swap with graphic ’bove.
    const unsigned int abs_pos = gfx_ptrs_id_to_pos[ id ];
    NasrGraphic gfx = graphics[ abs_pos ];
    graphics[ abs_pos ] = graphics[ abs_pos + 1 ];
    const unsigned int t = gfx_ptrs_pos_to_id[ abs_pos + 1 ];
    --gfx_ptrs_id_to_pos[ t ];
    gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    graphics[ abs_pos + 1 ] = gfx;
    gfx_ptrs_id_to_pos[ id ] = abs_pos + 1;
    gfx_ptrs_pos_to_id[ abs_pos + 1 ] = id;
};

void NasrRaiseGraphicBackwardInLayer( unsigned int id )
{
    const unsigned int rel_pos = NasrGetLayerPosition( id );

    // Skip if already @ back.
    if ( rel_pos == 0 )
    {
        return;
    }

    // Make copy o’ graphic & swap with graphic below.
    NasrGraphic gfx = graphics[ gfx_ptrs_id_to_pos[ id ] ];
    const unsigned int abs_pos = gfx_ptrs_id_to_pos[ id ];
    graphics[ abs_pos ] = graphics[ abs_pos - 1 ];
    const unsigned int t = gfx_ptrs_pos_to_id[ abs_pos - 1 ];
    ++gfx_ptrs_id_to_pos[ t ];
    gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    graphics[ abs_pos - 1 ] = gfx;
    gfx_ptrs_id_to_pos[ id ] = abs_pos - 1;
    gfx_ptrs_pos_to_id[ abs_pos - 1 ] = id;
};

void NasrPlaceGraphicBelowPositionInLayer( unsigned int id, unsigned int pos )
{
    // Skip if already @ position or lower.
    if ( NasrGetLayerPosition( id ) <= pos )
    {
        return;
    }

    // Push forward all graphics between current and target positions.
    const unsigned int layer_index = GetStateLayerIndex( state_for_gfx[ id ], layer_for_gfx[ id ] );
    const unsigned int prev_layer = layer_index > 0 ? layer_index - 1 : 0;
    const unsigned int current_abs_pos = gfx_ptrs_id_to_pos[ id ];
    const unsigned int target_abs_pos = layer_pos[ prev_layer ] + pos;
    const NasrGraphic gfx = graphics[ current_abs_pos ];
    for ( int i = current_abs_pos; i > target_abs_pos; --i )
    {
        graphics[ i ] = graphics[ i - 1 ];

        // Update pointers so they still point to correct graphics.
        const unsigned int t = gfx_ptrs_pos_to_id[ i - 1 ];
        ++gfx_ptrs_id_to_pos[ t ];
        gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    }

    graphics[ target_abs_pos ] = gfx;
    gfx_ptrs_id_to_pos[ id ] = target_abs_pos;
    gfx_ptrs_pos_to_id[ target_abs_pos ] = id;
};

void NasrPlaceGraphicAbovePositionInLayer( unsigned int id, unsigned int pos )
{
    // Skip if already @ position or ’bove.
    if ( NasrGetLayerPosition( id ) >= pos )
    {
        return;
    }

    // Push back all graphics between current and target positions.
    const unsigned int layer_index = GetStateLayerIndex( state_for_gfx[ id ], layer_for_gfx[ id ] );
    const unsigned int prev_layer = layer_index > 0 ? layer_index - 1 : 0;
    const unsigned int current_abs_pos = gfx_ptrs_id_to_pos[ id ];
    const unsigned int target_abs_pos = layer_pos[ prev_layer ] + pos - 1;
    const NasrGraphic gfx = graphics[ current_abs_pos ];
    for ( unsigned int i = current_abs_pos; i < target_abs_pos; ++i )
    {
        graphics[ i ] = graphics[ i + 1 ];

        // Update pointers so they still point to correct graphics.
        const unsigned int t = gfx_ptrs_pos_to_id[ i + 1 ];
        --gfx_ptrs_id_to_pos[ t ];
        gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    }

    // Move copied graphic to front o’ layer & update pointers.
    graphics[ target_abs_pos ] = gfx;
    gfx_ptrs_id_to_pos[ id ] = target_abs_pos;
    gfx_ptrs_pos_to_id[ target_abs_pos ] = id;
};

unsigned int NasrGetLayer( unsigned int id )
{
    return layer_for_gfx[ id ];
};

unsigned int NasrGetLayerPosition( unsigned int id )
{
    const unsigned int state = state_for_gfx[ id ];
    const unsigned int layer_index = GetStateLayerIndex( state, layer_for_gfx[ id ] );
    const unsigned int abs_pos = gfx_ptrs_id_to_pos[ id ];
    const unsigned int prev_layer = layer_index > 0 ? layer_index - 1 : 0;
    const unsigned int layer_start = layer_pos[ prev_layer ];
    return abs_pos - layer_start;
};

unsigned int NasrNumOGraphicsInLayer( unsigned int state, unsigned int layer )
{
    const unsigned int layer_index = GetStateLayerIndex( state, layer );
    const unsigned int prev_layer = layer_index > 0 ? layer_index - 1 : 0;
    return layer_pos[ layer_index ] - layer_pos[ prev_layer ];
};



// Graphics
int NasrGraphicsAddCanvas
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrColor color
)
{
    return NasrGraphicsAddRect
    (
        scrollx,
        scrolly,
        state,
        layer,
        canvas,
        color
    );
};

int NasrGraphicsAddRect
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrRect rect,
    NasrColor color
)
{
    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_RECT;
    graphic.data.rect.rect = rect;
    graphic.data.rect.color = color;
    int id = AddGraphic( state, layer, graphic );
    if ( id > -1 ) {
        ResetVertices( GetVertices( id ) );
        SetVerticesColors( id, &graphic.data.rect.color, &graphic.data.rect.color, &graphic.data.rect.color, &graphic.data.rect.color );
    }
    return id;
};

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
)
{
    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_RECT_GRADIENT;
    graphic.data.gradient.rect = rect;
    graphic.data.gradient.dir = dir;
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color2 = color2;
            graphic.data.gradient.color3 = graphic.data.gradient.color4 = color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color3 = graphic.data.gradient.color4 = color1;
            graphic.data.gradient.color2 = color2;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color2 = graphic.data.gradient.color4 = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color2 = graphic.data.gradient.color4 = color1;
            graphic.data.gradient.color3 = color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color2 = color1;
            graphic.data.gradient.color3 = graphic.data.gradient.color4 = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color2 = graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color2;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            graphic.data.gradient.color1 = graphic.data.gradient.color3 = color2;
            graphic.data.gradient.color2 = graphic.data.gradient.color4 = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            graphic.data.gradient.color1 = color2;
            graphic.data.gradient.color2 = graphic.data.gradient.color3 = graphic.data.gradient.color4 = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsAddRectGradient! %d\n", dir );

            // Default direction.
            graphic.data.gradient.color1 = graphic.data.gradient.color2 = color2;
            graphic.data.gradient.color3 = graphic.data.gradient.color4 = color1;
        }
        break;
    }
    const int id = AddGraphic( state, layer, graphic );
    if ( id > -1 )
    {
        ResetVertices( GetVertices( id ) );
        SetVerticesColors( id, &graphic.data.gradient.color1, &graphic.data.gradient.color2, &graphic.data.gradient.color3, &graphic.data.gradient.color4 );
    }
    return id;
};

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
)
{
    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_RECT_PAL;
    graphic.data.rectpal.rect = rect;
    graphic.data.rectpal.palette = palette;
    graphic.data.rectpal.color1 = graphic.data.rectpal.color2 = color;
    graphic.data.rectpal.useglobalpal = useglobalpal;
    graphic.data.rectpal.opacity = opacity;
    const int id = AddGraphic( state, layer, graphic );
    if ( id >= 0 )
    {
        GraphicsUpdateRectPalette( id, color );
    }
    return id;
};

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
)
{
    uint_fast8_t c[ 4 ];

    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            c[ 0 ] = c[ 1 ] = color2;
            c[ 2 ] = c[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            c[ 0 ] = c[ 2 ] = c[ 3 ] = color1;
            c[ 1 ] = color2;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            c[ 0 ] = c[ 2 ] = color1;
            c[ 1 ] = c[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            c[ 0 ] = c[ 1 ] = c[ 2 ] = color1;
            c[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            c[ 0 ] = c[ 1 ] = color1;
            c[ 2 ] = c[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            c[ 0 ] = c[ 1 ] = c[ 3 ] = color1;
            c[ 2 ] = color2;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            c[ 0 ] = c[ 2 ] = color2;
            c[ 1 ] = c[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            c[ 0 ] = color2;
            c[ 1 ] = c[ 2 ] = c[ 3 ] = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsAddRectGradientPalette! %d\n", dir );

            // Default direction.
            c[ 0 ] = c[ 1 ] = color2;
            c[ 2 ] = c[ 3 ] = color1;
        }
        break;
    }

    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_RECT_PAL;
    graphic.data.rectpal.rect = rect;
    graphic.data.rectpal.palette = palette;
    graphic.data.rectpal.color1 = color1;
    graphic.data.rectpal.color2 = color2;
    graphic.data.rectpal.useglobalpal = useglobalpal;
    graphic.data.rectpal.opacity = opacity;
    const int id = AddGraphic( state, layer, graphic );
    if ( id > -1 )
    {
        GraphicsRectGradientPaletteUpdateColors( id, c );
    }
    return id;
};

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
    unsigned char palette,
    int_fast8_t useglobalpal,
    float tilingx,
    float tilingy
)
{
    #ifdef NASR_SAFE
        if ( texture >= texture_count )
        {
            NasrLog( "NasrGraphicsAddSprite Error: invalid texture #%u.", texture );
            return -1;
        }
    #endif
    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_SPRITE;
    graphic.data.sprite.texture = texture;
    graphic.data.sprite.src = src;
    graphic.data.sprite.dest = dest;
    graphic.data.sprite.flip_x = flip_x;
    graphic.data.sprite.flip_y = flip_y;
    graphic.data.sprite.rotation_x = rotation_x;
    graphic.data.sprite.rotation_y = rotation_y;
    graphic.data.sprite.rotation_z = rotation_z;
    graphic.data.sprite.opacity = opacity;
    graphic.data.sprite.palette = palette;
    graphic.data.sprite.useglobalpal = useglobalpal;
    graphic.data.sprite.tilingx = tilingx;
    graphic.data.sprite.tilingy = tilingy;
    const int id = AddGraphic( state, layer, graphic );

    if ( id > -1 )
    {
        ResetVertices( GetVertices( id ) );

        NasrGraphic * g = GetGraphic( id );
        mat4 model = BASE_MATRIX;
        memcpy( &g->data.sprite.model, &model, sizeof( model ) );
        vec3 scale = { dest.w, dest.h, 0.0 };
        glm_scale( g->data.sprite.model, scale );
        vec3 xrot = { 0.0, 1.0, 0.0 };
        glm_rotate( g->data.sprite.model, DEGREES_TO_RADIANS( rotation_x ), xrot );
        vec3 yrot = { 0.0, 0.0, 1.0 };
        glm_rotate( g->data.sprite.model, DEGREES_TO_RADIANS( rotation_y ), yrot );
        vec3 zrot = { 1.0, 0.0, 0.0 };
        glm_rotate( g->data.sprite.model, DEGREES_TO_RADIANS( rotation_z ), zrot );

        UpdateSpriteVertices( id );
    }
    return id;
};

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
)
{
    // Generate texture from tile data.
    unsigned char * data = ( unsigned char * )( calloc( w * h * 4, sizeof( unsigned char ) ) );
    if ( data == NULL )
    {
        NasrLog( "Couldn’t generate tilemap." );
        return -1;
    }
    int i4 = 0;
    for ( int i = 0; i < w * h; ++i )
    {
        data[ i4 ] = tiles[ i ].x;
        data[ i4 + 1 ] = tiles[ i ].y;
        data[ i4 + 2 ] = tiles[ i ].palette;
        data[ i4 + 3 ] = tiles[ i ].animation;
        i4 += 4;
    }
    const int tilemap_texture = NasrAddTextureEx( data, w, h, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );

    if ( tilemap_texture < 0 )
    {
        free( data );
        return -1;
    }

    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_TILEMAP;
    graphic.data.tilemap.texture = texture;
    graphic.data.tilemap.tilemap = ( unsigned int )( tilemap_texture );
    graphic.data.tilemap.src.x = 0.0f;
    graphic.data.tilemap.src.y = 0.0f;
    graphic.data.tilemap.src.w = ( float )( w );
    graphic.data.tilemap.src.h = ( float )( h );
    graphic.data.tilemap.dest.x = 0.0f;
    graphic.data.tilemap.dest.y = 0.0f;
    graphic.data.tilemap.dest.w = ( float )( w ) * 16.0f;
    graphic.data.tilemap.dest.h = ( float )( h ) * 16.0f;
    graphic.data.tilemap.useglobalpal = useglobalpal;
    graphic.data.tilemap.opacity = opacity;
    graphic.data.tilemap.data = data;
    graphic.data.tilemap.tilingx = tilingx;
    graphic.data.tilemap.tilingy = tilingy;
    const int id = AddGraphic( state, layer, graphic );
    if ( id > -1 )
    {
        BindBuffers( id );
        float * vptr = GetVertices( id );
        ResetVertices( vptr );
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ tilemap_texture ].width ) * graphic.data.tilemap.src.x; // Left X
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ tilemap_texture ].width ) * ( graphic.data.tilemap.src.x + graphic.data.tilemap.src.w );  // Right X
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / ( float )( textures[ tilemap_texture ].height ) * ( graphic.data.tilemap.src.y + graphic.data.tilemap.src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ tilemap_texture ].height ) * graphic.data.tilemap.src.y;  // Bottom Y
        BufferVertices( vptr );
        ClearBufferBindings();
    }
    return id;
}

int NasrGraphicsAddText
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    NasrColor color
)
{
    return GraphicAddText
    (
        scrollx,
        scrolly,
        state,
        layer,
        text,
        &color,
        &color,
        &color,
        &color,
        0,
        NASR_PALETTE_NONE
    );
};

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
)
{
    NasrColor top_left_color;
    NasrColor top_right_color;
    NasrColor bottom_left_color;
    NasrColor bottom_right_color;
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            top_left_color = color2;
            top_right_color = color2;
            bottom_left_color = color1;
            bottom_right_color = color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            top_left_color = color1;
            top_right_color = color2;
            bottom_left_color = color1;
            bottom_right_color = color1;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            top_left_color = color1;
            top_right_color = color2;
            bottom_left_color = color1;
            bottom_right_color = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            top_left_color = color1;
            top_right_color = color1;
            bottom_left_color = color1;
            bottom_right_color = color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            top_left_color = color1;
            top_right_color = color1;
            bottom_left_color = color2;
            bottom_right_color = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            top_left_color = color1;
            top_right_color = color1;
            bottom_left_color = color2;
            bottom_right_color = color1;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            top_left_color = color2;
            top_right_color = color1;
            bottom_left_color = color2;
            bottom_right_color = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            top_left_color = color2;
            top_right_color = color1;
            bottom_left_color = color1;
            bottom_right_color = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsAddTextGradient! %d\n", dir );

            // Default direction.
            top_left_color = color2;
            top_right_color = color2;
            bottom_left_color = color1;
            bottom_right_color = color1;
        }
        break;
    }
    return GraphicAddText
    (
        scrollx,
        scrolly,
        state,
        layer,
        text,
        &top_left_color,
        &top_right_color,
        &bottom_left_color,
        &bottom_right_color,
        0,
        NASR_PALETTE_NONE
    );
};

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
)
{
    NasrColor c =
    {
        ( float )( color ),
        0.0f,
        0.0f,
        255.0f
    };
    return GraphicAddText
    (
        scrollx,
        scrolly,
        state,
        layer,
        text,
        &c,
        &c,
        &c,
        &c,
        palette,
        useglobalpal ? NASR_PALETTE_DEFAULT : NASR_PALETTE_SET
    );
};

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
)
{
    uint_fast8_t c[4];

    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            c[ 0 ] = color2;
            c[ 1 ] = color2;
            c[ 2 ] = color1;
            c[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            c[ 0 ] = color1;
            c[ 1 ] = color2;
            c[ 2 ] = color1;
            c[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            c[ 0 ] = color1;
            c[ 1 ] = color2;
            c[ 2 ] = color1;
            c[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            c[ 0 ] = color1;
            c[ 1 ] = color1;
            c[ 2 ] = color1;
            c[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            c[ 0 ] = color1;
            c[ 1 ] = color1;
            c[ 2 ] = color2;
            c[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            c[ 0 ] = color1;
            c[ 1 ] = color1;
            c[ 2 ] = color2;
            c[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            c[ 0 ] = color2;
            c[ 1 ] = color1;
            c[ 2 ] = color2;
            c[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            c[ 0 ] = color2;
            c[ 1 ] = color1;
            c[ 2 ] = color1;
            c[ 3 ] = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsAddTextGradientPalette! %d\n", dir );

            // Default direction.
            c[ 0 ] = color2;
            c[ 1 ] = color2;
            c[ 2 ] = color1;
            c[ 3 ] = color1;
        }
        break;
    }

    NasrColor cobj[ 4 ];
    for ( int i = 0; i < 4; ++i )
    {
        cobj[ i ].r = ( float )( c[ i ] );
        cobj[ i ].a = 255.0f;
    }

    return GraphicAddText
    (
        scrollx,
        scrolly,
        state,
        layer,
        text,
        &cobj[ 0 ],
        &cobj[ 1 ],
        &cobj[ 2 ],
        &cobj[ 3 ],
        palette,
        useglobalpal ? NASR_PALETTE_DEFAULT : NASR_PALETTE_SET
    );
};

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
)
{
    NasrColor * colors[ 4 ] = {
        &color,
        &color,
        &color,
        &color
    };
    return GraphicsAddCounter
    (
        scrollx,
        scrolly,
        state,
        layer,
        charset,
        num,
        maxdigits,
        maxdecimals,
        x,
        y,
        shadow,
        opacity,
        numpadding,
        decimalpadding,
        colors,
        0,
        NASR_PALETTE_NONE
    );
};



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
)
{
    NasrColor * colors[ 4 ];
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            colors[ 0 ] = &color2;
            colors[ 1 ] = &color2;
            colors[ 2 ] = &color1;
            colors[ 3 ] = &color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            colors[ 0 ] = &color1;
            colors[ 1 ] = &color2;
            colors[ 2 ] = &color1;
            colors[ 3 ] = &color1;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            colors[ 0 ] = &color1;
            colors[ 1 ] = &color2;
            colors[ 2 ] = &color1;
            colors[ 3 ] = &color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            colors[ 0 ] = &color1;
            colors[ 1 ] = &color1;
            colors[ 2 ] = &color1;
            colors[ 3 ] = &color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            colors[ 0 ] = &color1;
            colors[ 1 ] = &color1;
            colors[ 2 ] = &color2;
            colors[ 3 ] = &color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            colors[ 0 ] = &color1;
            colors[ 1 ] = &color1;
            colors[ 2 ] = &color2;
            colors[ 3 ] = &color1;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            colors[ 0 ] = &color2;
            colors[ 1 ] = &color1;
            colors[ 2 ] = &color2;
            colors[ 3 ] = &color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            colors[ 0 ] = &color2;
            colors[ 1 ] = &color1;
            colors[ 2 ] = &color1;
            colors[ 3 ] = &color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsAddCounterGradient! %d\n", dir );

            // Default direction.
            colors[ 0 ] = &color2;
            colors[ 1 ] = &color2;
            colors[ 2 ] = &color1;
            colors[ 3 ] = &color1;
        }
        break;
    }
    return GraphicsAddCounter
    (
        scrollx,
        scrolly,
        state,
        layer,
        charset,
        num,
        maxdigits,
        maxdecimals,
        x,
        y,
        shadow,
        opacity,
        numpadding,
        decimalpadding,
        colors,
        0,
        NASR_PALETTE_NONE
    );
};

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
)
{
    NasrColor c =
    {
        ( float )( color ),
        0.0f,
        0.0f,
        255.0f
    };
    NasrColor * colors[ 4 ] = { &c, &c, &c, &c };
    return GraphicsAddCounter
    (
        scrollx,
        scrolly,
        state,
        layer,
        charset,
        num,
        maxdigits,
        maxdecimals,
        x,
        y,
        shadow,
        opacity,
        numpadding,
        decimalpadding,
        colors,
        palette,
        useglobalpal ? NASR_PALETTE_DEFAULT : NASR_PALETTE_SET
    );
};

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
)
{
    uint_fast8_t colors[ 4 ];
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            colors[ 0 ] = color2;
            colors[ 1 ] = color2;
            colors[ 2 ] = color1;
            colors[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            colors[ 0 ] = color1;
            colors[ 1 ] = color2;
            colors[ 2 ] = color1;
            colors[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            colors[ 0 ] = color1;
            colors[ 1 ] = color2;
            colors[ 2 ] = color1;
            colors[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            colors[ 0 ] = color1;
            colors[ 1 ] = color1;
            colors[ 2 ] = color1;
            colors[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            colors[ 0 ] = color1;
            colors[ 1 ] = color1;
            colors[ 2 ] = color2;
            colors[ 3 ] = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            colors[ 0 ] = color1;
            colors[ 1 ] = color1;
            colors[ 2 ] = color2;
            colors[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            colors[ 0 ] = color2;
            colors[ 1 ] = color1;
            colors[ 2 ] = color2;
            colors[ 3 ] = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            colors[ 0 ] = color2;
            colors[ 1 ] = color1;
            colors[ 2 ] = color1;
            colors[ 3 ] = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsAddCounterGradient! %d\n", dir );

            // Default direction.
            colors[ 0 ] = color2;
            colors[ 1 ] = color2;
            colors[ 2 ] = color1;
            colors[ 3 ] = color1;
        }
        break;
    }

    NasrColor colorsfull[ 4 ];
    NasrColor * colorsfinal[ 4 ];
    for ( int i = 0; i < 4; ++i )
    {
        colorsfull[ i ].r = ( float )( colors[ i ] );
        colorsfull[ i ].a = 255.0f;
        colorsfinal[ i ] = &colorsfull[ i ];
    }
    return GraphicsAddCounter
    (
        scrollx,
        scrolly,
        state,
        layer,
        charset,
        num,
        maxdigits,
        maxdecimals,
        x,
        y,
        shadow,
        opacity,
        numpadding,
        decimalpadding,
        colorsfinal,
        palette,
        useglobalpal ? NASR_PALETTE_DEFAULT : NASR_PALETTE_SET
    );
};

void NasrGraphicsRemove( unsigned int id )
{
    // Clean up graphic.
    const unsigned int pos = gfx_ptrs_id_to_pos[ id ];
    DestroyGraphic( &graphics[ pos ] );

    // Push down all graphics ’bove
    for ( unsigned int i = pos; i < num_o_graphics - 1; ++i )
    {
        graphics[ i ] = graphics[ i + 1 ];

        // Update pointers so they still point to correct graphics.
        const unsigned int t = gfx_ptrs_pos_to_id[ i + 1 ];
        --gfx_ptrs_id_to_pos[ t ];
        gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    }

    // Decrease this & following layers.
    const unsigned int current_layer_index = GetStateLayerIndex( state_for_gfx[ id ], layer_for_gfx[ id ] );
    for ( unsigned int i = current_layer_index; i < max_gfx_layers * max_states; ++i )
    {
        --layer_pos[ i ];
    }

    --num_o_graphics;

    gfx_ptrs_pos_to_id[ num_o_graphics ] = gfx_ptrs_id_to_pos[ id ] = state_for_gfx[ id ] = layer_for_gfx[ id ] = -1;
};

void NasrGraphicsClearState( unsigned int state )
{
    unsigned int pp = ( state * max_gfx_layers ) - 1;
    unsigned int p = layer_pos[ pp ];

    while ( p < num_o_graphics )
    {
        NasrGraphicsRemove( gfx_ptrs_pos_to_id[ p ] );
    }
};

void NasrClearGraphics( void )
{
    // Destroy specific graphic objects.
    for ( unsigned int i = 0; i < num_o_graphics; ++i )
    {
        DestroyGraphic( &graphics[ i ] );
    }

    // Reset all vertices.
    for ( unsigned int i = 0; i < max_graphics + 1; ++i )
    {
        float * vptr = GetVertices( i );
        ResetVertices( vptr );
        BindBuffers( i );
        BufferDefault( vptr );
    }
    ClearBufferBindings();

    // Reset maps to null values ( since 0 is a valid value, we use -1 ).
    for ( unsigned int i = 0; i < max_graphics; ++i )
    {
        gfx_ptrs_id_to_pos[ i ] = gfx_ptrs_pos_to_id[ i ] = state_for_gfx[ i ] = layer_for_gfx[ i ] = -1;
    }

    for ( unsigned int i = 0; i < max_states * max_gfx_layers; ++i )
    {
        layer_pos[ i ] = 0;
    }

    num_o_graphics = 0;
};



// SpriteGraphics Manipulation
NasrRect NasrGraphicsSpriteGetDest( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetDest Error: invalid id %u", id );
            NasrRect r = { NAN, NAN, NAN, NAN };
            return r;
        }
    #endif
    return GetGraphic( id )->data.sprite.dest;
};

void NasrGraphicsSpriteSetDest( unsigned int id, NasrRect v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetDest Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest = v;
    UpdateSpriteModel( id );
};

float NasrGraphicsSpriteGetDestY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetDestY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.dest.y;
};

void NasrGraphicsSpriteSetDestY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetDestY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.y = v;
};

void NasrGraphicsSpriteAddToDestY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToDestY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.y += v;
};

float NasrGraphicsSpriteGetDestX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetDestX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.dest.x;
};

void NasrGraphicsSpriteSetDestX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetDestX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.x = v;
};

void NasrGraphicsSpriteAddToDestX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToDestX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.x += v;
};

float NasrGraphicsSpriteGetDestW( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetDestW Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.dest.w;
};

void NasrGraphicsSpriteSetDestW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetDestW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.w = v;
    UpdateSpriteModel( id );
};

void NasrGraphicsSpriteAddToDestW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToDestW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.w += v;
    UpdateSpriteModel( id );
};

float NasrGraphicsSpriteGetDestH( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetDestH Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.dest.h;
};

void NasrGraphicsSpriteSetDestH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetDestH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.h = v;
    UpdateSpriteModel( id );
};

void NasrGraphicsSpriteAddToDestH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToDestH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.dest.h += v;
    UpdateSpriteModel( id );
};

float NasrGraphicsSpriteGetSrcX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetSrcX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.src.x;
};

void NasrGraphicsSpriteSetSrcX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetSrcX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.x = v;
    UpdateSpriteVertices( id );
};

void NasrGraphicsSpriteAddToSrcX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToSrcX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.x += v;
};

float NasrGraphicsSpriteGetSrcY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetSrcY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.src.y;
};

void NasrGraphicsSpriteSetSrcY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetSrcY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.y = v;
    UpdateSpriteVertices( id );
};

void NasrGraphicsSpriteAddToSrcY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToSrcY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.y += v;
};

float NasrGraphicsSpriteGetSrcW( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetSrcW Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.src.w;
};

void NasrGraphicsSpriteSetSrcW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetSrcW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.w = v;
    UpdateSpriteVertices( id );
};

void NasrGraphicsSpriteAddToSrcW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToSrcW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.w += v;
};

float NasrGraphicsSpriteGetSrcH( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetSrcH Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.src.h;
};

void NasrGraphicsSpriteSetSrcH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetSrcH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.h = v;
    UpdateSpriteVertices( id );
};

void NasrGraphicsSpriteAddToSrcH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToSrcH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.src.h += v;
};

float NasrGraphicsSpriteGetRotationX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetRotationX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.rotation_x;
};

void NasrGraphicsSpriteSetRotationX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetRotationX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.rotation_x = v;
    UpdateSpriteModel( id );
};

void NasrGraphicsSpriteAddToRotationX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToRotationX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.rotation_x += v;
    UpdateSpriteModel( id );
};

float NasrGraphicsSpriteGetRotationY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetRotationY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.rotation_y;
};

void NasrGraphicsSpriteSetRotationY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetRotationY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.rotation_y = v;
    UpdateSpriteModel( id );
};

void NasrGraphicsSpriteAddToRotationY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToRotationY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.rotation_y += v;
    UpdateSpriteModel( id );
};

float NasrGraphicsSpriteGetRotationZ( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetRotationZ Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.rotation_z;
};

void NasrGraphicsSpriteSetRotationZ( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetRotationZ Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.rotation_z = v;
    UpdateSpriteModel( id );
};

void NasrGraphicsSpriteAddToRotationZ( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToRotationZ Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.rotation_z += v;
    UpdateSpriteModel( id );
};

uint_fast8_t NasrGraphicsSpriteGetPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetPalette Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.sprite.palette;
};

void NasrGraphicsSpriteSetPalette( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetPalette Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.palette = v;
};

void NasrGraphicsSpriteIncrementPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteIncrementPalette Error: invalid id %u", id );
            return;
        }
    #endif
    ++GetGraphic( id )->data.sprite.palette;
};

void NasrGraphicsSpriteDecrementPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteDecrementPalette Error: invalid id %u", id );
            return;
        }
    #endif
    --GetGraphic( id )->data.sprite.palette;
};

float NasrGraphicsSpriteGetOpacity( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetOpacity Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.sprite.opacity;
};

void NasrGraphicsSpriteSetOpacity( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetOpacity Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.opacity = v;
};

void NasrGraphicsSpriteAddToOpacity( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteAddToOpacity Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.opacity += v;
};

uint_fast8_t NasrGraphicsSpriteGetFlipX( unsigned id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetFlipX Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.sprite.flip_x;
};

void NasrGraphicsSpriteSetFlipX( unsigned id, int v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetFlipX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.flip_x = v;
    UpdateSpriteX( id );
};

void NasrGraphicsSpriteFlipX( unsigned id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteFlipX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.flip_x = !GetGraphic( id )->data.sprite.flip_x;
    UpdateSpriteX( id );
};

uint_fast8_t NasrGraphicsSpriteGetFlipY( unsigned id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetFlipY Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.sprite.flip_y;
};

void NasrGraphicsSpriteSetFlipY( unsigned id, int v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetFlipY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.flip_y = v;
    UpdateSpriteY( id );
};

void NasrGraphicsSpriteFlipY( unsigned id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteFlipY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.flip_y = !GetGraphic( id )->data.sprite.flip_y;
    UpdateSpriteY( id );
};

unsigned int NasrGraphicsSpriteGetTexture( unsigned id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteGetTexture Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.sprite.texture;
};

void NasrGraphicsSpriteSetTexture( unsigned id, unsigned int v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsSpriteSetTexture Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.sprite.texture = v;
};



// RectGraphics Manipulation
float NasrGraphicsRectGetX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGetX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rect.rect.x;
};

void NasrGraphicsRectSetX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.x = v;
};

void NasrGraphicsRectAddToX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectAddToX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.x += v;
};

float NasrGraphicsRectGetY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGetY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rect.rect.y;
};

void NasrGraphicsRectSetY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.y = v;
};

void NasrGraphicsRectAddToY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectAddToY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.y += v;
};

float NasrGraphicsRectGetW( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGetW Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rect.rect.w;
};

void NasrGraphicsRectSetW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.w = v;
};

void NasrGraphicsRectAddToW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectAddToW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.w += v;
};

float NasrGraphicsRectGetH( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGetH Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rect.rect.h;
};

void NasrGraphicsRectSetH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.h = v;
};

void NasrGraphicsRectAddToH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectAddToH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rect.rect.h += v;
};

void NasrGraphicsRectSetColor( unsigned int id, NasrColor v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetColor Error: invalid id %u", id );
            return;
        }
    #endif
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 4 ] = vptr[ 4 + VERTEX_SIZE ] = vptr[ 4 + VERTEX_SIZE * 2 ] = vptr[ 4 + VERTEX_SIZE * 3 ] = v.r / 255.0f;
    vptr[ 5 ] = vptr[ 5 + VERTEX_SIZE ] = vptr[ 5 + VERTEX_SIZE * 2 ] = vptr[ 5 + VERTEX_SIZE * 3 ] = v.g / 255.0f;
    vptr[ 6 ] = vptr[ 6 + VERTEX_SIZE ] = vptr[ 6 + VERTEX_SIZE * 2 ] = vptr[ 6 + VERTEX_SIZE * 3 ] = v.b / 255.0f;
    vptr[ 7 ] = vptr[ 7 + VERTEX_SIZE ] = vptr[ 7 + VERTEX_SIZE * 2 ] = vptr[ 7 + VERTEX_SIZE * 3 ] = v.a / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicsRectSetColorR( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetColorR Error: invalid id %u", id );
            return;
        }
    #endif
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 4 ] = vptr[ 4 + VERTEX_SIZE ] = vptr[ 4 + VERTEX_SIZE * 2 ] = vptr[ 4 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicsRectSetColorG( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetColorG Error: invalid id %u", id );
            return;
        }
    #endif
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 5 ] = vptr[ 5 + VERTEX_SIZE ] = vptr[ 5 + VERTEX_SIZE * 2 ] = vptr[ 5 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicsRectSetColorB( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetColorB Error: invalid id %u", id );
            return;
        }
    #endif
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 6 ] = vptr[ 6 + VERTEX_SIZE ] = vptr[ 6 + VERTEX_SIZE * 2 ] = vptr[ 6 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicsRectSetColorA( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectSetColorA Error: invalid id %u", id );
            return;
        }
    #endif
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 7 ] = vptr[ 7 + VERTEX_SIZE ] = vptr[ 7 + VERTEX_SIZE * 2 ] = vptr[ 7 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};



// RectGraphicsGradient Manipulation
float NasrGraphicsRectGradientGetX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientGetX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.gradient.rect.x;
};

void NasrGraphicsRectGradientSetX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.x = v;
};

void NasrGraphicsRectGradientAddToX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientAddToX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.x += v;
};

float NasrGraphicsRectGradientGetY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientGetY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.gradient.rect.y;
};

void NasrGraphicsRectGradientSetY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.y = v;
};

void NasrGraphicsRectGradientAddToY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientAddToY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.y += v;
};

float NasrGraphicsRectGradientGetW( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientGetW Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.gradient.rect.w;
};

void NasrGraphicsRectGradientSetW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.w = v;
};

void NasrGraphicsRectGradientAddToW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientAddToW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.w += v;
};

float NasrGraphicsRectGradientGetH( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientGetH Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.gradient.rect.h;
};

void NasrGraphicsRectGradientSetH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.h = v;
};

void NasrGraphicsRectGradientAddToH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientAddToH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.gradient.rect.h += v;
};

uint_fast8_t NasrGraphicsRectGradientGetDir( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientGetDir Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.gradient.dir;
};

void NasrGraphicsRectGradientSetDir( unsigned int id, uint_fast8_t dir )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetDir Error: invalid id %u", id );
            return;
        }
    #endif

    NasrGraphicRectGradient * r = &GetGraphic( id )->data.gradient;
    NasrColor color1;
    NasrColor color2;

    switch ( r->dir )
    {
        case ( NASR_DIR_UP ):
        {
            color2 = r->color1;
            color1 = r->color3;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            color1 = r->color1;
            color2 = r->color2;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            color1 = r->color1;
            color2 = r->color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            color1 = r->color1;
            color2 = r->color3;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            color1 = r->color1;
            color2 = r->color3;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            color1 = r->color1;
            color2 = r->color4;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            color2 = r->color1;
            color1 = r->color2;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            color2 = r->color1;
            color1 = r->color2;
        }
        break;
        default:
        {
            // Default direction.
            color2 = r->color1;
            color1 = r->color3;
        }
        break;
    }

    r->dir = dir;
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            r->color3 = r->color4 = color1;
            r->color1 = r->color2 = color2;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            r->color1 = r->color3 = r->color4 = color1;
            r->color2 = color2;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            r->color1 = r->color3 = color1;
            r->color2 = r->color4 = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            r->color1 = r->color2 = r->color4 = color1;
            r->color3 = color2;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            r->color1 = r->color2 = color1;
            r->color3 = r->color4 = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            r->color1 = r->color2 = r->color3 = color1;
            r->color4 = color2;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            r->color1 = r->color3 = color2;
            r->color2 = r->color4 = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            r->color1 = color2;
            r->color2 = r->color3 = r->color4 = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsRectGradientSetDir! %d\n", dir );

            // Default direction.
            r->color1 = r->color2 = color2;
            r->color3 = r->color4 = color1;
        }
        break;
    }
    ResetVertices( GetVertices( id ) );
    SetVerticesColors( id, &r->color1, &r->color2, &r->color3, &r->color4 );
};

void NasrGraphicsRectGradientSetColor1( unsigned int id, NasrColor color )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor1 Error: invalid id %u", id );
            return;
        }
    #endif

    NasrGraphicRectGradient * r = &GetGraphic( id )->data.gradient;
    switch ( r->dir )
    {
        case ( NASR_DIR_UP ):
        {
            r->color3 = r->color4 = color;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            r->color1 = r->color3 = r->color4 = color;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            r->color1 = r->color3 = color;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            r->color1 = r->color2 = r->color4 = color;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            r->color1 = r->color2 = color;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            r->color1 = r->color2 = r->color3 = color;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            r->color2 = r->color4 = color;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            r->color1 = r->color2 = r->color3 = r->color4 = color;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsRectGradientSetColor1! %d\n", r->dir );

            // Default direction.
            r->color3 = r->color4 = color;
        }
        break;
    }
    ResetVertices( GetVertices( id ) );
    SetVerticesColors( id, &r->color1, &r->color2, &r->color3, &r->color4 );
};

void NasrGraphicsRectGradientSetColor1R( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor1R Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color1;
    c.r = v;
    NasrGraphicsRectGradientSetColor1( id, c );
};

void NasrGraphicsRectGradientSetColor1G( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor1G Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color1;
    c.g = v;
    NasrGraphicsRectGradientSetColor1( id, c );
};

void NasrGraphicsRectGradientSetColor1B( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor1B Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color1;
    c.b = v;
    NasrGraphicsRectGradientSetColor1( id, c );
};

void NasrGraphicsRectGradientSetColor1A( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor1A Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color1;
    c.a = v;
    NasrGraphicsRectGradientSetColor1( id, c );
};

void NasrGraphicsRectGradientSetColor2( unsigned int id, NasrColor color )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor2 Error: invalid id %u", id );
            return;
        }
    #endif

    NasrGraphicRectGradient * r = &GetGraphic( id )->data.gradient;
    switch ( r->dir )
    {
        case ( NASR_DIR_UP ):
        {
            r->color1 = r->color2 = color;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            r->color2 = color;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            r->color2 = r->color4 = color;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            r->color3 = color;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            r->color3 = r->color4 = color;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            r->color4 = color;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            r->color1 = r->color3 = color;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsRectGradientSetColor2! %d\n", r->dir );

            // Default direction.
            r->color1 = r->color2 = color;
        }
        break;
    }
    ResetVertices( GetVertices( id ) );
    SetVerticesColors( id, &r->color1, &r->color2, &r->color3, &r->color4 );
};

void NasrGraphicsRectGradientSetColor2R( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor2R Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color2;
    c.r = v;
    NasrGraphicsRectGradientSetColor2( id, c );
};

void NasrGraphicsRectGradientSetColor2G( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor2G Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color2;
    c.g = v;
    NasrGraphicsRectGradientSetColor2( id, c );
};

void NasrGraphicsRectGradientSetColor2B( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor2B Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color2;
    c.b = v;
    NasrGraphicsRectGradientSetColor2( id, c );
};

void NasrGraphicsRectGradientSetColor2A( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientSetColor2A Error: invalid id %u", id );
            return;
        }
    #endif
    NasrColor c = GetGraphic( id )->data.gradient.color2;
    c.a = v;
    NasrGraphicsRectGradientSetColor2( id, c );
};



// RectGraphicsPalette Manipulation
float NasrGraphicsRectPaletteGetX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.x;
};

void NasrGraphicsRectPaletteSetX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.x = v;
};

void NasrGraphicsRectPaletteAddToX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteAddToX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.x += v;
};

float NasrGraphicsRectPaletteGetY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.y;
};

void NasrGraphicsRectPaletteSetY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.y = v;
};

void NasrGraphicsRectPaletteAddToY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteAddToY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.y += v;
};

float NasrGraphicsRectPaletteGetW( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetW Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.w;
};

void NasrGraphicsRectPaletteSetW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.w = v;
};

void NasrGraphicsRectPaletteAddToW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteAddToW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.w += v;
};

float NasrGraphicsRectPaletteGetH( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetH Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.h;
};

void NasrGraphicsRectPaletteSetH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.h = v;
};

void NasrGraphicsRectPaletteAddToH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteAddToH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.h += v;
};

uint_fast8_t NasrGraphicsRectPaletteGetPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetPalette Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.rectpal.palette;
};

void NasrGraphicsRectPaletteSetPalette( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetPalette Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.palette = v;
};

void NasrGraphicsRectPaletteIncrementPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteIncrementPalette Error: invalid id %u", id );
            return;
        }
    #endif
    ++GetGraphic( id )->data.rectpal.palette;
};

void NasrGraphicsRectPaletteDecrementPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteDecrementPalette Error: invalid id %u", id );
            return;
        }
    #endif
    --GetGraphic( id )->data.rectpal.palette;
};


uint_fast8_t NasrGraphicsRectPaletteGetColor( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetColor Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.rectpal.color1;
};

void NasrGraphicsRectPaletteSetColor( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetColor Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.color1 =
        GetGraphic( id )->data.rectpal.color2 = v;
    GraphicsUpdateRectPalette( id, v );
};

void NasrGraphicsRectPaletteIncrementColor( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteIncrementColor Error: invalid id %u", id );
            return;
        }
    #endif
    GraphicsUpdateRectPalette( id, ++GetGraphic( id )->data.rectpal.color1 );
};

void NasrGraphicsRectPaletteDecrementColor( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteDecrementColor Error: invalid id %u", id );
            return;
        }
    #endif
    GraphicsUpdateRectPalette( id, --GetGraphic( id )->data.rectpal.color1 );
};

float NasrGraphicsRectPaletteGetOpacity( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteGetOpacity Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.opacity;
};

void NasrGraphicsRectPaletteSetOpacity( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteSetOpacity Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.opacity = v;
};

void NasrGraphicsRectPaletteAddToOpacity( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectPaletteAddToOpacity Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.opacity += v;
};



// RectGraphicsGradientPalette Manipulation
float NasrGraphicsRectGradientPaletteGetX( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetX Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.x;
};

void NasrGraphicsRectGradientPaletteSetX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.x = v;
};

void NasrGraphicsRectGradientPaletteAddToX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteAddToX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.x += v;
};

float NasrGraphicsRectGradientPaletteGetY( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetY Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.y;
};

void NasrGraphicsRectGradientPaletteSetY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.y = v;
};

void NasrGraphicsRectGradientPaletteAddToY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteAddToY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.y += v;
};

float NasrGraphicsRectGradientPaletteGetW( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetW Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.w;
};

void NasrGraphicsRectGradientPaletteSetW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.w = v;
};

void NasrGraphicsRectGradientPaletteAddToW( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteAddToW Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.w += v;
};

float NasrGraphicsRectGradientPaletteGetH( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetH Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.rectpal.rect.h;
};

void NasrGraphicsRectGradientPaletteSetH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.h = v;
};

void NasrGraphicsRectGradientPaletteAddToH( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteAddToH Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.rect.h += v;
};

uint_fast8_t NasrGraphicsRectGradientPaletteGetDir( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetDir Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.rectpal.dir;
};

void NasrGraphicsRectGradientPaletteSetDir( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetDir Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.dir = v;
};

uint_fast8_t NasrGraphicsRectGradientPaletteGetPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetPalette Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.rectpal.palette;
};

void NasrGraphicsRectGradientPaletteSetPalette( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetPalette Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.rectpal.palette = v;
};

void NasrGraphicsRectGradientPaletteIncrementPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteIncrementPalette Error: invalid id %u", id );
            return;
        }
    #endif
    ++GetGraphic( id )->data.rectpal.palette;
};

void NasrGraphicsRectGradientPaletteDecrementPalette( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteDecrementPalette Error: invalid id %u", id );
            return;
        }
    #endif
    --GetGraphic( id )->data.rectpal.palette;
};

uint_fast8_t NasrGraphicsRectGradientPaletteGetColor1( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetColor1 Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.rectpal.color1;
};

void NasrGraphicsRectGradientPaletteSetColor1( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetColor1 Error: invalid id %u", id );
            return;
        }
    #endif

    NasrGraphicRectPalette * r = &GetGraphic( id )->data.rectpal;

    r->color1 = v;
    uint_fast8_t c[ 4 ];

    switch ( r->dir )
    {
        case ( NASR_DIR_UP ):
        {
            c[ 2 ] = c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            c[ 0 ] = c[ 2 ] = c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            c[ 0 ] = c[ 2 ] = v;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            c[ 0 ] = c[ 1 ] = c[ 2 ] = v;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            c[ 0 ] = c[ 1 ] = v;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            c[ 0 ] = c[ 1 ] = c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            c[ 1 ] = c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            c[ 1 ] = c[ 2 ] = c[ 3 ] = v;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsRectGradientPaletteSetColor1! %d\n", r->dir );

            // Default direction.
            c[ 2 ] = c[ 3 ] = v;
        }
        break;
    }

    GraphicsRectGradientPaletteUpdateColors( id, c );
};

void NasrGraphicsRectGradientPaletteIncrementColor1( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteIncrementColor1 Error: invalid id %u", id );
            return;
        }
    #endif
    NasrGraphicsRectGradientPaletteSetColor1( id, GetGraphic( id )->data.rectpal.color1 + 1 );
};

void NasrGraphicsRectGradientPaletteDecrementColor1( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteDecrementColor1 Error: invalid id %u", id );
            return;
        }
    #endif
    NasrGraphicsRectGradientPaletteSetColor1( id, GetGraphic( id )->data.rectpal.color1 - 1 );
};

uint_fast8_t NasrGraphicsRectGradientPaletteGetColor2( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteGetColor2 Error: invalid id %u", id );
            return 0;
        }
    #endif
    return GetGraphic( id )->data.rectpal.color2;
};

void NasrGraphicsRectGradientPaletteSetColor2( unsigned int id, uint_fast8_t v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteSetColor2 Error: invalid id %u", id );
            return;
        }
    #endif

    NasrGraphicRectPalette * r = &GetGraphic( id )->data.rectpal;

    r->color1 = v;
    uint_fast8_t c[ 4 ];

    switch ( r->dir )
    {
        case ( NASR_DIR_UP ):
        {
            c[ 0 ] = c[ 1 ] = v;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            c[ 1 ] = v;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            c[ 1 ] = c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            c[ 2 ] = c[ 3 ] = v;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            c[ 2 ] = v;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            c[ 0 ] = c[ 2 ] = v;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            c[ 0 ] = v;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrGraphicsRectGradientPaletteSetColor2! %d\n", r->dir );

            // Default direction.
            c[ 0 ] = c[ 1 ] = v;
        }
        break;
    }

    GraphicsRectGradientPaletteUpdateColors( id, c );
};

void NasrGraphicsRectGradientPaletteIncrementColor2( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteIncrementColor2 Error: invalid id %u", id );
            return;
        }
    #endif
    GraphicsUpdateRectPalette( id, ++GetGraphic( id )->data.rectpal.color2 );
};

void NasrGraphicsRectGradientPaletteDecrementColor2( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsRectGradientPaletteDecrementColor2 Error: invalid id %u", id );
            return;
        }
    #endif
    GraphicsUpdateRectPalette( id, --GetGraphic( id )->data.rectpal.color2 );
};



// TilemapGraphics Manipulation
void NasrGraphicsTilemapSetX( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetX Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.tilemap.dest.x = v;
};

void NasrGraphicsTilemapSetY( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetY Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.tilemap.dest.y = v;
};

unsigned int NasrGraphicsTilemapGetWidth( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapGetWidth Error: invalid id %u", id );
            return 0;
        }
    #endif
    return textures[ GetGraphic( id )->data.tilemap.tilemap ].width;
};

unsigned int NasrGraphicsTilemapGetHeight( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapGetHeight Error: invalid id %u", id );
            return 0;
        }
    #endif
    return textures[ GetGraphic( id )->data.tilemap.tilemap ].height;
};

void NasrGraphicsTilemapSetTileX( unsigned int id, unsigned int x, unsigned int y, unsigned char v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetTileX Error: invalid id %u", id );
            return;
        }
    #endif

    #define TEX textures[ t->tilemap ]
    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    const unsigned int i = ( y * TEX.width + x ) * 4;
    t->data[ i ] = v;

    glBindTexture( GL_TEXTURE_2D, texture_ids[ t->tilemap ] );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX.width, TEX.height, GL_RGBA, GL_UNSIGNED_BYTE, t->data );
    ClearBufferBindings();

    #undef TEX
};

void NasrGraphicsTilemapSetTileY( unsigned int id, unsigned int x, unsigned int y, unsigned char v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetTileY Error: invalid id %u", id );
            return;
        }
    #endif

    #define TEX textures[ t->tilemap ]
    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    const unsigned int i = ( ( y * TEX.width + x ) * 4 ) + 1;
    t->data[ i ] = v;

    glBindTexture( GL_TEXTURE_2D, texture_ids[ t->tilemap ] );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX.width, TEX.height, GL_RGBA, GL_UNSIGNED_BYTE, t->data );
    ClearBufferBindings();

    #undef TEX
};

void NasrGraphicsTilemapSetTilePalette( unsigned int id, unsigned int x, unsigned int y, unsigned char v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetTilePalette Error: invalid id %u", id );
            return;
        }
    #endif

    #define TEX textures[ t->tilemap ]
    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    const unsigned int i = ( ( y * TEX.width + x ) * 4 ) + 2;
    t->data[ i ] = v;

    glBindTexture( GL_TEXTURE_2D, texture_ids[ t->tilemap ] );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX.width, TEX.height, GL_RGBA, GL_UNSIGNED_BYTE, t->data );
    ClearBufferBindings();

    #undef TEX
};

void NasrGraphicsTilemapSetTileAnimation( unsigned int id, unsigned int x, unsigned int y, unsigned char v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetTileAnimation Error: invalid id %u", id );
            return;
        }
    #endif

    #define TEX textures[ t->tilemap ]
    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    const unsigned int i = ( ( y * TEX.width + x ) * 4 ) + 3;
    t->data[ i ] = v;

    glBindTexture( GL_TEXTURE_2D, texture_ids[ t->tilemap ] );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX.width, TEX.height, GL_RGBA, GL_UNSIGNED_BYTE, t->data );
    ClearBufferBindings();

    #undef TEX
};

void NasrGraphicsTilemapSetTile( unsigned int id, unsigned int x, unsigned int y, NasrTile tile )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetTile Error: invalid id %u", id );
            return;
        }
    #endif

    #define TEX textures[ t->tilemap ]
    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    const unsigned int i = ( ( y * TEX.width + x ) * 4 );
    t->data[ i ]     = tile.x;
    t->data[ i + 1 ] = tile.y;
    t->data[ i + 2 ] = tile.palette;
    t->data[ i + 3 ] = tile.animation;

    glBindTexture( GL_TEXTURE_2D, texture_ids[ t->tilemap ] );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX.width, TEX.height, GL_RGBA, GL_UNSIGNED_BYTE, t->data );
    ClearBufferBindings();

    #undef TEX
};

void NasrGraphicsTilemapClearTile( unsigned int id, unsigned int x, unsigned int y )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapClearTile Error: invalid id %u", id );
            return;
        }
    #endif

    #define TEX textures[ t->tilemap ]
    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    const unsigned int i = ( ( y * TEX.width + x ) * 4 ) + 3;
    t->data[ i ] = 255;

    glBindTexture( GL_TEXTURE_2D, texture_ids[ t->tilemap ] );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX.width, TEX.height, GL_RGBA, GL_UNSIGNED_BYTE, t->data );
    ClearBufferBindings();

    #undef TEX
};

float NasrGraphicsTilemapGetOpacity( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapGetOpacity Error: invalid id %u", id );
            return 0.0f;
        }
    #endif

    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    return t->opacity;
};

void NasrGraphicsTilemapSetOpacity( unsigned int id, float opacity )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTilemapSetOpacity Error: invalid id %u", id );
            return;
        }
    #endif

    NasrGraphicTilemap * t = &GetGraphic( id )->data.tilemap;
    t->opacity = opacity;
};



// TextGraphics Manipulation
float NasrGraphicsTextGetXOffset( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextGetXOffset Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.text.xoffset;
};

void NasrGraphicsTextSetXOffset( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextSetXOffset Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.text.xoffset = v;
};

void NasrGraphicsTextAddToXOffset( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextAddToXOffset Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.text.xoffset += v;
};

float NasrGraphicsTextGetYOffset( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextGetYOffset Error: invalid id %u", id );
            return NAN;
        }
    #endif
    return GetGraphic( id )->data.text.yoffset;
};

void NasrGraphicsTextSetYOffset( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextSetYOffset Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.text.yoffset = v;
};

void NasrGraphicsTextAddToYOffset( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextAddToYOffset Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.text.yoffset += v;
};

void NasrGraphicsTextSetCount( unsigned int id, int count )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextSetCount Error: invalid id %u", id );
            return;
        }
    #endif
    NasrGraphicText * t = &GetGraphic( id )->data.text;
    t->count = NASR_MATH_MIN( count, t->capacity );
};

void NasrGraphicsTextIncrementCount( unsigned int id )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsTextIncrementCount Error: invalid id %u", id );
            return;
        }
    #endif
    NasrGraphicText * t = &GetGraphic( id )->data.text;
    t->count = NASR_MATH_MIN( t->count + 1, t->capacity );
};

void NasrSetTextOpacity( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrSetTextOpacity Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.text.opacity = v;
};



// CounterGraphics Manipulation
void NasrGraphicsCounterSetNumber( unsigned int id, float n )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsCounterSetNumber Error: invalid id %u", id );
            return;
        }
    #endif
    NasrGraphic * g = GetGraphic( id );
    if ( !g )
    {
        return;
    }
    n = NASR_MATH_MIN( n, g->data.counter->maxnum );
    int intnum = ( int )( floor( n ) );
    int maxdigits = g->data.counter->maxdigits;
    for ( int i = 0; i < g->data.counter->count; ++i )
    {
        float * vptr = &g->data.counter->vertices[ i * VERTEX_RECT_SIZE ];

        const int c = i == maxdigits ? 10 : ( i > maxdigits ) ? NasrGetDigit( ( int )( floor( n * pow( 10, i - maxdigits ) ) ), 1 ) : NasrGetDigit( intnum, maxdigits - i );
        CharNum * character = &charmaps.list[ g->data.counter->charset ].nums[ c ];

        g->data.counter->chars[ i ].src = character->src;
        g->data.counter->chars[ i ].dest.x = ( charmaps.list[ g->data.counter->charset ].numwidth * i ) + character->xoffset;
        g->data.counter->chars[ i ].dest.y = character->yoffset;
        g->data.counter->chars[ i ].dest.w = character->src.w;
        g->data.counter->chars[ i ].dest.h = character->src.h;

        glBindVertexArray( g->data.counter->vaos[ i ] );
        glBindBuffer( GL_ARRAY_BUFFER, g->data.counter->vbos[ i ] );

        ResetVertices( vptr );
        const float texturew = ( float )( charmaps.list[ g->data.counter->charset ].texture.width );
        const float textureh = ( float )( charmaps.list[ g->data.counter->charset ].texture.height );
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / texturew * character->src.x; // Left X
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / texturew * ( character->src.x + character->src.w );  // Right X
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / textureh * ( character->src.y + character->src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / textureh * character->src.y;  // Bottom Y

        vptr[ 4 ] = g->data.counter->colors[ 3 ].r / 255.0f;
        vptr[ 5 ] = g->data.counter->colors[ 3 ].g / 255.0f;
        vptr[ 6 ] = g->data.counter->colors[ 3 ].b / 255.0f;
        vptr[ 7 ] = g->data.counter->colors[ 3 ].a / 255.0f;

        vptr[ 4 + VERTEX_SIZE ] = g->data.counter->colors[ 1 ].r / 255.0f;
        vptr[ 5 + VERTEX_SIZE ] = g->data.counter->colors[ 1 ].g / 255.0f;
        vptr[ 6 + VERTEX_SIZE ] = g->data.counter->colors[ 1 ].b / 255.0f;
        vptr[ 7 + VERTEX_SIZE ] = g->data.counter->colors[ 1 ].a / 255.0f;

        vptr[ 4 + VERTEX_SIZE * 2 ] = g->data.counter->colors[ 0 ].r / 255.0f;
        vptr[ 5 + VERTEX_SIZE * 2 ] = g->data.counter->colors[ 0 ].g / 255.0f;
        vptr[ 6 + VERTEX_SIZE * 2 ] = g->data.counter->colors[ 0 ].b / 255.0f;
        vptr[ 7 + VERTEX_SIZE * 2 ] = g->data.counter->colors[ 0 ].a / 255.0f;

        vptr[ 4 + VERTEX_SIZE * 3 ] = g->data.counter->colors[ 2 ].r / 255.0f;
        vptr[ 5 + VERTEX_SIZE * 3 ] = g->data.counter->colors[ 2 ].g / 255.0f;
        vptr[ 6 + VERTEX_SIZE * 3 ] = g->data.counter->colors[ 2 ].b / 255.0f;
        vptr[ 7 + VERTEX_SIZE * 3 ] = g->data.counter->colors[ 2 ].a / 255.0f;

        BufferVertices( vptr );
       
        #undef CHARACTER
    }
    ClearBufferBindings(); 
};

void NasrGraphicsCounterSetOpacity( unsigned int id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsCounterSetOpacity Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.counter->opacity = v;
};

void NasrGraphicsCounterSetXOffset( unsigned id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsCounterSetXOffset Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.counter->xoffset = v;
};

void NasrGraphicsCounterSetYOffset( unsigned id, float v )
{
    #ifdef NASR_SAFE
        if ( id >= max_graphics )
        {
            NasrLog( "NasrGraphicsCounterSetYOffset Error: invalid id %u", id );
            return;
        }
    #endif
    GetGraphic( id )->data.counter->yoffset = v;
};



// Texture
int NasrLoadFileAsTexture( const char * filename )
{
    return NasrLoadFileAsTextureEx( filename, NASR_SAMPLING_DEFAULT, NASR_INDEXED_DEFAULT );
};

int NasrLoadFileAsTextureEx( const char * filename, int sampling, int indexed )
{
    const hash_t needle_hash = TextureMapHashString( filename );
    TextureMapEntry * entry = &texture_map[ needle_hash ];

    // If root entry itself is empty, just fill it in.
    if ( !entry->key.string )
    {
        entry->key.string = ( char * )( malloc( strlen( filename ) + 1 ) );
        strcpy( entry->key.string, filename );
        entry->key.hash = needle_hash;
        entry->value = texture_count;
        entry->next = 0;
    }
    else
    {
        const int cmp = strcmp( entry->key.string, filename );
        // If root entry has the same string, it's what we're looking for, so just return its value.
        if ( cmp == 0 )
        {
            return entry->value;
        }
        // If root entry is alphabetically after what we're looking for, we already know it's not here.
        // Add new entry & set as new root & move ol’ root to new root's next node.
        else if ( cmp > 0 )
        {
            TextureMapEntry * new_entry = calloc( 1, sizeof( TextureMapEntry ) );
            *new_entry = *entry;
            entry->key.string = ( char * )( malloc( strlen( filename ) + 1 ) );
            strcpy( entry->key.string, filename );
            entry->key.hash = needle_hash;
            entry->value = texture_count;
            entry->next = new_entry;
        }
        else
        {
            int cmp = -1;
            TextureMapEntry * preventry;

            // Loop thru linked list till we reach entry whose string is alphabetically after what we’re looking for.
            do
            {
                preventry = entry;
                entry = entry->next;
            }
            while ( entry && ( ( cmp = strcmp( entry->key.string, filename ) ) > 0 ) );

            // If the entry we stop on has the same string, we already have what we’re looking for, so just return its value.
            if ( cmp == 0 )
            {
                return entry->value;
            }

            // Otherwise, this means we don’t have what we’re looking for, so add new entry & put it ’tween preventry & entry
            // too keep linked list alphabetically ordered.
            TextureMapEntry * new_entry = calloc( 1, sizeof( TextureMapEntry ) );
            new_entry->key.string = ( char * )( malloc( strlen( filename ) + 1 ) );
            strcpy( new_entry->key.string, filename );
            new_entry->key.hash = needle_hash;
            new_entry->value = texture_count;
            new_entry->next = preventry->next;
            preventry->next = new_entry;
        }
    }

    unsigned int width;
    unsigned int height;
    unsigned char * data = LoadTextureFileData( filename, &width, &height, sampling, indexed );
    if ( !data )
    {
        NasrLog( "NasrLoadFileAsTextureEx Error: could not load data from “%s”.", filename );
        return -1;
    }
    const int id = NasrAddTextureEx( data, width, height, sampling, indexed );
    free( data );
    return id;
};

int NasrAddTexture( unsigned char * data, unsigned int width, unsigned int height )
{
    return NasrAddTextureEx( data, width, height, NASR_SAMPLING_DEFAULT, NASR_INDEXED_DEFAULT );
};

int NasrAddTextureEx( unsigned char * data, unsigned int width, unsigned int height, int sampling, int indexed )
{
    if ( texture_count >= max_textures )
    {
        const unsigned int prev_max_textures = max_textures;
        Texture * new_textures = calloc( max_textures * 2, sizeof( Texture ) );
        unsigned int * new_texture_ids = calloc( max_textures * 2, sizeof( unsigned int ) );
        if ( !new_textures || !new_texture_ids )
        {
            NasrLog( "NasrAddTextureEx Error: ¡Not enough memory for textures!" );
            return -1;
        }
        max_textures *= 2;
        memcpy( new_textures, textures, sizeof( Texture ) * prev_max_textures );
        memcpy( new_texture_ids, texture_ids, sizeof( unsigned int ) * prev_max_textures );
        free( textures );
        free( texture_ids );
        textures = new_textures;
        texture_ids = new_texture_ids;
        glGenTextures( max_textures - prev_max_textures, &new_texture_ids[ prev_max_textures ] );
    }

    AddTexture( &textures[ texture_count ], texture_ids[ texture_count ], data, width, height, sampling, indexed );
    return texture_count++;
};

int NasrAddTextureBlank( unsigned int width, unsigned int height )
{
    return NasrAddTexture( 0, width, height );
};

int NasrAddTextureBlankEx( unsigned int width, unsigned int height, int sampling, int indexed )
{
    return NasrAddTextureEx( 0, width, height, sampling, indexed );
};

void NasrSetTextureAsTarget( unsigned int texture )
{
    if ( texture >= texture_count )
    {
        NasrLog( "NasrSetTextureAsTarget Error: texture #%d is beyond texture limit.", texture );
        return;
    }
    glBindFramebuffer( GL_FRAMEBUFFER, framebuffer );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_ids[ texture ], 0 );
    glViewport( 0, 0, textures[ texture ].width, textures[ texture ].height );
    selected_texture = texture;
    BindBuffers( max_graphics );
    UpdateShaderOrtho( 0.0f, 0.0f, textures[ selected_texture ].width, textures[ selected_texture ].height );
};

void NasrReleaseTextureTarget()
{
    ResetVertices( GetVertices( max_graphics ) );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( magnified_canvas_x, magnified_canvas_y, magnified_canvas_width, magnified_canvas_height );
    selected_texture = -1;
    ClearBufferBindings();
    UpdateShaderOrthoToCamera();
};

void NasrGetTexturePixels( unsigned int texture, void * pixels )
{
    #ifdef NASR_SAFE
        if ( texture >= texture_count )
        {
            NasrLog( "NasrGetTexturePixels Error: texture #%u is beyond texture limit.", texture );
            return;
        }
    #endif
    glGetTextureImage
    (
        texture_ids[ texture ],
  	    0,
  	    GL_RGBA,
  	    GL_UNSIGNED_BYTE,
        textures[ texture ].width * textures[ texture ].height * 4,
        pixels
  	);
};

void NasrCopyTextureToTexture( unsigned int src, unsigned int dest, NasrRectInt srccoords, NasrRectInt destcoords )
{
    #ifdef NASR_SAFE
        if ( dest >= texture_count )
        {
            NasrLog( "NasrCopyTextureToTexture Error: texture #%u is beyond texture limit.", dest );
            return;
        }
    #endif
    unsigned char pixels[ textures[ dest ].width * textures[ dest ].height * 4 ];
    NasrGetTexturePixels( dest, pixels );
    NasrApplyTextureToPixelData( src, pixels, srccoords, destcoords );
    glBindTexture( GL_TEXTURE_2D, texture_ids[ dest ] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, textures[ dest ].width, textures[ dest ].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
};

void NasrApplyTextureToPixelData( unsigned int texture, unsigned char * dest, NasrRectInt srccoords, NasrRectInt destcoords )
{
    #ifdef NASR_SAFE
        if ( texture >= texture_count )
        {
            NasrLog( "NasrApplyTextureToPixelData Error: texture #%u is beyond texture limit.", texture );
            return;
        }
    #endif
    unsigned char src[ textures[ texture ].width * textures[ texture ].height * 4 ];
    NasrGetTexturePixels( texture, src );
    int maxx = srccoords.w;
    if ( maxx + srccoords.x > textures[ texture ].width )
    {
        maxx = textures[ texture ].width - srccoords.x;
    }
    if ( maxx + destcoords.x > destcoords.w )
    {
        maxx = destcoords.w - destcoords.x;
    }
    maxx *= 4;
    int maxh = srccoords.h;
    if ( maxh + srccoords.y > textures[ texture ].height )
    {
        maxh = textures[ texture ].height - srccoords.y;
    }
    if ( maxh + destcoords.y > destcoords.h )
    {
        maxh = destcoords.h - destcoords.y;
    }
    for ( int y = 0; y < maxh; ++y )
    {
        const int srci = ( ( ( srccoords.y + y ) * textures[ texture ].width ) + srccoords.x ) * 4;
        const int desti = ( ( ( destcoords.y + y ) * destcoords.w ) + destcoords.x ) * 4;
        memcpy( &dest[ desti ], &src[ srci ], maxx );
    }
};

void NasrCopyPixelData( unsigned char * src, unsigned char * dest, NasrRectInt srccoords, NasrRectInt destcoords, int maxsrcw, int maxsrch )
{
    int maxx = srccoords.w;
    if ( maxx + srccoords.x > maxsrcw )
    {
        maxx = maxsrcw - srccoords.x;
    }
    if ( maxx + destcoords.x > destcoords.w )
    {
        maxx = destcoords.w - destcoords.x;
    }
    maxx *= 4;
    int maxh = srccoords.h;
    if ( maxh + srccoords.y > maxsrch )
    {
        maxh = maxsrch - srccoords.y;
    }
    if ( maxh + destcoords.y > destcoords.h )
    {
        maxh = destcoords.h - destcoords.y;
    }
    for ( int y = 0; y < maxh; ++y )
    {
        const int srci = ( ( ( srccoords.y + y ) * maxsrcw ) + srccoords.x ) * 4;
        const int desti = ( ( ( destcoords.y + y ) * destcoords.w ) + destcoords.x ) * 4;
        memcpy( &dest[ desti ], &src[ srci ], maxx );
    }
};

void NasrTileTexture( unsigned int texture, unsigned char * pixels, NasrRectInt srccoords, NasrRectInt destcoords )
{
    #ifdef NASR_SAFE
        if ( texture >= texture_count )
        {
            NasrLog( "NasrTileTexture Error: texture #%u is beyond texture limit.", texture );
            return;
        }
    #endif
    NasrApplyTextureToPixelData( texture, pixels, srccoords, destcoords );
    int w = srccoords.w;
    int h = srccoords.h;
    while ( w < destcoords.w && h < destcoords.h )
    {
        const NasrRectInt src4 = { 0, 0, w, h };
        const NasrRectInt dest7 = { w, 0, destcoords.w, destcoords.h };
        const NasrRectInt dest8 = { 0, h, destcoords.w, destcoords.h };
        const NasrRectInt dest9 = { w, h, destcoords.w, destcoords.h };
        NasrCopyPixelData( pixels, pixels, src4, dest7, destcoords.w, destcoords.h );
        NasrCopyPixelData( pixels, pixels, src4, dest8, destcoords.w, destcoords.h );
        NasrCopyPixelData( pixels, pixels, src4, dest9, destcoords.w, destcoords.h );
        w *= 2;
        h *= 2;
    }
};

void NasrClearTextures( void )
{
    for ( int i = 0; i < texture_map_size; ++i )
    {
        if
        (
            texture_map[ i ].key.string != NULL
        )
        {
            free( texture_map[ i ].key.string );
            texture_map[ i ].key.string = 0;
            texture_map[ i ].key.hash = 0;
            while ( texture_map[ i ].next )
            {
                TextureMapEntry * t = texture_map[ i ].next->next;
                free( texture_map[ i ].next->key.string );
                free( texture_map[ i ].next );
                texture_map[ i ].next = t;
            }
        }
    }
    texture_count = 0;
};

unsigned int NasrTextureGetWidth( unsigned int texture )
{
    #if NASR_SAFE
        if ( texture >= max_textures )
        {
            NasrLog( "NasrTextureGetWidth Error: texture #%u is beyond texture limit.", texture );
            return 0;
        }
    #endif
    return textures[ texture ].width;
};

unsigned int NasrTextureGetHeight( unsigned int texture )
{
    #if NASR_SAFE
        if ( texture >= max_textures )
        {
            NasrLog( "NasrTextureGetHeight Error: texture #%u is beyond texture limit.", texture );
            return 0;
        }
    #endif
    return textures[ texture ].height;
};



// Draw to Texture
void NasrDrawRectToTexture( NasrRect rect, NasrColor color )
{
    ResetVertices( GetVertices( max_graphics ) );
    SetVerticesColorValues( GetVertices( max_graphics ), &color, &color, &color, &color );
    rect.x *= canvas.w / textures[ selected_texture ].width;
    rect.y = ( textures[ selected_texture ].height - ( rect.y + rect.h ) ) * ( canvas.h / textures[ selected_texture ].height );
    rect.w *= canvas.w / textures[ selected_texture ].width;
    rect.h *= canvas.h / textures[ selected_texture ].height;
    DrawBox
    (
        vaos[ max_graphics ],
        &rect,
        0.0f,
        0.0f
    );
    SetupVertices( vaos[ max_graphics ] );
};

void NasrDrawGradientRectToTexture( NasrRect rect, int dir, NasrColor color1, NasrColor color2 )
{
    NasrColor cul = color2;
    NasrColor cur = color2;
    NasrColor cbr = color1;
    NasrColor cbl = color1;
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            cul = color2;
            cur = color1;
            cbr = color1;
            cbl = color1;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            cul = color2;
            cur = color1;
            cbr = color2;
            cbl = color1;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            cul = color1;
            cur = color1;
            cbr = color2;
            cbl = color1;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            cul = color1;
            cur = color1;
            cbr = color2;
            cbl = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            cul = color1;
            cur = color1;
            cbr = color1;
            cbl = color2;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            cul = color1;
            cur = color2;
            cbr = color1;
            cbl = color2;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            cul = color1;
            cur = color2;
            cbr = color1;
            cbl = color1;
        }
        break;
        default:
        {
            NasrLog( "¡Invalid gradient direction for NasrDrawGradientRectToTexture! %d\n", dir );
        }
        break;
    }
    ResetVertices( GetVertices( max_graphics ) );
    SetVerticesColorValues( GetVertices( max_graphics ), &cbl, &cbr, &cur, &cul );
    rect.y = ( textures[ selected_texture ].height - ( rect.y + rect.h ) );
    DrawBox
    (
        vaos[ max_graphics ],
        &rect,
        0.0f,
        0.0f
    );
    SetupVertices( vaos[ max_graphics ] );
};

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
)
{
    NasrGraphicSprite sprite;
    sprite.texture = texture;
    sprite.src = src;
    sprite.dest = dest;
    sprite.flip_x = flip_x;
    sprite.flip_y = !flip_y;
    sprite.rotation_x = rotation_x;
    sprite.rotation_y = rotation_y;
    sprite.rotation_z = rotation_z;
    sprite.opacity = opacity;
    sprite.palette = palette;
    sprite.useglobalpal = useglobalpal;
    sprite.tilingx = tilingx;
    sprite.tilingy = tilingy;

    ResetVertices( GetVertices( max_graphics ) );
    sprite.dest.y = ( textures[ selected_texture ].height - ( sprite.dest.y + sprite.dest.h ) );

    SetShader( sprite_shader );

    UpdateSpriteVerticesValues( GetVertices( max_graphics ), &sprite );

    SetVerticesView( sprite.dest.x + ( sprite.dest.w / 2.0f ), sprite.dest.y + ( sprite.dest.h / 2.0f ), 0.0f, 0.0f );

    mat4 model = BASE_MATRIX;
    vec3 scale = { sprite.dest.w, sprite.dest.h, 0.0 };
    glm_scale( model, scale );
    vec3 xrot = { 0.0, 1.0, 0.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( sprite.rotation_x ), xrot );
    vec3 yrot = { 0.0, 0.0, 1.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( sprite.rotation_y ), yrot );
    vec3 zrot = { 1.0, 0.0, 0.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( sprite.rotation_z ), zrot );
    glUniformMatrix4fv( sprite_uniforms.model, 1, GL_FALSE, ( float * )( model ) );

    glUniform1f( sprite_uniforms.opacity, ( float )( sprite.opacity ) );

    glUniform2f( sprite_uniforms.tiling, sprite.tilingx, sprite.tilingy );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture_ids[ sprite.texture ] );
    glUniform1i( sprite_uniforms.texture_data, 0 );
    SetupVertices( vaos[ max_graphics ] );
};



// Debug Functions
void NasrColorPrint( const NasrColor * c )
{
    printf( "NasrColor: %f, %f, %f, %f\n", c->r, c->g, c->b, c->a );
};

void NasrDebugGraphics( void )
{
    printf( "=================\n" );
    for ( int i = 0; i < num_o_graphics; ++i )
    {
        const int id = gfx_ptrs_pos_to_id[ i ];
        const int pos = gfx_ptrs_id_to_pos[ id ];
        const int state = state_for_gfx[ id ];
        const int layer = layer_for_gfx[ id ];
        printf( "%d : %d : %d : %d : %d\n", i, pos, id, state, layer );
    }
    printf( "=================\n" );
    for ( int state = 0; state < max_states; ++state )
    {
        printf( "%d: ", state );
        for ( int layer = 0; layer < max_gfx_layers; ++layer )
        {
            printf( "%d,", layer_pos[ GetStateLayerIndex( state, layer ) ] );
        }
        printf( "\n" );
    }
    printf( "=================\n" );
};

void NasrRectPrint( const NasrRect * r )
{
    printf( "NasrRect: %f, %f, %f, %f\n", r->x, r->y, r->w, r->h );
};



// Static functions
static int AddGraphic
(
    unsigned int state,
    unsigned int layer,
    struct NasrGraphic graphic
)
{
    if ( num_o_graphics >= max_graphics )
    {
        if ( !GrowGraphics() )
        {
            return -1;
        }
    }

    // Find last graphic of current layer.
    unsigned int pp = state * max_gfx_layers + layer;
    unsigned int p = layer_pos[ pp ];

    // Push forward this & following positions.
    for ( unsigned int i = pp; i < max_gfx_layers * max_states; ++i )
    {
        ++layer_pos[ i ];
    }

    // Push forward all following graphics.
    for ( unsigned int i = num_o_graphics++; i > p; --i )
    {
        graphics[ i ] = graphics[ i - 1 ];

        // Update pointers so they still point to correct graphics.
        const unsigned int t = gfx_ptrs_pos_to_id[ i - 1 ];
        ++gfx_ptrs_id_to_pos[ t ];
        gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    }

    // Find 1st free graphic ID.
    int current_graphic_id = -1;
    for ( int i = 0; i < max_graphics; ++i )
    {
        if ( gfx_ptrs_id_to_pos[ i ] == -1 )
        {
            current_graphic_id = i;
            break;
        }
    }

    if ( current_graphic_id == -1 )
    {
        NasrLog( "¡Strange error! ¡All graphics IDs filled e’en tho we’re apparently below max graphics!" );
    }

    // Add current graphic & pointer.
    graphics[ p ] = graphic;
    gfx_ptrs_id_to_pos[ current_graphic_id ] = p;
    gfx_ptrs_pos_to_id[ p ] = current_graphic_id;
    state_for_gfx[ current_graphic_id ] = state;
    layer_for_gfx[ current_graphic_id ] = layer;

    return current_graphic_id;
};

static void AddTexture( Texture * texture, unsigned int texture_id, const unsigned char * data, unsigned int width, unsigned int height, int sampling, int indexed )
{
    const GLint sample_type = GetGLSamplingType( sampling );
    const GLint index_type = GetGLRGBA( indexed );

    texture->width = width;
    texture->height = height;
    texture->indexed = index_type == GL_R8;
    glBindTexture( GL_TEXTURE_2D, texture_id );
    glTexImage2D( GL_TEXTURE_2D, 0, index_type, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sample_type );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sample_type );
};

static void BindBuffers( unsigned int id )
{
    glBindVertexArray( vaos[ id ] );
    glBindBuffer( GL_ARRAY_BUFFER, vbos[ id ] );
};

static void BufferDefault( float * vptr )
{
    // EBO
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );

    // VBO
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices_base ), vptr, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof( float ), 0 );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof( float ), ( void * )( 2 * sizeof( float ) ) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof( float ), ( void * )( 4 * sizeof( float ) ) );
    glEnableVertexAttribArray( 2 );
    BufferVertices( vptr );
};

static void BufferVertices( float * vptr )
{
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * VERTEX_RECT_SIZE, vptr, GL_STATIC_DRAW );
};

static CharMapEntry * CharMapGenEntry( unsigned int id, const char * key )
{
    hash_t needle_hash = CharMapHashString( id, key );
    CharMapEntry * entry = CharMapHashFindEntry( id, key, needle_hash );
    if ( !entry->key.string )
    {
        entry->key.string = ( char * )( malloc( strlen( key ) + 1 ) );
        strcpy( entry->key.string, key );
        entry->key.hash = needle_hash;
    }
    return entry;
};

static CharMapEntry * CharMapHashFindEntry( unsigned int id, const char * needle_string, hash_t needle_hash )
{
    while ( 1 )
    {
        CharMapEntry * entry = &charmaps.list[ id ].list[ needle_hash ];
        if ( entry->key.string == NULL || strcmp( entry->key.string, needle_string ) == 0 )
        {
            return entry;
        }
        needle_hash = ( needle_hash + 1 ) % charmaps.list[ id ].hashmax;
    }
};

static uint32_t CharMapHashString( unsigned int id, const char * key )
{
    return NasrHashString( key, charmaps.list[ id ].hashmax );
};

static void CharsetMalformedError( const char * msg, const char * file )
{
    NasrLog( "NasrAddCharset Error: Charset file “%s” malformed: “%s”.", file, msg );
};

static void ClearBufferBindings( void )
{
    glBindVertexArray( 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
};

static void DestroyGraphic( NasrGraphic * graphic )
{
    switch ( graphic->type )
    {
        case ( NASR_GRAPHIC_TEXT ):
        {
            if ( graphic->data.text.vaos )
            {
                glDeleteVertexArrays( graphic->data.text.capacity, graphic->data.text.vaos );
                free( graphic->data.text.vaos );
            }
            if ( graphic->data.text.vbos )
            {
                glDeleteRenderbuffers( graphic->data.text.capacity, graphic->data.text.vbos );
                free( graphic->data.text.vbos );
            }
            free( graphic->data.text.vertices );
            free( graphic->data.text.chars );
            graphic->type = NASR_GRAPHIC_NONE;
        }
        break;
        case ( NASR_GRAPHIC_COUNTER ):
        {
            if ( graphic->data.counter )
            {
                if ( graphic->data.counter->vaos )
                {
                    glDeleteVertexArrays( graphic->data.counter->count, graphic->data.counter->vaos );
                    free( graphic->data.counter->vaos );
                }
                if ( graphic->data.counter->vbos )
                {
                    glDeleteRenderbuffers( graphic->data.counter->count, graphic->data.counter->vbos );
                    free( graphic->data.counter->vbos );
                }
                free( graphic->data.counter->vertices );
                free( graphic->data.counter->chars );
                free( graphic->data.counter );
            }
            graphic->type = NASR_GRAPHIC_NONE;
        }
        break;
        case ( NASR_GRAPHIC_TILEMAP ):
        {
            if ( graphic->data.tilemap.data )
            {
                free( graphic->data.tilemap.data );
            }
            graphic->type = NASR_GRAPHIC_NONE;
        }
        break;
    }
};

static void DrawBox( unsigned int vao, const NasrRect * rect, float scrollx, float scrolly )
{
    // Set shader.
    SetShader( rect_shader );

    // Set view.
    SetVerticesView( rect->x + ( rect->w / 2.0f ), rect->y + ( rect->h / 2.0f ), scrollx, scrolly );

    // Set scale.
    mat4 model = BASE_MATRIX;
    vec3 scale = { rect->w, rect->h, 0.0 };
    glm_scale( model, scale );
    glUniformMatrix4fv( rect_uniforms.model, 1, GL_FALSE, ( float * )( model ) );
};

static void FramebufferSizeCallback( GLFWwindow * window, int screen_width, int screen_height )
{
    double screen_aspect_ratio = ( double )( canvas.w / canvas.h );
    double monitor_aspect_ratio = ( double )( screen_width ) / ( double )( screen_height );

    // Base magnification on max that fits in window.
    magnification = 
        ( int )( floor(
            ( monitor_aspect_ratio > screen_aspect_ratio )
                ? ( double )( screen_height ) / ( double )( canvas.h )
                : ( double )( screen_width ) / ( double )( canvas.w )
        ));
    if ( magnification < 1 )
    {
        magnification = 1;
    }

    magnified_canvas_width = canvas.w * magnification;
    magnified_canvas_height = canvas.h * magnification;
    magnified_canvas_x = ( int )( floor( ( double )( screen_width - magnified_canvas_width ) / 2.0 ) );
    magnified_canvas_y = ( int )( floor( ( double )( screen_height - magnified_canvas_height ) / 2.0 ) );
    glViewport( magnified_canvas_x, magnified_canvas_y, magnified_canvas_width, magnified_canvas_height );
};

static unsigned int GenerateShaderProgram( const NasrShader * shaders, int shadersnum )
{
    unsigned int program = glCreateProgram();
    unsigned int shader_ids[ shadersnum ];

    // Compile each shader and attach to program
    for ( int i = 0; i < shadersnum; ++i )
    {
        int success;
        char infoLog[ 512 ];

        GLenum type;
        switch ( shaders[ i ].type )
        {
            case ( NASR_SHADER_VERTEX ):
            {
                type = GL_VERTEX_SHADER;
            }
            break;
            case ( NASR_SHADER_FRAGMENT ):
            {
                type = GL_FRAGMENT_SHADER;
            }
            break;
            default:
            {
                NasrLog( "Invalid shader type given to shader!" );
            }
            break;
        }

        shader_ids[ i ] = glCreateShader( type );
        glShaderSource( shader_ids[ i ], 1, ( const GLchar * const * )( &shaders[ i ].code ), NULL );
        glCompileShader( shader_ids[ i ] );
        glGetShaderiv( shader_ids[ i ], GL_COMPILE_STATUS, &success );
        if ( !success )
        {
            glGetShaderInfoLog( shader_ids[ i ], 512, NULL, infoLog );
            NasrLog( "ERROR::SHADER::VERTEX::COMPILATION_FAILED." );
            NasrLog( infoLog );
        }
        glAttachShader( program, shader_ids[ i ] );
    }

    glLinkProgram( program );

    // If there are any errors, log them
    int  success;
    char infoLog[ 512 ];
    glGetProgramiv( program, GL_LINK_STATUS, &success );
    if( !success )
    {
        glGetProgramInfoLog( program, 512, NULL, infoLog );
        NasrLog( "ERROR: Failed to create shader program." );
        NasrLog( infoLog );
    }

    // Delete shaders
    for ( int i = 0; i < shadersnum; ++i )
    {
        glDeleteShader( shader_ids[ i ] );
    }

    return program;
};

static int GetCharacterSize( const char * s )
{
    const int code = ( int )( *s );
    return ( code & ( 1 << 7 ) )
        ? ( code & ( 1 << 5 ) )
            ? ( code & ( 1 << 4 ) )
                ? 4
                : 3
            : 2
        : 1;
};

static GLint GetGLRGBA( int indexed )
{
    switch ( indexed )
    {
        case ( NASR_INDEXED_YES ): return GL_R8;
        case ( NASR_INDEXED_NO ): return GL_RGBA;
        default: return default_indexed_mode;
    }
};

static GLint GetGLSamplingType( int sampling )
{
    switch ( sampling )
    {
        case ( NASR_SAMPLING_NEAREST ): return GL_NEAREST;
        case ( NASR_SAMPLING_LINEAR ): return GL_LINEAR;
        default: return default_sample_type;
    }
};

static NasrGraphic * GetGraphic( unsigned int id )
{
    return &graphics[ gfx_ptrs_id_to_pos[ id ] ];
};

static unsigned int GetStateLayerIndex( unsigned int state, unsigned int layer )
{
    return state * max_gfx_layers + layer;
};

static float * GetVertices( unsigned int id )
{
    return &vertices[ id * VERTEX_RECT_SIZE ];
};

static int GraphicsAddCounter
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    unsigned int charset,
    float num,
    unsigned int maxdigits,
    unsigned int maxdecimals,
    float x,
    float y,
    float shadow,
    float opacity,
    uint_fast8_t numpadding,
    uint_fast8_t decimalpadding,
    NasrColor ** colors,
    uint_fast8_t palette,
    uint_fast8_t palette_type
)
{
    if ( charset >= charmaps.capacity || !charmaps.list[ charset ].list )
    {
        return -1;
    }

    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_COUNTER;
    graphic.data.counter = calloc( 1, sizeof( NasrGraphicCounter ) );
    graphic.data.counter->palette = palette;
    graphic.data.counter->palette_type = palette_type;
    graphic.data.counter->numpadding = numpadding;
    graphic.data.counter->decimalpadding = decimalpadding;
    graphic.data.counter->maxdigits = maxdigits;
    graphic.data.counter->maxdecimals = maxdecimals;
    // If no decimals, just have max chars maxdecimals, else have 1 extra for floating point char.
    int count = maxdigits + ( maxdecimals > 0 ? maxdecimals + 1 : 0 );
    graphic.data.counter->count = count;
    graphic.data.counter->maxnum = pow( 10, maxdigits ) - pow( 10, ( int )( -maxdecimals - 1 ) );
    graphic.data.counter->charset = charset;
    graphic.data.counter->xoffset = x;
    graphic.data.counter->yoffset = y;
    graphic.data.counter->shadow = shadow;
    graphic.data.counter->opacity = opacity;
    graphic.data.counter->vaos = calloc( count, sizeof( unsigned int ) );
    graphic.data.counter->vbos = calloc( count, sizeof( unsigned int ) );
    graphic.data.counter->vertices = calloc( count * VERTEX_RECT_SIZE, sizeof( float ) );
    graphic.data.counter->chars = calloc( count, sizeof( NasrChar ) );
    if ( colors )
    {
        for ( int j = 0; j < 4; ++j )
        {
            memcpy( &graphic.data.counter->colors[ j ], colors[ j ], sizeof( NasrColor ) );
        }
    }
    const int id = AddGraphic( state, layer, graphic );

    NasrGraphic * g = GetGraphic( id );
    glGenVertexArrays( count, g->data.counter->vaos );
    glGenBuffers( count, g->data.counter->vbos );

    // If # goes beyond maxdecimals, make it show all 9s ’stead o’ seeming to loop back round.
    num = NASR_MATH_MIN( num, graphic.data.counter->maxnum );
    int intnum = ( int )( floor( num ) );

    for ( int i = 0; i < count; ++i )
    {
        float * vptr = &g->data.counter->vertices[ i * VERTEX_RECT_SIZE ];

        // Get main digits, then floating point, then decimals.
        const int c = i == maxdigits ? 10 : ( i > maxdigits ) ? NasrGetDigit( ( int )( floor( num * pow( 10, i - maxdigits ) ) ), 1 ) : NasrGetDigit( intnum, maxdigits - i );
        CharNum * character = &charmaps.list[ charset ].nums[ c ];

        graphic.data.counter->chars[ i ].src = character->src;
        graphic.data.counter->chars[ i ].dest.x = ( charmaps.list[ charset ].numwidth * i ) + character->xoffset;
        graphic.data.counter->chars[ i ].dest.y = character->yoffset;
        graphic.data.counter->chars[ i ].dest.w = character->src.w;
        graphic.data.counter->chars[ i ].dest.h = character->src.h;

        glBindVertexArray( g->data.counter->vaos[ i ] );
        glBindBuffer( GL_ARRAY_BUFFER, g->data.counter->vbos[ i ] );

        ResetVertices( vptr );
        const float texturew = ( float )( charmaps.list[ charset ].texture.width );
        const float textureh = ( float )( charmaps.list[ charset ].texture.height );
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / texturew * character->src.x; // Left X
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / texturew * ( character->src.x + character->src.w );  // Right X
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / textureh * ( character->src.y + character->src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / textureh * character->src.y;  // Bottom Y

        vptr[ 4 ] = graphic.data.counter->colors[ 3 ].r / 255.0f;
        vptr[ 5 ] = graphic.data.counter->colors[ 3 ].g / 255.0f;
        vptr[ 6 ] = graphic.data.counter->colors[ 3 ].b / 255.0f;
        vptr[ 7 ] = graphic.data.counter->colors[ 3 ].a / 255.0f;

        vptr[ 4 + VERTEX_SIZE ] = graphic.data.counter->colors[ 1 ].r / 255.0f;
        vptr[ 5 + VERTEX_SIZE ] = graphic.data.counter->colors[ 1 ].g / 255.0f;
        vptr[ 6 + VERTEX_SIZE ] = graphic.data.counter->colors[ 1 ].b / 255.0f;
        vptr[ 7 + VERTEX_SIZE ] = graphic.data.counter->colors[ 1 ].a / 255.0f;

        vptr[ 4 + VERTEX_SIZE * 2 ] = graphic.data.counter->colors[ 0 ].r / 255.0f;
        vptr[ 5 + VERTEX_SIZE * 2 ] = graphic.data.counter->colors[ 0 ].g / 255.0f;
        vptr[ 6 + VERTEX_SIZE * 2 ] = graphic.data.counter->colors[ 0 ].b / 255.0f;
        vptr[ 7 + VERTEX_SIZE * 2 ] = graphic.data.counter->colors[ 0 ].a / 255.0f;

        vptr[ 4 + VERTEX_SIZE * 3 ] = graphic.data.counter->colors[ 2 ].r / 255.0f;
        vptr[ 5 + VERTEX_SIZE * 3 ] = graphic.data.counter->colors[ 2 ].g / 255.0f;
        vptr[ 6 + VERTEX_SIZE * 3 ] = graphic.data.counter->colors[ 2 ].b / 255.0f;
        vptr[ 7 + VERTEX_SIZE * 3 ] = graphic.data.counter->colors[ 2 ].a / 255.0f;
       
        #undef CHARACTER

        BufferDefault( vptr );
    }
    ClearBufferBindings();

    return id;
};

static int GraphicAddText
(
    float scrollx,
	float scrolly,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    NasrColor * top_left_color,
    NasrColor * top_right_color,
    NasrColor * bottom_left_color,
    NasrColor * bottom_right_color,
    uint_fast8_t palette,
    uint_fast8_t palette_type
)
{
    if ( text.charset >= charmaps.capacity || !charmaps.list[ text.charset ].list )
    {
        return -1;
    }

    // Generate char list from string.
    float charw = text.coords.w - text.padding_left - text.padding_right;
    float charh = text.coords.h - text.padding_top - text.padding_bottom;
    float charx = text.coords.x + text.padding_left;
    float chary = text.coords.y + text.padding_top;
    const float lnend = charx + charw;

    char * string = text.string;
    CharTemplate letters[ strlen( string ) ];
    int lettercount = 0;
    while ( *string )
    {
        const int charlen = GetCharacterSize( string );

        // Generate letter string
        char letter[ charlen + 1 ];
        strncpy( letter, string, charlen );
        letter[ charlen ] = 0;

        // Find character
        hash_t needle_hash = CharMapHashString( text.charset, letter );
        CharMapEntry * entry = CharMapHashFindEntry( text.charset, letter, needle_hash );
        if ( entry->key.string == NULL )
        {
            needle_hash = CharMapHashString( text.charset, "default" );
            entry = CharMapHashFindEntry( text.charset, "default", needle_hash );
            if ( entry->key.string )
            {
                letters[ lettercount ] = entry->value;
            }
        }
        else
        {
            letters[ lettercount ] = entry->value;
        }

        ++lettercount;
        string += charlen;
    }
    int maxlines = ( int )( charh );
    int maxperline = ( int )( charw );
    CharTemplate lines[ maxlines ][ maxperline ];
    float line_widths[ maxlines ];
    line_widths[ 0 ] = 0;
    int line_character_counts[ maxlines ];
    int line_count = 0;
    int line_character = 0;
    long unsigned int i = 0;
    int lx = ( int )( charx );
    int endswithnewline[ maxlines ];
    while ( i < lettercount )
    {
        long unsigned int ib = i;
        float xb = lx;
        int look_ahead = 1;

        // Look ahead so we can know ahead o’ time whether we need to add a new line.
        // This autobreaks text without cutting midword.
        while ( look_ahead )
        {
            if ( ib >= lettercount )
            {
                look_ahead = 0;
                break;
            }

            if ( letters[ ib ].chartype == NASR_CHAR_NEWLINE )
            {
                look_ahead = 0;
            }
            else if ( letters[ ib ].chartype == NASR_CHAR_WHITESPACE )
            {
                look_ahead = 0;
            }
            else if ( xb >= lnend )
            {
                lx = ( int )( charx );
                line_character_counts[ line_count ] = line_character;
                endswithnewline[ line_count ] = 0;
                ++line_count;
                line_widths[ line_count ] = 0;
                line_character = 0;
                look_ahead = 0;
            }
            else if ( ib >= lettercount )
            {
                look_ahead = 0;
                break;
            }
            xb += letters[ ib ].src.w;
            ++ib;
        }

        while ( i < ib )
        {
            if ( letters[ i ].chartype == NASR_CHAR_NEWLINE || lx >= lnend )
            {
                lx = ( int )( charx );
                line_character_counts[ line_count ] = line_character;
                endswithnewline[ line_count ] = letters[ i ].chartype == NASR_CHAR_NEWLINE;
                ++line_count;
                line_widths[ line_count ] = 0;
                line_character = 0;
            }
            else
            {
                lines[ line_count ][ line_character ] = letters[ i ];
                line_widths[ line_count ] += letters[ i ].src.w;
                ++line_character;
                lx += letters[ i ].src.w;
            }
            ++i;
        }
    }
    line_character_counts[ line_count ] = line_character;
    ++line_count;

    int finalcharcount = 0;
    float maxh[ line_count ];

    for ( int l = 0; l < line_count; ++l )
    {
        finalcharcount += line_character_counts[ l ];

        maxh[ l ] = 8.0f;
        for ( int c = 0; c < line_character_counts[ l ]; ++c )
        {
            if ( lines[ l ][ c ].src.h > maxh[ l ] )
            {
                maxh[ l ] = lines[ l ][ c ].src.h;
            }
        }

        // Sometimes the previous loop keeps whitespace @ the end o’ lines.
        // Since this messes with x alignment, remove these.
        if ( lines[ l ][ line_character_counts[ l ] - 1 ].chartype == NASR_CHAR_WHITESPACE )
        {
            --line_character_counts[ l ];
            line_widths[ l ] -= lines[ l ][ line_character_counts[ l ] - 1 ].src.w;
        }
    }

    NasrChar chars[ finalcharcount ];
    int count = 0;
    // Final loop: we have all the info we need now to set x & y positions.
    float dy = ( text.valign == NASR_VALIGN_MIDDLE )
        ? chary + ( ( charh - ( line_count * 8.0 ) ) / 2.0 )
        : ( text.valign == NASR_VALIGN_BOTTOM )
            ? chary + charh - ( line_count * 8.0 )
            : chary;
    for ( int l = 0; l < line_count; ++l )
    {
        float dx = ( text.align == NASR_ALIGN_CENTER )
            ? charx + ( ( charw - line_widths[ l ] ) / 2.0 )
            : ( text.align == NASR_ALIGN_RIGHT )
                ? lnend - line_widths[ l ]
                : charx;

        // Add justified spacing if set to justified & not an endline.
        float letterspace = text.align == NASR_ALIGN_JUSTIFIED && line_character_counts[ l ] > 0 && l < line_count - 1 && !endswithnewline[ l ]
            ? ( charw - line_widths[ l ] ) / ( float )( line_character_counts[ l ] - 1 )
            : 0.0f;

        for ( int c = 0; c < line_character_counts[ l ]; ++c )
        {
            // Just in case o’ character index misalignment, just copy o’er whole characters.
            if ( lines[ l ][ c ].chartype != NASR_CHAR_WHITESPACE ) {
                chars[ count ].src = lines[ l ][ c ].src;
                chars[ count ].dest = lines[ l ][ c ].src;
                chars[ count ].dest.x = dx;
                chars[ count ].dest.y = dy + ( ( maxh[ l ] - lines[ l ][ c ].src.h ) / 2.0 );
                ++count;
            }
            if ( lines[ l ][ c ].src.w > 0.0f ) {
                dx += ( lines[ l ][ c ].src.w + letterspace );
            }
        }
        dy += maxh[ l ];
    }

    // End charlist

    struct NasrGraphic graphic;
    graphic.scrollx = scrollx;
    graphic.scrolly = scrolly;
    graphic.type = NASR_GRAPHIC_TEXT;
    graphic.data.text.charset = text.charset;
    graphic.data.text.palette = palette;
    graphic.data.text.palette_type = palette_type;
    graphic.data.text.capacity = graphic.data.text.count = count;
    graphic.data.text.xoffset = text.xoffset;
    graphic.data.text.yoffset = text.yoffset;
    graphic.data.text.shadow = text.shadow;
    graphic.data.text.opacity = text.opacity;
    graphic.data.text.vaos = calloc( count, sizeof( unsigned int ) );
    graphic.data.text.vbos = calloc( count, sizeof( unsigned int ) );
    graphic.data.text.vertices = calloc( count * VERTEX_RECT_SIZE, sizeof( float ) );
    graphic.data.text.chars = calloc( count, sizeof( NasrChar ) );
    memcpy( graphic.data.text.chars, chars, count * sizeof( NasrChar ) );
    const int id = AddGraphic( state, layer, graphic );

    NasrGraphic * g = GetGraphic( id );
    glGenVertexArrays( count, g->data.text.vaos );
    glGenBuffers( count, g->data.text.vbos );

    for ( int i = 0; i < count; ++i )
    {
        float * vptr = &g->data.text.vertices[ i * VERTEX_RECT_SIZE ];
        #define CHARACTER g->data.text.chars[ i ]

        glBindVertexArray( g->data.text.vaos[ i ] );
        glBindBuffer( GL_ARRAY_BUFFER, g->data.text.vbos[ i ] );

        ResetVertices( vptr );
        const float texturew = ( float )( charmaps.list[ text.charset ].texture.width );
        const float textureh = ( float )( charmaps.list[ text.charset ].texture.height );
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / texturew * CHARACTER.src.x; // Left X
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / texturew * ( CHARACTER.src.x + CHARACTER.src.w );  // Right X
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / textureh * ( CHARACTER.src.y + CHARACTER.src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / textureh * CHARACTER.src.y;  // Bottom Y

        if ( bottom_right_color )
        {
            vptr[ 4 ] = bottom_right_color->r / 255.0f;
            vptr[ 5 ] = bottom_right_color->g / 255.0f;
            vptr[ 6 ] = bottom_right_color->b / 255.0f;
            vptr[ 7 ] = bottom_right_color->a / 255.0f;

            vptr[ 4 + VERTEX_SIZE ] = top_right_color->r / 255.0f;
            vptr[ 5 + VERTEX_SIZE ] = top_right_color->g / 255.0f;
            vptr[ 6 + VERTEX_SIZE ] = top_right_color->b / 255.0f;
            vptr[ 7 + VERTEX_SIZE ] = top_right_color->a / 255.0f;

            vptr[ 4 + VERTEX_SIZE * 2 ] = top_left_color->r / 255.0f;
            vptr[ 5 + VERTEX_SIZE * 2 ] = top_left_color->g / 255.0f;
            vptr[ 6 + VERTEX_SIZE * 2 ] = top_left_color->b / 255.0f;
            vptr[ 7 + VERTEX_SIZE * 2 ] = top_left_color->a / 255.0f;

            vptr[ 4 + VERTEX_SIZE * 3 ] = bottom_left_color->r / 255.0f;
            vptr[ 5 + VERTEX_SIZE * 3 ] = bottom_left_color->g / 255.0f;
            vptr[ 6 + VERTEX_SIZE * 3 ] = bottom_left_color->b / 255.0f;
            vptr[ 7 + VERTEX_SIZE * 3 ] = bottom_left_color->a / 255.0f;
        }
       
        #undef CHARACTER

        BufferDefault( vptr );
    }
    ClearBufferBindings();

    return id;
};

static void GraphicsRectGradientPaletteUpdateColors( unsigned int id, uint_fast8_t * c )
{
    NasrColor cobj[ 4 ];
    for ( int i = 0; i < 4; ++i )
    {
        cobj[ i ].r = ( float )( c[ i ] );
        cobj[ i ].a = 255.0f;
    }
    ResetVertices( GetVertices( id ) );
    SetVerticesColors( id, &cobj[ 0 ], &cobj[ 1 ], &cobj[ 2 ], &cobj[ 3 ] );
};

static void GraphicsUpdateRectPalette( unsigned int id, uint_fast8_t color )
{
    NasrColor c =
    {
        ( float )( color ),
        0.0f,
        0.0f,
        255.0f
    };
    ResetVertices( GetVertices( id ) );
    SetVerticesColors( id, &c, &c, &c, &c );
};

static int GrowGraphics( void )
{
    const unsigned int new_max_graphics = max_graphics * 2;
    const unsigned int size_diff = new_max_graphics - max_graphics;

    unsigned int * new_vaos = calloc( ( new_max_graphics + 1 ), sizeof( unsigned int ) );
    unsigned int * new_vbos = calloc( ( new_max_graphics + 1 ), sizeof( unsigned int ) );
    float * new_vertices = calloc( ( new_max_graphics + 1 ) * VERTEX_RECT_SIZE, sizeof( float ) );
    NasrGraphic * new_graphics = calloc( new_max_graphics, sizeof( NasrGraphic ) );
    int * new_gfx_ptrs_id_to_pos = calloc( new_max_graphics, sizeof( int ) );
    int * new_gfx_ptrs_pos_to_id = calloc( new_max_graphics, sizeof( int ) );
    int * new_state_for_gfx = calloc( new_max_graphics, sizeof( int ) );
    int * new_layer_for_gfx = calloc( new_max_graphics, sizeof( int ) );
    if
    (
        !new_vaos ||
        !new_vbos ||
        !new_vertices ||
        !new_graphics ||
        !new_gfx_ptrs_id_to_pos ||
        !new_gfx_ptrs_pos_to_id ||
        !new_state_for_gfx ||
        !new_layer_for_gfx
    )
    {
        NasrLog( "AddGraphic Error: ¡Not ’nough memory for graphics!" );
        return 0;
    }

    memcpy( new_vaos, vaos, ( max_graphics + 1 ) * sizeof( unsigned int ) );
    memcpy( new_vbos, vbos, ( max_graphics + 1 ) * sizeof( unsigned int ) );
    memcpy( new_vertices, vertices, ( max_graphics + 1 ) * VERTEX_RECT_SIZE * sizeof( float ) );
    memcpy( new_graphics, graphics, max_graphics * sizeof( NasrGraphic ) );
    memcpy( new_gfx_ptrs_id_to_pos, gfx_ptrs_id_to_pos, max_graphics * sizeof( int ) );
    memcpy( new_gfx_ptrs_pos_to_id, gfx_ptrs_pos_to_id, max_graphics * sizeof( int ) );
    memcpy( new_state_for_gfx, state_for_gfx, max_graphics * sizeof( int ) );
    memcpy( new_layer_for_gfx, layer_for_gfx, max_graphics * sizeof( int ) );

    free( vaos );
    free( vbos );
    free( vertices );
    free( graphics );
    free( gfx_ptrs_id_to_pos );
    free( gfx_ptrs_pos_to_id );
    free( state_for_gfx );
    free( layer_for_gfx );

    vaos = new_vaos;
    vbos = new_vbos;
    vertices = new_vertices;
    graphics = new_graphics;
    gfx_ptrs_id_to_pos = new_gfx_ptrs_id_to_pos;
    gfx_ptrs_pos_to_id = new_gfx_ptrs_pos_to_id;
    state_for_gfx = new_state_for_gfx;
    layer_for_gfx = new_layer_for_gfx;

    glGenVertexArrays( size_diff, &vaos[ max_graphics + 1 ] );
    glGenBuffers( size_diff, &vbos[ max_graphics + 1 ] );
    for ( int i = max_graphics + 1; i < new_max_graphics + 1; ++i )
    {
        float * vptr = GetVertices( i );
        ResetVertices( vptr );
        BindBuffers( i );
        BufferDefault( vptr );
    }
    ClearBufferBindings();

    // Initialize these to null values ( since 0 is a valid value, we use -1 ).
    for ( int i = max_graphics; i < new_max_graphics; ++i )
    {
        gfx_ptrs_id_to_pos[ i ] = gfx_ptrs_pos_to_id[ i ] = state_for_gfx[ i ] = layer_for_gfx[ i ] = -1;
    }

    max_graphics = new_max_graphics;
    return 1;
};

static unsigned char * LoadTextureFileData( const char * filename, unsigned int * width, unsigned int * height, int sampling, int indexed )
{
    int channels;
    int w;
    int h;
    unsigned char * data = stbi_load( filename, &w, &h, &channels, STBI_rgb_alpha );
    if ( data == NULL || w < 0 || h < 0 )
    {
        NasrLog( "Couldn’t load texture file “%s”.", filename );
        return 0;
    }
    *width = w;
    *height = h;
    return data;
};

static void ResetVertices( float * vptr )
{
    memcpy( vptr, &vertices_base, sizeof( vertices_base ) );
};

static void SetShader( unsigned int shader )
{
    if ( current_shader != shader )
    {
        glUseProgram( shader );
        current_shader = shader;
    }
};

static void SetVerticesColors( unsigned int id, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    SetVerticesColorValues( vptr, top_left_color, top_right_color, bottom_left_color, bottom_right_color );
    ClearBufferBindings();
};

static void SetVerticesColorValues( float * vptr, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color )
{
    vptr[ 4 ] = bottom_right_color->r / 255.0f;
    vptr[ 5 ] = bottom_right_color->g / 255.0f;
    vptr[ 6 ] = bottom_right_color->b / 255.0f;
    vptr[ 7 ] = bottom_right_color->a / 255.0f;

    vptr[ 4 + VERTEX_SIZE ] = top_right_color->r / 255.0f;
    vptr[ 5 + VERTEX_SIZE ] = top_right_color->g / 255.0f;
    vptr[ 6 + VERTEX_SIZE ] = top_right_color->b / 255.0f;
    vptr[ 7 + VERTEX_SIZE ] = top_right_color->a / 255.0f;

    vptr[ 4 + VERTEX_SIZE * 2 ] = top_left_color->r / 255.0f;
    vptr[ 5 + VERTEX_SIZE * 2 ] = top_left_color->g / 255.0f;
    vptr[ 6 + VERTEX_SIZE * 2 ] = top_left_color->b / 255.0f;
    vptr[ 7 + VERTEX_SIZE * 2 ] = top_left_color->a / 255.0f;

    vptr[ 4 + VERTEX_SIZE * 3 ] = bottom_left_color->r / 255.0f;
    vptr[ 5 + VERTEX_SIZE * 3 ] = bottom_left_color->g / 255.0f;
    vptr[ 6 + VERTEX_SIZE * 3 ] = bottom_left_color->b / 255.0f;
    vptr[ 7 + VERTEX_SIZE * 3 ] = bottom_left_color->a / 255.0f;

    BufferVertices( vptr );
};

static void SetVerticesView( float x, float y, float scrollx, float scrolly )
{
    x += camera.x * scrollx;
    y += camera.y * scrolly;
    mat4 view = BASE_MATRIX;
    vec3 trans = { x, y, 0.0f };
    glm_translate( view, trans );
    unsigned int view_location = glGetUniformLocation( rect_shader, "view" );
    glUniformMatrix4fv( view_location, 1, GL_FALSE, ( float * )( view ) );
};

static void SetupVertices( unsigned int vao )
{
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
};

static uint32_t TextureMapHashString( const char * key )
{
    return NasrHashString( key, texture_map_size );
};

static void UpdateShaderOrtho( float x, float y, float w, float h )
{
    for ( int i = 0; i < NUMBER_O_BASE_SHADERS; ++i )
    {
        const unsigned int shader = *base_shaders[ i ];
        SetShader( shader );
        mat4 ortho =
        {
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f }
        };
        glm_ortho_rh_no( x, w, h, y, -1.0f, 1.0f, ortho );
        unsigned int ortho_location = glGetUniformLocation( shader, "ortho" );
        glUniformMatrix4fv( ortho_location, 1, GL_FALSE, ( float * )( ortho ) );
    }
};

static void UpdateShaderOrthoToCamera( void )
{
    UpdateShaderOrtho( camera.x, camera.y, camera.w, camera.h );
};

static void UpdateSpriteModel( unsigned int id )
{
    NasrGraphic * g = GetGraphic( id );
    mat4 model = BASE_MATRIX;
    memcpy( &g->data.sprite.model, &model, sizeof( model ) );
    vec3 scale = { g->data.sprite.dest.w, g->data.sprite.dest.h, 0.0 };
    glm_scale( g->data.sprite.model, scale );
    vec3 xrot = { 0.0, 1.0, 0.0 };
    glm_rotate( g->data.sprite.model, DEGREES_TO_RADIANS( g->data.sprite.rotation_x ), xrot );
    vec3 yrot = { 0.0, 0.0, 1.0 };
    glm_rotate( g->data.sprite.model, DEGREES_TO_RADIANS( g->data.sprite.rotation_y ), yrot );
    vec3 zrot = { 1.0, 0.0, 0.0 };
    glm_rotate( g->data.sprite.model, DEGREES_TO_RADIANS( g->data.sprite.rotation_z ), zrot );
};

static void UpdateSpriteVertices( unsigned int id )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    const NasrGraphicSprite * sprite = &graphics[ gfx_ptrs_id_to_pos[ id ] ].data.sprite;
    UpdateSpriteVerticesValues( vptr, sprite );
    ClearBufferBindings();
};

static void UpdateSpriteVerticesValues( float * vptr, const NasrGraphicSprite * sprite )
{
    const unsigned int texture_id = sprite->texture;
    if ( sprite->flip_x )
    {
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].width ) * sprite->src.x; // Left X
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture_id ].width ) * ( sprite->src.x + sprite->src.w );  // Right X
    }
    else
    {
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture_id ].width ) * sprite->src.x; // Left X
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].width ) * ( sprite->src.x + sprite->src.w );  // Right X
    }

    if ( sprite->flip_y )
    {
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].height ) * ( sprite->src.y + sprite->src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / ( float )( textures[ texture_id ].height ) * sprite->src.y;  // Bottom Y
    }
    else
    {
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / ( float )( textures[ texture_id ].height ) * ( sprite->src.y + sprite->src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].height ) * sprite->src.y;  // Bottom Y
    }
    BufferVertices( vptr );
};

static void UpdateSpriteX( unsigned int id )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    const NasrGraphicSprite * sprite = &graphics[ gfx_ptrs_id_to_pos[ id ] ].data.sprite;
    const unsigned int texture_id = sprite->texture;
    if ( sprite->flip_x )
    {
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].width ) * sprite->src.x; // Left X
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture_id ].width ) * ( sprite->src.x + sprite->src.w );  // Right X
    }
    else
    {
        vptr[ 2 + VERTEX_SIZE * 3 ] = vptr[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture_id ].width ) * sprite->src.x; // Left X
        vptr[ 2 ] = vptr[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].width ) * ( sprite->src.x + sprite->src.w );  // Right X
    }
    BufferVertices( vptr );
    ClearBufferBindings();
};

static void UpdateSpriteY( unsigned int id )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    const NasrGraphicSprite * sprite = &graphics[ gfx_ptrs_id_to_pos[ id ] ].data.sprite;
    const unsigned int texture_id = sprite->texture;
    if ( sprite->flip_y )
    {
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].height ) * ( sprite->src.y + sprite->src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / ( float )( textures[ texture_id ].height ) * sprite->src.y;  // Bottom Y
    }
    else
    {
        vptr[ 3 + VERTEX_SIZE * 3 ] = vptr[ 3 ] = 1.0f / ( float )( textures[ texture_id ].height ) * ( sprite->src.y + sprite->src.h ); // Top Y
        vptr[ 3 + VERTEX_SIZE * 2 ] = vptr[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].height ) * sprite->src.y;  // Bottom Y
    }
    BufferVertices( vptr );
    ClearBufferBindings();
};