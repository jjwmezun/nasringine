#include <stdio.h>
#include <cglm/cglm.h>
#include <cglm/call.h>
#include "nasr.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define DEGREES_TO_RADIANS( n ) ( ( n ) * 3.14159f / 180.0f )

#define VERTEX_SIZE 8

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

static float vertices[] =
{
    // Vertices     // Texture coords   // Color
     0.5f,  0.5f,   1.0f, 1.0f,         1.0f, 1.0f, 1.0f, 1.0f,// top right
     0.5f, -0.5f,   1.0f, 0.0f,         1.0f, 1.0f, 1.0f, 1.0f, // bottom right
    -0.5f, -0.5f,   0.0f, 0.0f,         1.0f, 1.0f, 1.0f, 1.0f, // bottom left
    -0.5f,  0.5f,   0.0f, 1.0f,         1.0f, 1.0f, 1.0f, 1.0f,  // top left 
};

static unsigned int indices[] =
{  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
};

#define MAX_ANIMATION_FRAME 2 * 3 * 4 * 5 * 6 * 7 * 8
#define NUMBER_O_BASE_SHADERS 4

typedef uint32_t hash_t;
typedef struct { char * string; hash_t hash; } TextureMapKey;
typedef struct { TextureMapKey key; unsigned int value; } TextureMapEntry;

static int magnification = 1;
static GLFWwindow * window;
static unsigned int VAO;
static unsigned int rect_shader;
static unsigned int sprite_shader;
static unsigned int indexed_sprite_shader;
static unsigned int tilemap_shader;
static unsigned int * base_shaders[ NUMBER_O_BASE_SHADERS ] = { &rect_shader, &sprite_shader, &indexed_sprite_shader, &tilemap_shader };
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
static unsigned int animation_timer = 0;

static void FramebufferSizeCallback( GLFWwindow * window, int width, int height );
static unsigned int GenerateShaderProgram( const NasrShader * shaders, int shadersnum );
static void BufferVertices();
static void DrawBox( const NasrRect * rect, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesColors( const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesView( float x, float y );
static void SetupVertices();
static void UpdateShaderOrtho( void );
static void HandleInput( GLFWwindow * window, int key, int scancode, int action, int mods );
static int * GetKeyInputs( int key );
static int * GetInputKeys( int input );
static int * GetHeldKeys( int input );
static int * GetHeld( int id );
static GLint GetGLSamplingType( int sampling );
static TextureMapEntry * hash_find_entry( const char * needle_string, hash_t needle_hash );
static uint32_t hash_string( const char * key );
static GLint GetGLRGBA( int indexed );
static unsigned char * LoadTextureFileData( const char * filename, unsigned int * width, unsigned int * height, int sampling, int indexed );
static void AddTexture( Texture * texture, unsigned int texture_id, const unsigned char * data, unsigned int width, unsigned int height, int sampling, int indexed );
static unsigned int GetStateLayerIndex( unsigned int state, unsigned int layer );

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
    int default_indexed_type
)
{
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

    // VBO
    unsigned int VBO;
    glGenBuffers( 1, &VBO );
    glBindBuffer( GL_ARRAY_BUFFER, VBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

    // VAO
    glGenVertexArrays( 1, &VAO );
    glBindVertexArray( VAO );

    // EBO
    unsigned int EBO;
    glGenBuffers( 1, &EBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

    // Bind buffers
    glBindBuffer( GL_ARRAY_BUFFER, VBO );
    BufferVertices();

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
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform float palette_id;\nuniform float opacity;\n\nvoid main()\n{\nvec4 index = texture( texture_data, texture_coords );\nfloat palette = palette_id / 256.0;\nfinal_color = texture( palette_data, vec2( index.r / 16.0, palette ) );\nfinal_color.a *= opacity;\n}"
        }
    };

    NasrShader tilemap_shaders[] =
    {
        vertex_shader,
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec2 texture_coords;\n\nuniform sampler2D texture_data;\nuniform sampler2D palette_data;\nuniform sampler2D map_data;\nuniform float map_width;\nuniform float map_height;\nuniform float tileset_width;\nuniform float tileset_height;\nuniform uint animation;\n  \nvoid main()\n{\n    vec4 tile = texture( map_data, texture_coords );\n    if ( tile.a > 0.0 && tile.a < 1.0 )\n    {\n        float frames = floor( tile.a * 255.0 );\n        float frame = mod( float( animation ), frames );\n        // I don’t know why mod sometimes doesn’t work right & still sometimes says 6 is the mod o’ 6 / 6 ’stead o’ 0;\n        // This fixes it.\n        while ( frame >= frames )\n        {\n            frame -= frames;\n        }\n        tile.x += frame / 255.0;\n    }\n    float xrel = mod( texture_coords.x * 256.0, ( 256.0 / map_width ) ) / ( 4096.0 / map_width );\n    float yrel = mod( texture_coords.y * 256.0, ( 256.0 / map_height ) ) / ( 4096.0 / map_height );\n    float xoffset = tile.x * 255.0 * ( 16 / tileset_width );\n    float yoffset = tile.y * 255.0 * ( 16 / tileset_height );\n    float palette = tile.z;\n    vec4 index = texture( texture_data, vec2( xoffset + ( xrel / ( tileset_width / 256.0 ) ), yoffset + ( yrel / ( tileset_height / 256.0 ) ) ) );\n    final_color = ( tile.a < 1.0 ) ? texture( palette_data, vec2( index.r / 16.0, palette ) ) : vec4( 0.0, 0.0, 0.0, 0.0 );\n}"
        }
    };
    
    rect_shader = GenerateShaderProgram( rect_shaders, 2 );
    sprite_shader = GenerateShaderProgram( sprite_shaders, 2 );
    indexed_sprite_shader = GenerateShaderProgram( indexed_sprite_shaders, 2 );
    tilemap_shader = GenerateShaderProgram( tilemap_shaders, 2 );

    // Init camera
    NasrResetCamera();
    UpdateShaderOrtho();

    // Init graphics list
    max_graphics = init_max_graphics;
    max_states = init_max_states;
    max_gfx_layers = init_max_gfx_layers;
    graphics = calloc( max_graphics, sizeof( NasrGraphic ) );
    gfx_ptrs_id_to_pos = calloc( max_graphics, sizeof( int ) );
    gfx_ptrs_pos_to_id = calloc( max_graphics, sizeof( int ) );
    state_for_gfx = calloc( max_graphics, sizeof( int ) );
    layer_for_gfx = calloc( max_graphics, sizeof( int ) );
    layer_pos = calloc( max_states * max_gfx_layers, sizeof( int ) );

    // Init framebuffer.
    glGenFramebuffers( 1, &framebuffer );

    magnified_canvas_width = canvas.w * magnification;
    magnified_canvas_height = canvas.h * magnification;
    magnified_canvas_x = ( int )( floor( ( double )( magnified_canvas_width - magnified_canvas_width ) / 2.0 ) );
    magnified_canvas_y = ( int )( floor( ( double )( magnified_canvas_height - magnified_canvas_height ) / 2.0 ) );
    glViewport( magnified_canvas_x, magnified_canvas_y, magnified_canvas_width, magnified_canvas_height );

    // Init texture map.
    texture_map = calloc( max_textures, sizeof( TextureMapEntry ) );

    return 0;
};

void NasrSetPalette( const char * filename )
{
    unsigned int width;
    unsigned int height;
    const unsigned char * data = LoadTextureFileData( filename, &width, &height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
    AddTexture( &palette_texture, palette_texture_id, data, width, height, NASR_SAMPLING_NEAREST, NASR_INDEXED_NO );
};

void NasrClose( void )
{
    glDeleteFramebuffers( 1, &framebuffer );
    NasrClearTextures();
    glDeleteTextures( max_textures, texture_ids );
    free( texture_map );
    free( keydata );
    if ( textures != NULL )
    {
        free( textures );
    }
    if ( texture_ids != NULL )
    {
        free( texture_ids );
    }
    if ( graphics != NULL )
    {
        free( graphics );
    }
    if ( gfx_ptrs_id_to_pos != NULL )
    {
        free( gfx_ptrs_id_to_pos );
    }
    if ( gfx_ptrs_pos_to_id != NULL )
    {
        free( gfx_ptrs_pos_to_id );
    }
    if ( layer_pos != NULL )
    {
        free( layer_pos );
    }
    if ( state_for_gfx != NULL )
    {
        free( state_for_gfx );
    }
    if ( layer_for_gfx != NULL )
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

void NasrUpdate( void )
{
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    
    if ( camera.x != prev_camera.x || camera.y != prev_camera.y )
    {
        UpdateShaderOrtho();
    }

    for ( int i = 0; i < num_o_graphics; ++i )
    {
        switch ( graphics[ i ].type )
        {
            case ( NASR_GRAPHIC_RECT ):
            {
                DrawBox
                (
                    &graphics[ i ].data.rect.rect,
                    &graphics[ i ].data.rect.color,
                    &graphics[ i ].data.rect.color,
                    &graphics[ i ].data.rect.color,
                    &graphics[ i ].data.rect.color
                );
            }
            break;
            case ( NASR_GRAPHIC_RECT_GRADIENT ):
            {
                DrawBox
                (
                    &graphics[ i ].data.gradient.rect,
                    &graphics[ i ].data.gradient.color1,
                    &graphics[ i ].data.gradient.color2,
                    &graphics[ i ].data.gradient.color3,
                    &graphics[ i ].data.gradient.color4
                );
            }
            break;
            case ( NASR_GRAPHIC_SPRITE ):
            {
                const NasrGraphicSprite * sprite = &graphics[ i ].data.sprite;
                unsigned int texture_id = sprite->texture;
                const unsigned int shader = textures[ texture_id ].indexed ? indexed_sprite_shader : sprite_shader;
                const NasrRect * src = &sprite->src;
                const NasrRect * dest = &sprite->dest;
                glUseProgram( shader );

                // Src Coords
                if ( sprite->flip_x )
                {
                    vertices[ 2 ] = vertices[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].width ) * src->x; // Left X
                    vertices[ 2 + VERTEX_SIZE * 3 ] = vertices[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture_id ].width ) * ( src->x + src->w );  // Right X
                }
                else
                {
                    vertices[ 2 + VERTEX_SIZE * 3 ] = vertices[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture_id ].width ) * src->x; // Left X
                    vertices[ 2 ] = vertices[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].width ) * ( src->x + src->w );  // Right X
                }

                if ( sprite->flip_y )
                {
                    vertices[ 3 + VERTEX_SIZE * 2 ] = vertices[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].height ) * ( src->y + src->h ); // Top Y
                    vertices[ 3 + VERTEX_SIZE * 3 ] = vertices[ 3 ] = 1.0f / ( float )( textures[ texture_id ].height ) * src->y;  // Bottom Y
                }
                else
                {
                    vertices[ 3 + VERTEX_SIZE * 3 ] = vertices[ 3 ] = 1.0f / ( float )( textures[ texture_id ].height ) * ( src->y + src->h ); // Top Y
                    vertices[ 3 + VERTEX_SIZE * 2 ] = vertices[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture_id ].height ) * src->y;  // Bottom Y
                }

                BufferVertices();
                SetVerticesView( dest->x + ( dest->w / 2.0f ), dest->y + ( dest->h / 2.0f ) );

                mat4 model = BASE_MATRIX;
                vec3 scale = { dest->w, dest->h, 0.0 };
                glm_scale( model, scale );
                vec3 xrot = { 0.0, 1.0, 0.0 };
                glm_rotate( model, DEGREES_TO_RADIANS( sprite->rotation_x ), xrot );
                vec3 yrot = { 0.0, 0.0, 1.0 };
                glm_rotate( model, DEGREES_TO_RADIANS( sprite->rotation_y ), yrot );
                vec3 zrot = { 1.0, 0.0, 0.0 };
                glm_rotate( model, DEGREES_TO_RADIANS( sprite->rotation_z ), zrot );
                unsigned int model_location = glGetUniformLocation( shader, "model" );
                glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

                if ( textures[ texture_id ].indexed )
                {
                    GLint palette_id_location = glGetUniformLocation( shader, "palette_id" );
                    glUniform1f( palette_id_location, ( float )( sprite->palette ) );
                }

                GLint opacity_location = glGetUniformLocation( shader, "opacity" );
                glUniform1f( opacity_location, ( float )( sprite->opacity ) );

                GLint texture_data_location = glGetUniformLocation( shader, "texture_data" );
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, texture_ids[ texture_id ] );
                glUniform1i( texture_data_location, 0 );
                if ( textures[ texture_id ].indexed )
                {
                    GLint palette_data_location = glGetUniformLocation(shader, "palette_data");
                    glActiveTexture(GL_TEXTURE1 );
                    glBindTexture(GL_TEXTURE_2D, palette_texture_id );
                    glUniform1i(palette_data_location, 1);
                }
                SetupVertices();
            }
            break;
            case ( NASR_GRAPHIC_TILEMAP ):
            {
                glUseProgram( tilemap_shader );

                #define TG graphics[ i ].data.tilemap

                vertices[ 2 + VERTEX_SIZE * 3 ] = vertices[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ TG.tilemap ].width ) * TG.src.x; // Left X
                vertices[ 2 ] = vertices[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ TG.tilemap ].width ) * ( TG.src.x + TG.src.w );  // Right X
                vertices[ 3 + VERTEX_SIZE * 3 ] = vertices[ 3 ] = 1.0f / ( float )( textures[ TG.tilemap ].height ) * ( TG.src.y + TG.src.h ); // Top Y
                vertices[ 3 + VERTEX_SIZE * 2 ] = vertices[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ TG.tilemap ].height ) * TG.src.y;  // Bottom Y

                BufferVertices();
                SetVerticesView( TG.dest.x + ( TG.dest.w / 2.0f ), TG.dest.y + ( TG.dest.h / 2.0f ) );

                mat4 model = BASE_MATRIX;
                vec3 scale = { TG.dest.w, TG.dest.h, 0.0 };
                glm_scale( model, scale );
                unsigned int model_location = glGetUniformLocation( tilemap_shader, "model" );
                glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

                GLint map_width_location = glGetUniformLocation( tilemap_shader, "map_width" );
                glUniform1f( map_width_location, ( float )( textures[ TG.tilemap ].width ) );

                GLint map_height_location = glGetUniformLocation( tilemap_shader, "map_height" );
                glUniform1f( map_height_location, ( float )( textures[ TG.tilemap ].height ) );

                GLint tileset_width_location = glGetUniformLocation( tilemap_shader, "tileset_width" );
                glUniform1f( tileset_width_location, ( float )( textures[ TG.texture ].width ) );

                GLint tileset_height_location = glGetUniformLocation( tilemap_shader, "tileset_height" );
                glUniform1f( tileset_height_location, ( float )( textures[ TG.texture ].height ) );

                GLint animation_location = glGetUniformLocation( tilemap_shader, "animation" );
                glUniform1ui( animation_location, animation_frame );

                GLint texture_data_location = glGetUniformLocation(tilemap_shader, "texture_data");
                GLint palette_data_location = glGetUniformLocation(tilemap_shader, "palette_data");
                GLint map_data_location = glGetUniformLocation(tilemap_shader, "map_data");
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_ids[ TG.texture ] );
                glUniform1i(texture_data_location, 0);
                glActiveTexture(GL_TEXTURE1 );
                glBindTexture(GL_TEXTURE_2D, palette_texture_id );
                glUniform1i(palette_data_location, 1);
                glActiveTexture(GL_TEXTURE2 );
                glBindTexture(GL_TEXTURE_2D, texture_ids[ TG.tilemap ] );
                glUniform1i(map_data_location, 2);
                SetupVertices();

                #undef TG
            }
            break;
            default:
            {
                printf( "¡Trying to render invalid graphic type #%d!\n", graphics[ i ].type );
            }
            break;
        }
    }

    glfwSwapBuffers( window );
    glfwPollEvents();
    prev_camera = camera;

    if ( animation_timer == 7 )
    {
        animation_timer = 0;
        ++animation_frame;
        if ( animation_frame == MAX_ANIMATION_FRAME )
        {
            animation_frame = 0;
        }
    }
    else
    {
        ++animation_timer;
    }
};

int NasrHasClosed( void )
{
    return glfwWindowShouldClose( window );
};

void NasrLog( const char * message )
{
    printf( "%s\n", message );
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

static void BufferVertices( void )
{
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof( float ), 0 );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof( float ), ( void * )( 2 * sizeof( float ) ) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof( float ), ( void * )( 4 * sizeof( float ) ) );
    glEnableVertexAttribArray( 2 );
};

static void DrawBox( const NasrRect * rect, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color )
{
    glUseProgram( rect_shader );
    SetVerticesColors( top_left_color, top_right_color, bottom_left_color, bottom_right_color );
    BufferVertices();
    SetVerticesView( rect->x + ( rect->w / 2.0f ), rect->y + ( rect->h / 2.0f ) );
    mat4 model = BASE_MATRIX;
    vec3 scale = { rect->w, rect->h, 0.0 };
    glm_scale( model, scale );
    unsigned int model_location = glGetUniformLocation( rect_shader, "model" );
    glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );
    SetupVertices();
};

static void SetVerticesColors( const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color )
{
    vertices[ 4 ] = bottom_right_color->r / 255.0f;
    vertices[ 5 ] = bottom_right_color->g / 255.0f;
    vertices[ 6 ] = bottom_right_color->b / 255.0f;
    vertices[ 7 ] = bottom_right_color->a / 255.0f;

    vertices[ 4 + VERTEX_SIZE ] = top_right_color->r / 255.0f;
    vertices[ 5 + VERTEX_SIZE ] = top_right_color->g / 255.0f;
    vertices[ 6 + VERTEX_SIZE ] = top_right_color->b / 255.0f;
    vertices[ 7 + VERTEX_SIZE ] = top_right_color->a / 255.0f;

    vertices[ 4 + VERTEX_SIZE * 2 ] = top_left_color->r / 255.0f;
    vertices[ 5 + VERTEX_SIZE * 2 ] = top_left_color->g / 255.0f;
    vertices[ 6 + VERTEX_SIZE * 2 ] = top_left_color->b / 255.0f;
    vertices[ 7 + VERTEX_SIZE * 2 ] = top_left_color->a / 255.0f;

    vertices[ 4 + VERTEX_SIZE * 3 ] = bottom_left_color->r / 255.0f;
    vertices[ 5 + VERTEX_SIZE * 3 ] = bottom_left_color->g / 255.0f;
    vertices[ 6 + VERTEX_SIZE * 3 ] = bottom_left_color->b / 255.0f;
    vertices[ 7 + VERTEX_SIZE * 3 ] = bottom_left_color->a / 255.0f;
};

static void SetVerticesView( float x, float y )
{
    mat4 view = BASE_MATRIX;
    vec3 trans = { x, y, 0.0f };
    glm_translate( view, trans );
    unsigned int view_location = glGetUniformLocation( rect_shader, "view" );
    glUniformMatrix4fv( view_location, 1, GL_FALSE, ( float * )( view ) );
};

static void SetupVertices( void )
{
    glBindVertexArray( VAO );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
    glBindVertexArray( 0 );
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
        for ( unsigned int i = gfx_ptrs_id_to_pos[ id ]; i < target_layer_pos; ++i )
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
    int abs,
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
    for ( unsigned int i = num_o_graphics; i > p; --i )
    {
        graphics[ i ] = graphics[ i - 1 ];

        // Update pointers so they still point to correct graphics.
        const unsigned int t = gfx_ptrs_pos_to_id[ i - 1 ];
        ++gfx_ptrs_id_to_pos[ t ];
        gfx_ptrs_pos_to_id[ gfx_ptrs_id_to_pos[ t ] ] = t;
    }

    // Add current graphic & pointer.
    graphics[ p ] = graphic;
    gfx_ptrs_id_to_pos[ num_o_graphics ] = p;
    gfx_ptrs_pos_to_id[ p ] = num_o_graphics;
    state_for_gfx[ num_o_graphics ] = state;
    layer_for_gfx[ num_o_graphics ] = layer;

    return num_o_graphics++;
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
    graphic.type = NASR_GRAPHIC_RECT;
    graphic.data.rect.rect = rect;
    graphic.data.rect.color = color;
    return NasrGraphicsAdd( abs, state, layer, graphic );
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
    return NasrGraphicsAdd( abs, state, layer, graphic );
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
    unsigned char palette
)
{
    if ( num_o_graphics >= max_graphics )
    {
        return -1;
    }
    struct NasrGraphic graphic;
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
    return NasrGraphicsAdd( abs, state, layer, graphic );
};

int NasrGraphicsAddTilemap
(
    int abs,
    unsigned int state,
    unsigned int layer,
    unsigned int texture,
    const NasrTile * tiles,
    unsigned int w,
    unsigned int h
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
    return NasrGraphicsAdd( abs, state, layer, graphic );
}

int NasrLoadFileAsTexture( char * filename )
{
    return NasrLoadFileAsTextureEx( filename, NASR_SAMPLING_DEFAULT, NASR_INDEXED_DEFAULT );
};

int NasrLoadFileAsTextureEx( char * filename, int sampling, int indexed )
{
    const hash_t needle_hash = hash_string( filename );
    TextureMapEntry * entry = hash_find_entry( filename, needle_hash );
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
};

void NasrReleaseTextureTarget()
{
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glViewport( magnified_canvas_x, magnified_canvas_y, magnified_canvas_width, magnified_canvas_height );
    selected_texture = -1;
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
    rect.x *= canvas.w / textures[ selected_texture ].width;
    rect.y = ( textures[ selected_texture ].height - ( rect.y + rect.h ) ) * ( canvas.h / textures[ selected_texture ].height );
    rect.w *= canvas.w / textures[ selected_texture ].width;
    rect.h *= canvas.h / textures[ selected_texture ].height;
    DrawBox
    (
        &rect,
        &color,
        &color,
        &color,
        &color
    );
};

void NasrDrawGradientRectToTexture( NasrRect rect, int dir, NasrColor color1, NasrColor color2 )
{
    rect.x *= canvas.w / textures[ selected_texture ].width;
    rect.y = ( textures[ selected_texture ].height - ( rect.y + rect.h ) ) * ( canvas.h / textures[ selected_texture ].height );
    rect.w *= canvas.w / textures[ selected_texture ].width;
    rect.h *= canvas.h / textures[ selected_texture ].height;
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
    DrawBox
    (
        &rect,
        &cbl,
        &cbr,
        &cur,
        &cul
    );
};

void NasrDrawSpriteToTexture(
    int texture,
    NasrRect src,
    NasrRect dest,
    int flip_x,
    int flip_y,
    float rotation_x,
    float rotation_y,
    float rotation_z,
    float opacity
)
{
    dest.x *= canvas.w / textures[ selected_texture ].width;
    dest.y = ( textures[ selected_texture ].height - ( dest.y + dest.h ) ) * ( canvas.h / textures[ selected_texture ].height );
    dest.w *= canvas.w / textures[ selected_texture ].width;
    dest.h *= canvas.h / textures[ selected_texture ].height;

    glUseProgram( sprite_shader );

    // Src Coords
    if ( flip_x )
    {
        vertices[ 2 ] = vertices[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture ].width ) * src.x; // Left X
        vertices[ 2 + VERTEX_SIZE * 3 ] = vertices[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture ].width ) * ( src.x + src.w );  // Right X
    }
    else
    {
        vertices[ 2 + VERTEX_SIZE * 3 ] = vertices[ 2 + VERTEX_SIZE * 2 ] = 1.0f / ( float )( textures[ texture ].width ) * src.x; // Left X
        vertices[ 2 ] = vertices[ 2 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture ].width ) * ( src.x + src.w );  // Right X
    }

    if ( flip_y )
    {
        vertices[ 3 + VERTEX_SIZE * 3 ] = vertices[ 3 ] = 1.0f / ( float )( textures[ texture ].height ) * ( src.y + src.h ); // Top Y
        vertices[ 3 + VERTEX_SIZE * 2 ] = vertices[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture ].height ) * src.y;  // Bottom Y
    }
    else
    {
        vertices[ 3 + VERTEX_SIZE * 2 ] = vertices[ 3 + VERTEX_SIZE ] = 1.0f / ( float )( textures[ texture ].height ) * ( src.y + src.h ); // Top Y
        vertices[ 3 + VERTEX_SIZE * 3 ] = vertices[ 3 ] = 1.0f / ( float )( textures[ texture ].height ) * src.y;  // Bottom Y
    }

    BufferVertices();
    SetVerticesView( dest.x + ( dest.w / 2.0f ), dest.y + ( dest.h / 2.0f ) );

    mat4 model = BASE_MATRIX;
    vec3 scale = { dest.w, dest.h, 0.0 };
    glm_scale( model, scale );
    vec3 xrot = { 0.0, 1.0, 0.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( rotation_x ), xrot );
    vec3 yrot = { 0.0, 0.0, 1.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( rotation_y ), yrot );
    vec3 zrot = { 1.0, 0.0, 0.0 };
    glm_rotate( model, DEGREES_TO_RADIANS( rotation_z ), zrot );
    unsigned int model_location = glGetUniformLocation( sprite_shader, "model" );
    glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

    GLint opacity_location = glGetUniformLocation( sprite_shader, "opacity" );
    glUniform1f( opacity_location, ( float )( opacity ) );

    GLint texture_data_location = glGetUniformLocation( sprite_shader, "texture_data" );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture_ids[ texture ] );
    glUniform1i( texture_data_location, 0 );
    SetupVertices();
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

static TextureMapEntry * hash_find_entry( const char * needle_string, hash_t needle_hash )
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

static uint32_t hash_string( const char * key )
{
    uint32_t hash = 2166136261u;
    const int length = strlen( key );
    for ( int i = 0; i < length; i++ )
    {
        hash ^= ( uint8_t )( key[ i ] );
        hash *= 16777619;
    }
    return hash % max_textures;
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