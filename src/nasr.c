#include <stdio.h>
#include <cglm/cglm.h>
#include <cglm/call.h>
#include "json/json.h"
#include "nasr.h"
#include "nasr_log.h"
#include "nasr_math.h"
#include "nasr_io.h"

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

typedef struct KeyInputList {
    int count;
    int * data;
} KeyInputList;

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

static float * vertices;

#define MAX_ANIMATION_FRAME 2 * 3 * 4 * 5 * 6 * 7 * 8
#define NUMBER_O_BASE_SHADERS 8

typedef struct { NasrHashKey key; unsigned int value; } TextureMapEntry;

typedef struct CharTemplate
{
    NasrRect src;
    float xoffset;
    float yoffset;
    uint_fast8_t chartype;
} CharTemplate;
typedef struct { NasrHashKey key; CharTemplate value; } CharMapEntry;
typedef struct CharMap
{
    unsigned int capacity;
    unsigned int hashmax;
    CharMapEntry * list;
    unsigned int texture_id;
    Texture texture;
} CharMap;
typedef struct CharMapList
{
    unsigned int capacity;
    CharMap * list
} CharMapList;

static CharTemplate default_char_temp = { { 0.0f, 0.0f, 8.0f, 8.0f }, 0.0f, 0.0f, NASR_CHAR_NORMAL };

static int magnification = 1;
static GLFWwindow * window;
static unsigned int * vaos;
static unsigned int * vbos;
static unsigned int ebo;
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
static NasrGraphic * graphics = NULL;
static int max_graphics = 0;
static int num_o_graphics = 0;
static NasrRect camera = { 0.0f, 0.0f, 0.0f, 0.0f };
static NasrRect prev_camera = { 0.0f, 0.0f, 0.0f, 0.0f };
static NasrRect canvas = { 0.0f, 0.0f, 0.0f, 0.0f };
static int max_inputs = 0;
static int inputs_per_key = 0;
static int keys_per_input = 0;
static int key_input_size = 0;
static int input_key_size = 0;
static int input_keys_start = 0;
static int held_keys_start = 0;
static int held_start = 0;
static int * keydata = NULL;
static int max_textures;
static unsigned int * texture_ids;
static Texture * textures;
static int texture_count = 0;
static GLuint framebuffer = 0;
static GLint magnified_canvas_width = 0;
static GLint magnified_canvas_height = 0;
static GLint magnified_canvas_x = 0;
static GLint magnified_canvas_y = 0;
static int selected_texture = -1;
static GLint default_sample_type = GL_LINEAR;
static TextureMapEntry * texture_map;
static GLint default_indexed_mode = GL_RGBA;
static unsigned int palette_texture_id;
static Texture palette_texture;
static int max_states;
static int max_gfx_layers;
static int * layer_pos = NULL;
static int * gfx_ptrs_id_to_pos = NULL;
static int * gfx_ptrs_pos_to_id = NULL;
static int * state_for_gfx = NULL;
static int * layer_for_gfx = NULL;
static unsigned int animation_frame = 0;
static float animation_timer = 0;
static uint_fast8_t global_palette = 0;
static CharMapList charmaps = { 0, 0, 0 };
static float animation_ticks_per_frame = 0.0f;

static void UpdateSpriteX( unsigned int id );
static void UpdateSpriteY( unsigned int id );
static void FramebufferSizeCallback( GLFWwindow * window, int width, int height );
static unsigned int GenerateShaderProgram( const NasrShader * shaders, int shadersnum );
static void BufferVertices( float * vptr );
static void DrawBox( unsigned int vao, const NasrRect * rect, int abs );
static void SetVerticesColors( unsigned int id, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesColorValues( float * vptr, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesView( float x, float y, int abs );
static void SetupVertices( unsigned int vao );
static void UpdateShaderOrtho( void );
static void HandleInput( GLFWwindow * window, int key, int scancode, int action, int mods );
static int * GetKeyInputs( int key );
static int * GetInputKeys( int input );
static int * GetHeldKeys( int input );
static int * GetHeld( int id );
static GLint GetGLSamplingType( int sampling );
static TextureMapEntry * TextureMapHashFindEntry( const char * needle_string, hash_t needle_hash );
static uint32_t TextureMapHashString( const char * key );
static CharMapEntry * CharMapHashFindEntry( unsigned int id, const char * needle_string, hash_t needle_hash );
static CharMapEntry * CharMapGenEntry( unsigned int id, const char * key );
static uint32_t CharMapHashString( unsigned int id, const char * key );
static GLint GetGLRGBA( int indexed );
static unsigned char * LoadTextureFileData( const char * filename, unsigned int * width, unsigned int * height, int sampling, int indexed );
static void AddTexture( Texture * texture, unsigned int texture_id, const unsigned char * data, unsigned int width, unsigned int height, int sampling, int indexed );
static unsigned int GetStateLayerIndex( unsigned int state, unsigned int layer );
static float * GetVertices( unsigned int id );
static void ResetVertices( float * vptr );
static void UpdateSpriteVertices( unsigned int id );
static void UpdateSpriteVerticesValues( float * vptr, const NasrGraphicSprite * sprite );
static void BindBuffers( unsigned int id );
static void ClearBufferBindings( void );
static int GraphicAddText
(
    int abs,
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
static void DestroyGraphic( NasrGraphic * graphic );
static int getCharacterSize( const char * s );

void NasrColorPrint( const NasrColor * c )
{
    printf( "NasrColor: %f, %f, %f, %f\n", c->r, c->g, c->b, c->a );
};

float NasrRectRight( const NasrRect * r )
{
    return r->x + r->w;
};

float NasrRectBottom( const NasrRect * r )
{
    return r->y + r->h;
};

NasrRectInt NasrRectToNasrRectInt( const struct NasrRect r )
{
    NasrRectInt r2;
    r2.x = ( int )( r.x );
    r2.y = ( int )( r.y );
    r2.w = ( int )( r.w );
    r2.h = ( int )( r.h );
    return r2;
};

int NasrRectEqual( const struct NasrRect * a, const struct NasrRect * b )
{
    return a->x == b->x
        && a->y == b->y
        && a->w == b->w
        && a->h == b->h;
};

void NasrRectPrint( const NasrRect * r )
{
    printf( "NasrRect: %f, %f, %f, %f\n", r->x, r->y, r->w, r->h );
};

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

    // Init Input
    glfwSetKeyCallback( window, HandleInput );

    // Turn on blending.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Init textures list
    max_textures = init_max_textures;
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
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform float opacity;\n  \nvoid main()\n{\n    final_color = texture( texture_data, texture_coords );\n    final_color.a *= opacity;\n}"
        }
    };

    NasrShader indexed_sprite_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform float palette_id;\nuniform float opacity;\n\nvoid main()\n{\n    vec4 index = texture( texture_data, texture_coords );\n    float palette = palette_id / 256.0;\n    final_color = texture( palette_data, vec2( index.r, palette ) );\n    final_color.a *= opacity;\n}"
        }
    };

    NasrShader tilemap_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform sampler2D map_data;\nuniform float map_width;\nuniform float map_height;\nuniform float tileset_width;\nuniform float tileset_height;\nuniform uint animation;\n  \nvoid main()\n{\n    vec4 tile = texture( map_data, texture_coords );\n    if ( tile.a > 0.0 && tile.a < 1.0 )\n    {\n        float frames = floor( tile.a * 255.0 );\n        float frame = mod( float( animation ), frames );\n        // I don’t know why mod sometimes doesn’t work right & still sometimes says 6 is the mod o’ 6 / 6 ’stead o’ 0;\n        // This fixes it.\n        while ( frame >= frames )\n        {\n            frame -= frames;\n        }\n        tile.x += frame / 255.0;\n    }\n    float xrel = mod( texture_coords.x * 256.0, ( 256.0 / map_width ) ) / ( 4096.0 / map_width );\n    float yrel = mod( texture_coords.y * 256.0, ( 256.0 / map_height ) ) / ( 4096.0 / map_height );\n    float xoffset = tile.x * 255.0 * ( 16 / tileset_width );\n    float yoffset = tile.y * 255.0 * ( 16 / tileset_height );\n    float palette = tile.z;\n    vec4 index = texture( texture_data, vec2( xoffset + ( xrel / ( tileset_width / 256.0 ) ), yoffset + ( yrel / ( tileset_height / 256.0 ) ) ) );\n    final_color = ( tile.a < 1.0 ) ? texture( palette_data, vec2( index.r, palette ) ) : vec4( 0.0, 0.0, 0.0, 0.0 );\n}"
        }
    };

    NasrShader tilemap_mono_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform sampler2D map_data;\nuniform float map_width;\nuniform float map_height;\nuniform float tileset_width;\nuniform float tileset_height;\nuniform uint animation;\nuniform uint global_palette;\n  \nvoid main()\n{\n    vec4 tile = texture( map_data, texture_coords );\n    if ( tile.a > 0.0 && tile.a < 1.0 )\n    {\n        float frames = floor( tile.a * 255.0 );\n        float frame = mod( float( animation ), frames );\n        // I don’t know why mod sometimes doesn’t work right & still sometimes says 6 is the mod o’ 6 / 6 ’stead o’ 0;\n        // This fixes it.\n        while ( frame >= frames )\n        {\n            frame -= frames;\n        }\n        tile.x += frame / 255.0;\n    }\n    float xrel = mod( texture_coords.x * 256.0, ( 256.0 / map_width ) ) / ( 4096.0 / map_width );\n    float yrel = mod( texture_coords.y * 256.0, ( 256.0 / map_height ) ) / ( 4096.0 / map_height );\n    float xoffset = tile.x * 255.0 * ( 16 / tileset_width );\n    float yoffset = tile.y * 255.0 * ( 16 / tileset_height );\n    float palette = float( global_palette ) / 255.0;\n    vec4 index = texture( texture_data, vec2( xoffset + ( xrel / ( tileset_width / 256.0 ) ), yoffset + ( yrel / ( tileset_height / 256.0 ) ) ) );\n    final_color = ( tile.a < 1.0 ) ? texture( palette_data, vec2( index.r, palette ) ) : vec4( 0.0, 0.0, 0.0, 0.0 );\n}"
        }
    };

    NasrShader text_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform float shadow;\n  \nvoid main()\n{\n    vec4 texture_color = texture( texture_data, texture_coords );\n    if ( texture_color.r < 1.0 )\n    {\n        final_color = vec4( vec3( out_color.rgb * texture_color.rgb ), texture_color.a * shadow );\n    }\n    else\n    {\n        final_color = out_color * texture_color;\n    }\n}"
        }
    };

    NasrShader text_pal_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform float palette_id;\nuniform float shadow;\n\nvoid main()\n{\n    float palette = palette_id / 256.0;\n    vec4 texture_color = texture( texture_data, texture_coords );\n    final_color = texture_color * texture( palette_data, vec2( out_color.r, palette ) );\n    final_color.a *= texture_color.a;\n    if ( texture_color.r < 1.0 )\n    {\n        final_color.a *= shadow;\n    }\n}"
        }
    };

    NasrShader rect_pal_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\n\nuniform sampler2D palette_data;\nuniform float palette_id;\n\nvoid main()\n{\n    float palette = palette_id / 256.0;\n    final_color = texture( palette_data, vec2( out_color.r, palette ) );\n}"
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

    // Init camera
    NasrResetCamera();
    UpdateShaderOrtho();

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
    texture_map = calloc( max_textures, sizeof( TextureMapEntry ) );

    // Set framerate
    glfwSwapInterval( vsync );

    return 0;
};

double NasrGetTime( void )
{
    return glfwGetTime();
};

void NasrSetPalette( const char * filename )
{
    unsigned int width;
    unsigned int height;
    const unsigned char * data = LoadTextureFileData( filename, &width, &height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    AddTexture( &palette_texture, palette_texture_id, data, width, height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    free( data );
};

int NasrAddCharset( const char * texture, const char * chardata )
{
    char * text = NasrReadFile( chardata );
    if ( !text )
    {
        return -1;
    }
    json_char * json = ( json_char * )( text );
    json_value * root = json_parse( json, strlen( text ) + 1 );
    free( text );
    if ( !root || root->type != json_object )
    {
        NasrLog( "¡Charset failed to load!\n" );
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
                NasrLog( "Charset file malformed: characters is not an array." );
                return -1;
            }

            CharTemplate chars[ root_entry.value->u.array.length ];
            char * keys[ root_entry.value->u.array.length ];

            for ( unsigned int j = 0; j < root_entry.value->u.array.length; ++j )
            {
                const json_value * char_item = root_entry.value->u.array.values[ j ];
                if ( char_item->type != json_object )
                {
                    NasrLog( "Charset file malformed: character entry not an object." );
                    return -1;
                }

                // Setup defaults.
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
                            NasrLog( "Charset file malformed: character key is not a string." );
                            return -1;
                        }
                        keys[ j ] = char_entry.value->u.string.ptr;
                    }
                    else if ( strcmp( "type", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            NasrLog( "Charset file malformed: character type is not a string." );
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
                            NasrLog( "Charset file malformed: x is not an int." );
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
                            NasrLog( "Charset file malformed: y is not an int." );
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
                            NasrLog( "Charset file malformed: w is not an int." );
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
                            NasrLog( "Charset file malformed: h is not an int." );
                            return -1;
                        }
                    }
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

            charmaps.list[ id ].capacity = root_entry.value->u.array.length + 2;

            while ( !NasrMathIsPrime( charmaps.list[ id ].capacity ) )
            {
                ++charmaps.list[ id ].capacity;
            }

            charmaps.list[ id ].hashmax = charmaps.list[ id ].capacity;
            charmaps.list[ id ].list = calloc( charmaps.list[ id ].capacity, sizeof( CharMapEntry ) );

            unsigned int width;
            unsigned int height;
            const unsigned char * data = LoadTextureFileData( texture, &width, &height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
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
        }
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

void NasrClose( void )
{
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
    if ( vertices )
    {
        free( vertices );
    }
    glDeleteFramebuffers( 1, &framebuffer );
    NasrClearTextures();
    if ( texture_map )
    {
        free( texture_map );
    }
    if ( keydata )
    {
        free( keydata );
    }
    if ( textures )
    {
        free( textures );
    }
    glDeleteTextures( 1, &palette_texture_id );
    if ( texture_ids )
    {
        glDeleteTextures( max_textures, texture_ids );
        free( texture_ids );
    }
    if ( graphics )
    {
        free( graphics );
    }
    if ( gfx_ptrs_id_to_pos )
    {
        free( gfx_ptrs_id_to_pos );
    }
    if ( gfx_ptrs_pos_to_id )
    {
        free( gfx_ptrs_pos_to_id );
    }
    if ( layer_pos )
    {
        free( layer_pos );
    }
    if ( state_for_gfx )
    {
        free( state_for_gfx );
    }
    if ( layer_for_gfx )
    {
        free( layer_for_gfx );
    }
    glfwTerminate();
};

void NasrClearTextures( void )
{
    for ( int i = 0; i < max_textures; ++i )
    {
        if
        (
            texture_map[ i ].key.string != NULL
        )
        {
            free( texture_map[ i ].key.string );
            texture_map[ i ].key.string = 0;
            texture_map[ i ].key.hash = 0;
        }
    }
    texture_count = 0;
};

void NasrUpdate( float dt )
{
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    
    if ( camera.x != prev_camera.x || camera.y != prev_camera.y )
    {
        UpdateShaderOrtho();
    }

    for ( int i = 0; i < num_o_graphics; ++i )
    {
        const unsigned int id = ( unsigned int )( gfx_ptrs_pos_to_id[ i ] );
        const unsigned vao = vaos[ id ];
        float * vptr = GetVertices( id );
        BindBuffers( id );
        switch ( graphics[ i ].type )
        {
            case ( NASR_GRAPHIC_RECT ):
            {
                DrawBox
                (
                    vao,
                    &graphics[ i ].data.rect.rect,
                    graphics[ i ].abs
                );
            }
            break;
            case ( NASR_GRAPHIC_RECT_GRADIENT ):
            {
                DrawBox
                (
                    vao,
                    &graphics[ i ].data.gradient.rect,
                    graphics[ i ].abs
                );
            }
            break;
            case ( NASR_GRAPHIC_RECT_PAL ):
            {
                #define RECT graphics[ i ].data.rectpal.rect
                glUseProgram( rect_pal_shader );
                SetVerticesView( RECT.x + ( RECT.w / 2.0f ), RECT.y + ( RECT.h / 2.0f ), abs );
                mat4 model = BASE_MATRIX;
                vec3 scale = { RECT.w, RECT.h, 0.0 };
                glm_scale( model, scale );
                unsigned int model_location = glGetUniformLocation( rect_pal_shader, "model" );
                glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );
                GLint palette_id_location = glGetUniformLocation( rect_pal_shader, "palette_id" );
                glUniform1f( palette_id_location, ( float )( graphics[ i ].data.rectpal.useglobalpal ? global_palette : graphics[ i ].data.rectpal.palette ) );
                GLint palette_data_location = glGetUniformLocation( rect_pal_shader, "palette_data" );
                glActiveTexture( GL_TEXTURE1 );
                glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                glUniform1i( palette_data_location, 1 );
                #undef RECT
            }
            break;
            case ( NASR_GRAPHIC_SPRITE ):
            {
                #define SPRITE graphics[ i ].data.sprite
                #define SRC SPRITE.src
                #define DEST SPRITE.dest
                unsigned int texture_id = ( unsigned int )( SPRITE.texture );
                const unsigned int shader = textures[ texture_id ].indexed ? indexed_sprite_shader : sprite_shader;
                glUseProgram( shader );

                SetVerticesView( DEST.x + ( DEST.w / 2.0f ), DEST.y + ( DEST.h / 2.0f ), graphics[ i ].abs );

                mat4 model = BASE_MATRIX;
                vec3 scale = { DEST.w, DEST.h, 0.0 };
                glm_scale( model, scale );
                vec3 xrot = { 0.0, 1.0, 0.0 };
                glm_rotate( model, DEGREES_TO_RADIANS( SPRITE.rotation_x ), xrot );
                vec3 yrot = { 0.0, 0.0, 1.0 };
                glm_rotate( model, DEGREES_TO_RADIANS( SPRITE.rotation_y ), yrot );
                vec3 zrot = { 1.0, 0.0, 0.0 };
                glm_rotate( model, DEGREES_TO_RADIANS( SPRITE.rotation_z ), zrot );
                unsigned int model_location = glGetUniformLocation( shader, "model" );
                glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

                if ( textures[ texture_id ].indexed )
                {
                    GLint palette_id_location = glGetUniformLocation( shader, "palette_id" );
                    glUniform1f( palette_id_location, ( float )( SPRITE.useglobalpal ? global_palette : SPRITE.palette ) );
                }

                GLint opacity_location = glGetUniformLocation( shader, "opacity" );
                glUniform1f( opacity_location, ( float )( SPRITE.opacity ) );

                GLint texture_data_location = glGetUniformLocation( shader, "texture_data" );
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, texture_ids[ texture_id ] );
                glUniform1i( texture_data_location, 0 );
                if ( textures[ texture_id ].indexed )
                {
                    GLint palette_data_location = glGetUniformLocation(shader, "palette_data");
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, palette_texture_id );
                    glUniform1i(palette_data_location, 1);
                }

                #undef SPRITE
                #undef SRC
                #undef DEST
            }
            break;
            case ( NASR_GRAPHIC_TILEMAP ):
            {
                #define TG graphics[ i ].data.tilemap
                const unsigned int shader = TG.useglobalpal ? tilemap_mono_shader : tilemap_shader;
                glUseProgram( shader );

                SetVerticesView( TG.dest.x + ( TG.dest.w / 2.0f ), TG.dest.y + ( TG.dest.h / 2.0f ), graphics[ i ].abs );

                mat4 model = BASE_MATRIX;
                vec3 scale = { TG.dest.w, TG.dest.h, 0.0 };
                glm_scale( model, scale );
                unsigned int model_location = glGetUniformLocation( shader, "model" );
                glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

                GLint map_width_location = glGetUniformLocation( shader, "map_width" );
                glUniform1f( map_width_location, ( float )( textures[ TG.tilemap ].width ) );

                GLint map_height_location = glGetUniformLocation( shader, "map_height" );
                glUniform1f( map_height_location, ( float )( textures[ TG.tilemap ].height ) );

                GLint tileset_width_location = glGetUniformLocation( shader, "tileset_width" );
                glUniform1f( tileset_width_location, ( float )( textures[ TG.texture ].width ) );

                GLint tileset_height_location = glGetUniformLocation( shader, "tileset_height" );
                glUniform1f( tileset_height_location, ( float )( textures[ TG.texture ].height ) );

                GLint animation_location = glGetUniformLocation( shader, "animation" );
                glUniform1ui( animation_location, animation_frame );

                if ( TG.useglobalpal )
                {
                    GLint global_palette_location = glGetUniformLocation( shader, "global_palette" );
                    glUniform1ui( global_palette_location, ( GLuint )( global_palette ) );
                }

                GLint texture_data_location = glGetUniformLocation(shader, "texture_data");
                GLint palette_data_location = glGetUniformLocation(shader, "palette_data");
                GLint map_data_location = glGetUniformLocation(shader, "map_data");
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_ids[ TG.texture ] );
                glUniform1i(texture_data_location, 0);
                glActiveTexture(GL_TEXTURE1 );
                glBindTexture(GL_TEXTURE_2D, palette_texture_id );
                glUniform1i(palette_data_location, 1);
                glActiveTexture(GL_TEXTURE2 );
                glBindTexture(GL_TEXTURE_2D, texture_ids[ TG.tilemap ] );
                glUniform1i(map_data_location, 2);

                #undef TG
            }
            break;
            case ( NASR_GRAPHIC_TEXT ):
            {
                const unsigned int shader = graphics[ i ].data.text.palette_type ? text_pal_shader : text_shader;
                glUseProgram( shader );

                for ( int j = 0; j < graphics[ i ].data.text.count; ++j )
                {
                    #define CHAR graphics[ i ].data.text.chars[ j ]
                    
                    glBindVertexArray( graphics[ i ].data.text.vaos[ j ] );
                    glBindBuffer( GL_ARRAY_BUFFER, graphics[ i ].data.text.vbos[ j ] );
                    SetVerticesView( CHAR.dest.x + ( CHAR.dest.w / 2.0f ) + graphics[ i ].data.text.xoffset, CHAR.dest.y + ( CHAR.dest.h / 2.0f ) + graphics[ i ].data.text.yoffset, graphics[ i ].abs );

                    mat4 model = BASE_MATRIX;
                    vec3 scale = { CHAR.dest.w, CHAR.dest.h, 0.0 };
                    glm_scale( model, scale );
                    unsigned int model_location = glGetUniformLocation( shader, "model" );
                    glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );
                    GLint shadow_location = glGetUniformLocation( shader, "shadow" );
                    glUniform1f( shadow_location, graphics[ i ].data.text.shadow );

                    if ( graphics[ i ].data.text.palette_type )
                    {
                        GLint palette_id_location = glGetUniformLocation( shader, "palette_id" );
                        glUniform1f( palette_id_location, ( float )( graphics[ i ].data.text.palette_type == NASR_PALETTE_DEFAULT ? global_palette : graphics[ i ].data.text.palette ) );
                    }

                    GLint texture_data_location = glGetUniformLocation( shader, "texture_data" );
                    glActiveTexture( GL_TEXTURE0 );
                    glBindTexture( GL_TEXTURE_2D, charmaps.list[ graphics[ i ].data.text.charset ].texture_id );
                    glUniform1i( texture_data_location, 0 );
                    if ( graphics[ i ].data.text.palette_type )
                    {
                        GLint palette_data_location = glGetUniformLocation( shader, "palette_data" );
                        glActiveTexture( GL_TEXTURE1 );
                        glBindTexture( GL_TEXTURE_2D, palette_texture_id );
                        glUniform1i( palette_data_location, 1 );
                    }

                    SetupVertices( graphics[ i ].data.text.vaos[ j ] );
                    ClearBufferBindings();

                    #undef CHAR
                }
            }
            break;
            default:
            {
                printf( "¡Trying to render invalid graphic type #%d!\n", graphics[ i ].type );
            }
            break;
        }
        SetupVertices( vao );
        ClearBufferBindings();
    }

    glfwSwapBuffers( window );
    glfwPollEvents();
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

int NasrHasClosed( void )
{
    return glfwWindowShouldClose( window );
};

void NasrResetCamera( void )
{
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.w = canvas.w;
    camera.h = canvas.h;
};

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

static void BufferVertices( float * vptr )
{
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * VERTEX_RECT_SIZE, vptr, GL_STATIC_DRAW );
};

static void DrawBox( unsigned int vao, const NasrRect * rect, int abs )
{
    glUseProgram( rect_shader );
    SetVerticesView( rect->x + ( rect->w / 2.0f ), rect->y + ( rect->h / 2.0f ), abs );
    mat4 model = BASE_MATRIX;
    vec3 scale = { rect->w, rect->h, 0.0 };
    glm_scale( model, scale );
    unsigned int model_location = glGetUniformLocation( rect_shader, "model" );
    glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );
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

static void SetVerticesView( float x, float y, int abs )
{
    if ( abs )
    {
        x += camera.x;
        y += camera.y;
    }
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

static void UpdateShaderOrtho( void )
{
    for ( int i = 0; i < NUMBER_O_BASE_SHADERS; ++i )
    {
        const unsigned int shader = *base_shaders[ i ];
        glUseProgram( shader );
        mat4 ortho =
        {
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f }
        };
        glm_ortho_rh_no( camera.x, camera.w, camera.h, camera.y, -1.0f, 1.0f, ortho );
        unsigned int ortho_location = glGetUniformLocation( shader, "ortho" );
        glUniformMatrix4fv( ortho_location, 1, GL_FALSE, ( float * )( ortho ) );
    }
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

NasrGraphic * NasrGraphicGet( unsigned int id )
{
    return &graphics[ gfx_ptrs_id_to_pos[ id ] ];
};

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

int NasrGraphicsAdd
(
    unsigned int state,
    unsigned int layer,
    struct NasrGraphic graphic
)
{
    if ( num_o_graphics >= max_graphics )
    {
        return -1;
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

int NasrGraphicsAddCanvas
(
    int abs,
    unsigned int state,
    unsigned int layer,
    NasrColor color
)
{
    return NasrGraphicsAddRect
    (
        abs,
        state,
        layer,
        canvas,
        color
    );
};

int NasrGraphicsAddRect
(
    int abs,
    unsigned int state,
    unsigned int layer,
    NasrRect rect,
    NasrColor color
)
{
    struct NasrGraphic graphic;
    graphic.abs = abs;
    graphic.type = NASR_GRAPHIC_RECT;
    graphic.data.rect.rect = rect;
    graphic.data.rect.color = color;
    int id = NasrGraphicsAdd( state, layer, graphic );
    if ( id > -1 ) {
        ResetVertices( GetVertices( id ) );
        SetVerticesColors( id, &graphic.data.rect.color, &graphic.data.rect.color, &graphic.data.rect.color, &graphic.data.rect.color );
    }
    return id;
};

int NasrGraphicsAddRectGradient
(
    int abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    int dir,
    struct NasrColor color1,
    struct NasrColor color2
)
{
    struct NasrGraphic graphic;
    graphic.abs = abs;
    graphic.type = NASR_GRAPHIC_RECT_GRADIENT;
    graphic.data.gradient.rect = rect;
    switch ( dir )
    {
        case ( NASR_DIR_UP ):
        {
            graphic.data.gradient.color1 = color2;
            graphic.data.gradient.color2 = color2;
            graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color1;
        }
        break;
        case ( NASR_DIR_UPRIGHT ):
        {
            graphic.data.gradient.color1 = color1;
            graphic.data.gradient.color2 = color2;
            graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color1;
        }
        break;
        case ( NASR_DIR_RIGHT ):
        {
            graphic.data.gradient.color1 = color1;
            graphic.data.gradient.color2 = color2;
            graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color2;
        }
        break;
        case ( NASR_DIR_DOWNRIGHT ):
        {
            graphic.data.gradient.color1 = color1;
            graphic.data.gradient.color2 = color1;
            graphic.data.gradient.color3 = color2;
            graphic.data.gradient.color4 = color1;
        }
        break;
        case ( NASR_DIR_DOWN ):
        {
            graphic.data.gradient.color1 = color1;
            graphic.data.gradient.color2 = color1;
            graphic.data.gradient.color3 = color2;
            graphic.data.gradient.color4 = color2;
        }
        break;
        case ( NASR_DIR_DOWNLEFT ):
        {
            graphic.data.gradient.color1 = color1;
            graphic.data.gradient.color2 = color1;
            graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color2;
        }
        break;
        case ( NASR_DIR_LEFT ):
        {
            graphic.data.gradient.color1 = color2;
            graphic.data.gradient.color2 = color1;
            graphic.data.gradient.color3 = color2;
            graphic.data.gradient.color4 = color1;
        }
        break;
        case ( NASR_DIR_UPLEFT ):
        {
            graphic.data.gradient.color1 = color2;
            graphic.data.gradient.color2 = color1;
            graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color1;
        }
        break;
        default:
        {
            printf( "¡Invalid gradient direction for NasrGraphicsAddRectGradient! %d\n", dir );

            // Default direction.
            graphic.data.gradient.color1 = color2;
            graphic.data.gradient.color2 = color2;
            graphic.data.gradient.color3 = color1;
            graphic.data.gradient.color4 = color1;
        }
        break;
    }
    const int id = NasrGraphicsAdd( state, layer, graphic );
    if ( id > -1 )
    {
        ResetVertices( GetVertices( id ) );
        SetVerticesColors( id, &graphic.data.gradient.color1, &graphic.data.gradient.color2, &graphic.data.gradient.color3, &graphic.data.gradient.color4 );
    }
    return id;
};

int NasrGraphicsAddRectPalette
(
    int abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    uint_fast8_t palette,
    uint_fast8_t color,
    uint_fast8_t useglobalpal
)
{
    NasrColor c =
    {
        ( float )( color ),
        0.0f,
        0.0f,
        255.0f
    };
    struct NasrGraphic graphic;
    graphic.abs = abs;
    graphic.type = NASR_GRAPHIC_RECT_PAL;
    graphic.data.rectpal.rect = rect;
    graphic.data.rectpal.palette = palette;
    graphic.data.rectpal.useglobalpal = useglobalpal;
    const int id = NasrGraphicsAdd( state, layer, graphic );
    if ( id > -1 )
    {
        ResetVertices( GetVertices( id ) );
        SetVerticesColors( id, &c, &c, &c, &c );
    }
    return id;
};


int NasrGraphicsAddRectGradientPalette
(
    int abs,
    unsigned int state,
    unsigned int layer,
    struct NasrRect rect,
    uint_fast8_t palette,
    uint_fast8_t dir,
    uint_fast8_t color1,
    uint_fast8_t color2,
    uint_fast8_t useglobalpal
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
            printf( "¡Invalid gradient direction for NasrGraphicsAddRectGradientPalette! %d\n", dir );

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

    struct NasrGraphic graphic;
    graphic.abs = abs;
    graphic.type = NASR_GRAPHIC_RECT_PAL;
    graphic.data.rectpal.rect = rect;
    graphic.data.rectpal.palette = palette;
    graphic.data.rectpal.useglobalpal = useglobalpal;
    const int id = NasrGraphicsAdd( state, layer, graphic );
    if ( id > -1 )
    {
        ResetVertices( GetVertices( id ) );
        SetVerticesColors( id, &cobj[ 0 ], &cobj[ 1 ], &cobj[ 2 ], &cobj[ 3 ] );
    }
    return id;
};

int NasrGraphicsAddSprite
(
    int abs,
    unsigned int state,
    unsigned int layer,
    int texture,
    NasrRect src,
    NasrRect dest,
    int flip_x,
    int flip_y,
    float rotation_x,
    float rotation_y,
    float rotation_z,
    float opacity,
    unsigned char palette,
    int_fast8_t useglobalpal
)
{
    if ( num_o_graphics >= max_graphics )
    {
        return -1;
    }
    struct NasrGraphic graphic;
    graphic.abs = abs;
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
    const int id = NasrGraphicsAdd( state, layer, graphic );
    if ( id > -1 )
    {
        ResetVertices( GetVertices( id ) );
        UpdateSpriteVertices( id );
    }
    return id;
};

int NasrGraphicsAddTilemap
(
    int abs,
    unsigned int state,
    unsigned int layer,
    unsigned int texture,
    const NasrTile * tiles,
    unsigned int w,
    unsigned int h,
    int_fast8_t useglobalpal
)
{
    // Generate texture from tile data.
    unsigned char * data = ( unsigned char * )( calloc( w * h * 4, sizeof( unsigned char ) ) );
    int i4 = 0;
    for ( int i = 0; i < w * h; ++i )
    {
        data[ i4 ] = tiles[ i ].x;
        data[ i4 + 1 ] = tiles[ i ].y;
        data[ i4 + 2 ] = tiles[ i ].palette;
        data[ i4 + 3 ] = tiles[ i ].animation;
        i4 += 4;
    }
    if ( data == NULL )
    {
        NasrLog( "Couldn’t generate tilemap." );
        return -1;
    }
    const int tilemap_texture = NasrAddTextureEx( data, w, h, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    free( data );
    if ( tilemap_texture < 0 )
    {
        return -1;
    }

    struct NasrGraphic graphic;
    graphic.abs = abs;
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
    const int id = NasrGraphicsAdd( state, layer, graphic );
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

int NasrGraphicAddText
(
    int abs,
    unsigned int state,
    unsigned int layer,
    NasrText text,
    NasrColor color
)
{
    return GraphicAddText
    (
        abs,
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

int NasrGraphicAddTextGradient
(
    int abs,
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
            printf( "¡Invalid gradient direction for NasrGraphicAddTextGradient! %d\n", dir );

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
        abs,
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

int NasrGraphicAddTextPalette
(
    int abs,
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
        abs,
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

int NasrGraphicAddTextGradientPalette
(
    int abs,
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
            printf( "¡Invalid gradient direction for NasrGraphicAddTextGradientPalette! %d\n", dir );

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
        abs,
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

static int getCharacterSize( const char * s )
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
            if ( graphic->data.text.vertices )
            {
                free( graphic->data.text.vertices );
            }
            if ( graphic->data.text.chars )
            {
                free( graphic->data.text.chars );
            }
            graphic->type = NASR_GRAPHIC_NONE;
        }
        break;
    }
};

static int GraphicAddText
(
    int abs,
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
    float chary = text.coords.y + text.padding_bottom;
    const float lnend = charx + charw;

    char * string = text.string;
    CharTemplate letters[ strlen( string ) ];
    int lettercount = 0;
    while ( *string )
    {
        const int charlen = getCharacterSize( string );

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

    if ( num_o_graphics >= max_graphics )
    {
        return -1;
    }

    struct NasrGraphic graphic;
    graphic.abs = abs;
    graphic.type = NASR_GRAPHIC_TEXT;
    graphic.data.text.charset = text.charset;
    graphic.data.text.palette = palette;
    graphic.data.text.palette_type = palette_type;
    graphic.data.text.capacity = graphic.data.text.count = count;
    graphic.data.text.xoffset = text.xoffset;
    graphic.data.text.yoffset = text.yoffset;
    graphic.data.text.shadow = text.shadow;
    graphic.data.text.vaos = calloc( count, sizeof( unsigned int ) );
    graphic.data.text.vbos = calloc( count, sizeof( unsigned int ) );
    graphic.data.text.vertices = calloc( count * VERTEX_RECT_SIZE, sizeof( float ) );
    graphic.data.text.chars = calloc( count, sizeof( NasrChar ) );
    memcpy( graphic.data.text.chars, chars, count * sizeof( NasrChar ) );
    const int id = NasrGraphicsAdd( state, layer, graphic );

    NasrGraphic * g = NasrGraphicGet( id );
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

        BufferVertices( vptr );
       
        #undef CHARACTER

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
    }
    ClearBufferBindings();

    return id;
};

NasrRect NasrGraphicsSpriteGetDest( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.dest;
};

void NasrGraphicsSpriteSetDest( unsigned int id, NasrRect v )
{
    NasrGraphicGet( id )->data.sprite.dest = v;
};

float NasrGraphicsSpriteGetDestY( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.dest.y;
};

void NasrGraphicsSpriteSetDestY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.dest.y = v;
};

void NasrGraphicsSpriteAddToDestY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.dest.y += v;
};

float NasrGraphicsSpriteGetDestX( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.dest.x;
};

void NasrGraphicsSpriteSetDestX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.dest.x = v;
};

void NasrGraphicsSpriteAddToDestX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.dest.x += v;
};

float NasrGraphicsSpriteGetDestH( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.dest.h;
};

void NasrGraphicsSpriteSetDestH( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.dest.h = v;
};

float NasrGraphicsSpriteGetDestW( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.dest.w;
};

void NasrGraphicsSpriteSetDestW( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.dest.w = v;
};

float NasrGraphicsSpriteGetSrcY( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.src.y;
};

void NasrGraphicsSpriteSetSrcY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.src.y = v;
    UpdateSpriteVertices( id );
};

float NasrGraphicsSpriteGetSrcX( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.src.x;
};

void NasrGraphicsSpriteSetSrcX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.src.x = v;
    UpdateSpriteVertices( id );
};

float NasrGraphicsSpriteGetSrcH( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.src.h;
};

void NasrGraphicsSpriteSetSrcH( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.src.h = v;
    UpdateSpriteVertices( id );
};

float NasrGraphicsSpriteGetSrcW( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.src.w;
};

void NasrGraphicsSpriteSetSrcW( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.src.w = v;
    UpdateSpriteVertices( id );
};

float NasrGraphicsSpriteGetRotationX( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.rotation_x;
};

void NasrGraphicsSpriteSetRotationX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.rotation_x = v;
};

float NasrGraphicsSpriteGetRotationY( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.rotation_y;
};

void NasrGraphicsSpriteSetRotationY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.rotation_y = v;
};

unsigned char NasrGraphicsSpriteGetPalette( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.palette;
};

void NasrGraphicsSpriteSetPalette( unsigned int id, unsigned char v )
{
    NasrGraphicGet( id )->data.sprite.palette = v;
};

float NasrGraphicsSpriteGetRotationZ( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.rotation_z;
};

void NasrGraphicsSpriteSetRotationZ( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.rotation_z = v;
};

float NasrGraphicsSpriteGetOpacity( unsigned int id )
{
    return NasrGraphicGet( id )->data.sprite.opacity;
};

void NasrGraphicsSpriteSetOpacity( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.sprite.opacity = v;
};

int NasrGraphicsSpriteGetFlipX( unsigned id )
{
    return NasrGraphicGet( id )->data.sprite.flip_x;
};

void NasrGraphicsSpriteSetFlipX( unsigned id, int v )
{
    NasrGraphicGet( id )->data.sprite.flip_x = v;
    UpdateSpriteX( id );
};

void NasrGraphicsSpriteFlipX( unsigned id )
{
    NasrGraphicGet( id )->data.sprite.flip_x = !NasrGraphicGet( id )->data.sprite.flip_x;
    UpdateSpriteX( id );
};

int NasrGraphicsSpriteGetFlipY( unsigned id )
{
    return NasrGraphicGet( id )->data.sprite.flip_y;
};

void NasrGraphicsSpriteSetFlipY( unsigned id, int v )
{
    NasrGraphicGet( id )->data.sprite.flip_y = v;
    UpdateSpriteY( id );
};

void NasrGraphicsSpriteFlipY( unsigned id )
{
    NasrGraphicGet( id )->data.sprite.flip_y = !NasrGraphicGet( id )->data.sprite.flip_y;
    UpdateSpriteY( id );
};

float NasrGraphicsRectGetX( unsigned int id )
{
    return NasrGraphicGet( id )->data.rect.rect.x;
};

void NasrGraphicsRectSetX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.x = v;
};

void NasrGraphicsRectAddToX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.x += v;
};

float NasrGraphicsRectGetY( unsigned int id )
{
    return NasrGraphicGet( id )->data.rect.rect.y;
};

void NasrGraphicsRectSetY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.y = v;
};

void NasrGraphicsRectAddToY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.y += v;
};

float NasrGraphicsRectGetW( unsigned int id )
{
    return NasrGraphicGet( id )->data.rect.rect.w;
};

void NasrGraphicsRectSetW( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.w = v;
};

void NasrGraphicsRectAddToW( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.w += v;
};

float NasrGraphicsRectGetH( unsigned int id )
{
    return NasrGraphicGet( id )->data.rect.rect.h;
};

void NasrGraphicsRectSetH( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.h = v;
};

void NasrGraphicsRectAddToH( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.rect.rect.h += v;
};

void NasrGraphicRectSetColor( unsigned int id, NasrColor v )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 4 ] = vptr[ 4 + VERTEX_SIZE ] = vptr[ 4 + VERTEX_SIZE * 2 ] = vptr[ 4 + VERTEX_SIZE * 3 ] = v.r / 255.0f;
    vptr[ 5 ] = vptr[ 5 + VERTEX_SIZE ] = vptr[ 5 + VERTEX_SIZE * 2 ] = vptr[ 5 + VERTEX_SIZE * 3 ] = v.g / 255.0f;
    vptr[ 6 ] = vptr[ 6 + VERTEX_SIZE ] = vptr[ 6 + VERTEX_SIZE * 2 ] = vptr[ 6 + VERTEX_SIZE * 3 ] = v.b / 255.0f;
    vptr[ 7 ] = vptr[ 7 + VERTEX_SIZE ] = vptr[ 7 + VERTEX_SIZE * 2 ] = vptr[ 7 + VERTEX_SIZE * 3 ] = v.a / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicRectSetColorR( unsigned int id, float v )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 4 ] = vptr[ 4 + VERTEX_SIZE ] = vptr[ 4 + VERTEX_SIZE * 2 ] = vptr[ 4 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicRectSetColorG( unsigned int id, float v )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 5 ] = vptr[ 5 + VERTEX_SIZE ] = vptr[ 5 + VERTEX_SIZE * 2 ] = vptr[ 5 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicRectSetColorB( unsigned int id, float v )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 6 ] = vptr[ 6 + VERTEX_SIZE ] = vptr[ 6 + VERTEX_SIZE * 2 ] = vptr[ 6 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicRectSetColorA( unsigned int id, float v )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    vptr[ 7 ] = vptr[ 7 + VERTEX_SIZE ] = vptr[ 7 + VERTEX_SIZE * 2 ] = vptr[ 7 + VERTEX_SIZE * 3 ] = v / 255.0f;
    BufferVertices( vptr );
    ClearBufferBindings();
};

void NasrGraphicsTilemapSetX( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.tilemap.dest.x = v;
};

void NasrGraphicsTilemapSetY( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.tilemap.dest.y = v;
};

float NasrGraphicTextGetXOffset( unsigned int id )
{
    return NasrGraphicGet( id )->data.text.xoffset;
};

void NasrGraphicTextSetXOffset( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.text.xoffset = v;
};

void NasrGraphicTextAddToXOffset( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.text.xoffset += v;
};

float NasrGraphicTextGetYOffset( unsigned int id )
{
    return NasrGraphicGet( id )->data.text.yoffset;
};

void NasrGraphicTextSetYOffset( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.text.yoffset = v;
};

void NasrGraphicTextAddToYOffset( unsigned int id, float v )
{
    NasrGraphicGet( id )->data.text.yoffset += v;
};

void NasrGraphicTextSetCount( unsigned int id, int count )
{
    NasrGraphicText * t = &NasrGraphicGet( id )->data.text;
    t->count = NASR_MATH_MIN( count, t->capacity );
};

void NasrGraphicTextIncrementCount( unsigned int id )
{
    NasrGraphicText * t = &NasrGraphicGet( id )->data.text;
    t->count = NASR_MATH_MIN( t->count + 1, t->capacity );
};

int NasrLoadFileAsTexture( char * filename )
{
    return NasrLoadFileAsTextureEx( filename, NASR_SAMPLING_DEFAULT, NASR_INDEXED_DEFAULT );
};

int NasrLoadFileAsTextureEx( char * filename, int sampling, int indexed )
{
    const hash_t needle_hash = TextureMapHashString( filename );
    TextureMapEntry * entry = TextureMapHashFindEntry( filename, needle_hash );
    if ( entry->key.string != NULL )
    {
        return entry->value;
    }
    entry->key.string = ( char * )( malloc( strlen( filename ) + 1 ) );
    strcpy( entry->key.string, filename );
    entry->key.hash = needle_hash;
    entry->value = texture_count;

    unsigned int width;
    unsigned int height;
    unsigned char * data = LoadTextureFileData( filename, &width, &height, sampling, indexed );
    if ( !data )
    {
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
        NasrLog( "¡No mo’ space for textures!" );
        return -1;
    }

    AddTexture( &textures[ texture_count ], texture_ids[ texture_count ], data, width, height, sampling, indexed );
    return texture_count++;
};

static unsigned char * LoadTextureFileData( const char * filename, unsigned int * width, unsigned int * height, int sampling, int indexed )
{
    int channels;
    int w;
    int h;
    unsigned char * data = stbi_load( filename, &w, &h, &channels, STBI_rgb_alpha );
    if ( data == NULL || w < 0 || h < 0 )
    {
        printf( "Couldn’t load texture file.\n" );
        return 0;
    }
    *width = w;
    *height = h;
    return data;
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
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sample_type );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sample_type );
};

int NasrAddTextureBlank( unsigned int width, unsigned int height )
{
    return NasrAddTexture( 0, width, height );
};

int NasrAddTextureBlankEx( unsigned int width, unsigned int height, int sampling, int indexed )
{
    return NasrAddTextureEx( 0, width, height, sampling, indexed );
};

void NasrSetTextureAsTarget( int texture )
{
    glBindFramebuffer( GL_FRAMEBUFFER, framebuffer );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_ids[ texture ], 0 );
    glViewport( 0, 0, textures[ texture ].width, textures[ texture ].height );
    selected_texture = texture;
    BindBuffers( max_graphics );
};

void NasrReleaseTextureTarget()
{
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( magnified_canvas_x, magnified_canvas_y, magnified_canvas_width, magnified_canvas_height );
    selected_texture = -1;
    ClearBufferBindings();
};

void NasrGetTexturePixels( unsigned int texture, void * pixels )
{
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
    unsigned char pixels[ textures[ dest ].width * textures[ dest ].height * 4 ];
    NasrGetTexturePixels( dest, pixels );
    NasrApplyTextureToPixelData( src, pixels, srccoords, destcoords );
    glBindTexture( GL_TEXTURE_2D, texture_ids[ dest ] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, textures[ dest ].width, textures[ dest ].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
};

void NasrApplyTextureToPixelData( unsigned int texture, void * dest, NasrRectInt srccoords, NasrRectInt destcoords )
{
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

void NasrCopyPixelData( void * src, void * dest, NasrRectInt srccoords, NasrRectInt destcoords, int maxsrcw, int maxsrch )
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

void NasrTileTexture( unsigned int texture, void * pixels, NasrRectInt srccoords, NasrRectInt destcoords )
{
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
        0
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
            printf( "¡Invalid gradient direction for NasrDrawGradientRectToTexture! %d\n", dir );
        }
        break;
    }
    ResetVertices( GetVertices( max_graphics ) );
    SetVerticesColorValues( GetVertices( max_graphics ), &cbl, &cbr, &cur, &cul );
    rect.x *= canvas.w / textures[ selected_texture ].width;
    rect.y = ( textures[ selected_texture ].height - ( rect.y + rect.h ) ) * ( canvas.h / textures[ selected_texture ].height );
    rect.w *= canvas.w / textures[ selected_texture ].width;
    rect.h *= canvas.h / textures[ selected_texture ].height;
    DrawBox
    (
        vaos[ max_graphics ],
        &rect,
        0
    );
    SetupVertices( vaos[ max_graphics ] );
};

void NasrDrawSpriteToTexture( NasrGraphicSprite sprite )
{
    ResetVertices( GetVertices( max_graphics ) );
    sprite.dest.x *= canvas.w / textures[ selected_texture ].width;
    sprite.dest.y = ( textures[ selected_texture ].height - ( sprite.dest.y + sprite.dest.h ) ) * ( canvas.h / textures[ selected_texture ].height );
    sprite.dest.w *= canvas.w / textures[ selected_texture ].width;
    sprite.dest.h *= canvas.h / textures[ selected_texture ].height;

    glUseProgram( sprite_shader );

    UpdateSpriteVerticesValues( GetVertices( max_graphics ), &sprite );

    SetVerticesView( sprite.dest.x + ( sprite.dest.w / 2.0f ), sprite.dest.y + ( sprite.dest.h / 2.0f ), 0 );

    mat4 model = BASE_MATRIX;
    vec3 scale = { sprite.dest.w, sprite.dest.h, 0.0 };
    glm_scale( model, scale );
    vec3 xrot = { 0.0, 1.0, 0.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( sprite.rotation_x ), xrot );
    vec3 yrot = { 0.0, 0.0, 1.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( sprite.rotation_y ), yrot );
    vec3 zrot = { 1.0, 0.0, 0.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( sprite.rotation_z ), zrot );
    unsigned int model_location = glGetUniformLocation( sprite_shader, "model" );
    glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

    GLint opacity_location = glGetUniformLocation( sprite_shader, "opacity" );
    glUniform1f( opacity_location, ( float )( sprite.opacity ) );

    GLint texture_data_location = glGetUniformLocation( sprite_shader, "texture_data" );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture_ids[ sprite.texture ] );
    glUniform1i( texture_data_location, 0 );
    SetupVertices( vaos[ max_graphics ] );
};

int NasrHeld( int id )
{
    return GetHeld( id )[ 0 ];
};

void NasrRegisterInputs( const NasrInput * inputs, int num_o_inputs )
{
    // If this is not the 1st time using this function, make sure we reset e’erything to prevent memory leaks &
    // inaccurate #s.
    if ( keydata != NULL )
    {
        free( keydata );
    }
    max_inputs = 0;
    inputs_per_key = 0;
    keys_per_input = 0;

    // These several loops are necessary to find the max inputs per key & max keys per input.
    int keys_max_inputs[ GLFW_KEY_LAST ] = { 0 };
    for ( int i = 0; i < num_o_inputs; ++i )
    {
        if ( inputs[ i ].id + 1 > max_inputs )
        {
            max_inputs = inputs[ i ].id + 1;
        }
        ++keys_max_inputs[ inputs[ i ].key ];
    }
    int * inputs_max_keys = calloc( max_inputs, sizeof( int ) );
    for ( int i = 0; i < GLFW_KEY_LAST; ++i )
    {
        if ( keys_max_inputs[ i ] > inputs_per_key )
        {
            inputs_per_key = keys_max_inputs[ i ];
        }
    }
    for ( int i = 0; i < num_o_inputs; ++i )
    {
        ++inputs_max_keys[ inputs[ i ].id ];
    }
    for ( int i = 0; i < max_inputs; ++i )
    {
        if ( inputs_max_keys[ i ] > keys_per_input )
        {
            keys_per_input = inputs_max_keys[ i ];
        }
    }
    free( inputs_max_keys );

    // Calculate size o’ chunks.
    // 4 chunks are:
    // * List o’ inputs for each key ( list o’ lists )
    //       -> MAX_KEYS * ( 1 int for inner list count + inner list length )
    // * List o’ keys for each input ( list o’ lists )
    //       -> MAX_INPUTS * ( 1 int for inner list count + inner list length )
    // * List o’ keys held for each input ( list o’ lists )
    //       -> MAX_INPUTS * inner list length.
    //          We don’t need counts, as these are only e’er references from inputs & ne’er looped thru.
    // * List o’ inputs held ( 1D list )
    //       -> MAX_INPUTS
    //          Tho this value can be found thru the previous held_keys list, it’s mo’ efficient to only loop thru &
    //          check all keys held during press or release & set 1 simple value to be checked during the main game
    //          loop, 60 times per second.
    key_input_size = 1 + inputs_per_key;
    const int key_inputs_size = key_input_size * GLFW_KEY_LAST;
    input_keys_start = key_inputs_size;
    input_key_size = 1 + keys_per_input;
    const int input_keys_size = input_key_size * max_inputs;
    held_keys_start = input_keys_start + input_keys_size;
    const int held_keys_size = keys_per_input * max_inputs;
    held_start = held_keys_start + held_keys_size;
    const int held_size = max_inputs;
    const int total_size = key_inputs_size + input_keys_size + held_keys_size + held_size;
    keydata = calloc( total_size, sizeof( int ) );

    // Loop thru given list o’ key/input pairs & set KeyInputs & InputKeys,
    // incrementing their counts as we go.
    for ( int i = 0; i < num_o_inputs; ++i )
    {
        int * keyinputs = GetKeyInputs( inputs[ i ].key );
        int * keycount = &keyinputs[ 0 ];
        keyinputs[ 1 + keycount[ 0 ] ] = inputs[ i ].id;
        ++*keycount;

        int * inputkeys = GetInputKeys( inputs[ i ].id );
        int * inputcount = &inputkeys[ 0 ];
        inputkeys[ 1 + inputcount[ 0 ] ] = inputs[ i ].key;
        ++*inputcount;
    }
};

void NasrSetGlobalPalette( uint_fast8_t palette )
{
    global_palette = palette;
};

static void HandleInput( GLFWwindow * window, int key, int scancode, int action, int mods )
{
    switch ( action )
    {
        case ( GLFW_PRESS ):
        {
            // Loop thru this key’s inputs so we can set them as held.
            int * inputs = GetKeyInputs( key );
            const int count = inputs[ 0 ];
            for ( int i = 0; i < count; ++i )
            {
                const int input = inputs[ 1 + i ];
                GetHeld( input )[ 0 ] = 1;
                const int * input_keys = GetInputKeys( input );
                const int input_keys_count = input_keys[ 0 ];
                // While we can settle for just setting the 1D held list to 1, since if any key for an input is
                // pressed, the input is considered pressed, we need to also register the specific key held in
                // a list o’ keys for the input so we can check it later for release.
                for ( int j = 0; j < input_keys_count; ++j )
                {
                    if ( input_keys[ j + 1 ] == key )
                    {
                        GetHeldKeys( input )[ j ] = 1;
                    }
                }
            }
        }
        break;
        case ( GLFW_RELEASE ):
        {
            int * inputs = GetKeyInputs( key );
            const int count = inputs[ 0 ];
            for ( int i = 0; i < count; ++i )
            {
                const int input = inputs[ 1 + i ];
                // Unlike the press code, we need this for later, so keep pointer.
                int * held = GetHeld( input );
                *held = 0;
                const int * input_keys = GetInputKeys( input );
                const int input_keys_count = input_keys[ 0 ];
                for ( int j = 0; j < input_keys_count; ++j )
                {
                    // Unlike the press code, we need this for later, so keep pointer.
                    int * heldkeys = &GetHeldKeys( input )[ j ];
                    if ( input_keys[ j + 1 ] == key )
                    {
                        *heldkeys = 0;
                    }
                    // This is a simple & efficient way to say that if any o’ the keys for this input are held, still
                    // consider it held.
                    held[ 0 ] = held[ 0 ] || *heldkeys;
                }
            }
        }
        break;
    }
};

static int * GetKeyInputs( int key )
{
    return &keydata[ key_input_size * key ];
};

static int * GetInputKeys( int input )
{
    return &keydata[ input_keys_start + ( input_key_size * input ) ];
};

static int * GetHeldKeys( int input )
{
    return &keydata[ held_keys_start + ( keys_per_input * input ) ];
};

static int * GetHeld( int id )
{
    return &keydata[ held_start + id ];
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

static TextureMapEntry * TextureMapHashFindEntry( const char * needle_string, hash_t needle_hash )
{
    while ( 1 )
    {
        TextureMapEntry * entry = &texture_map[ needle_hash ];
        if ( entry->key.string == NULL || strcmp( entry->key.string, needle_string ) == 0 )
        {
            return entry;
        }
        needle_hash = ( needle_hash + 1 ) % max_textures;
    }
};

static uint32_t TextureMapHashString( const char * key )
{
    return NasrHashString( key, max_textures );
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

static uint32_t CharMapHashString( unsigned int id, const char * key )
{
    return NasrHashString( key, charmaps.list[ id ].hashmax );
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

static unsigned int GetStateLayerIndex( unsigned int state, unsigned int layer )
{
    return state * max_gfx_layers + layer;
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

static float * GetVertices( unsigned int id )
{
    return &vertices[ id * VERTEX_RECT_SIZE ];
};

static void ResetVertices( float * vptr )
{
    memcpy( vptr, &vertices_base, sizeof( vertices_base ) );
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

static void UpdateSpriteVertices( unsigned int id )
{
    BindBuffers( id );
    float * vptr = GetVertices( id );
    const NasrGraphicSprite * sprite = &graphics[ gfx_ptrs_id_to_pos[ id ] ].data.sprite;
    UpdateSpriteVerticesValues( vptr, sprite );
    ClearBufferBindings();
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

static void BindBuffers( unsigned int id )
{
    glBindVertexArray( vaos[ id ] );
    glBindBuffer( GL_ARRAY_BUFFER, vbos[ id ] );
};

static void ClearBufferBindings( void )
{
    glBindVertexArray( 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
};