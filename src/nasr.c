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

#define NUMBER_O_BASE_SHADERS 2

static int magnification = 1;
static GLFWwindow * window;
static unsigned int VAO;
static unsigned int rect_shader;
static unsigned int sprite_shader;
static unsigned int * base_shaders[ NUMBER_O_BASE_SHADERS ] = { &rect_shader, &sprite_shader };
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
    int init_max_graphics,
    int init_max_textures
)
{
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
    
    rect_shader = GenerateShaderProgram( rect_shaders, 2 );
    sprite_shader = GenerateShaderProgram( sprite_shaders, 2 );

    // Init camera
    NasrResetCamera();
    UpdateShaderOrtho();

    // Init graphics list
    max_graphics = init_max_graphics;
    graphics = calloc( max_graphics, sizeof( NasrGraphic ) );

    return 0;
};

void NasrClose( void )
{
    glDeleteTextures( max_textures, texture_ids );
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
    glfwTerminate();
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
                const NasrRect * src = &sprite->src;
                const NasrRect * dest = &sprite->dest;
                glUseProgram( sprite_shader );

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
                unsigned int model_location = glGetUniformLocation( sprite_shader, "model" );
                glUniformMatrix4fv( model_location, 1, GL_FALSE, ( float * )( model ) );

                GLint opacity_location = glGetUniformLocation( sprite_shader, "opacity" );
                glUniform1f( opacity_location, ( float )( sprite->opacity ) );

                GLint texture_data_location = glGetUniformLocation( sprite_shader, "texture_data" );
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, texture_ids[ texture_id ] );
                glUniform1i( texture_data_location, 0 );
                SetupVertices();
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

    GLint magnified_canvas_width = canvas.w * magnification;
    GLint magnified_canvas_height = canvas.h * magnification;
    GLint x = ( int )( floor( ( double )( screen_width - magnified_canvas_width ) / 2.0 ) );
    GLint y = ( int )( floor( ( double )( screen_height - magnified_canvas_height ) / 2.0 ) );
    glViewport( x, y, magnified_canvas_width, magnified_canvas_height );
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

NasrGraphic * NasrGraphicGet( int id )
{
    return &graphics[ id ];
};

int NasrGraphicsAddCanvas( NasrColor color )
{
    return NasrGraphicsAddRect(
        canvas,
        color
    );
};

int NasrGraphicsAddRect(
    NasrRect rect,
    NasrColor color
)
{
    if ( num_o_graphics < max_graphics )
    {
        graphics[ num_o_graphics ].type = NASR_GRAPHIC_RECT;
        graphics[ num_o_graphics ].data.rect.rect = rect;
        graphics[ num_o_graphics ].data.rect.color = color;
        return num_o_graphics++;
    }
    else
    {
        return -1;
    }
};

int NasrGraphicsAddRectGradient(
    struct NasrRect rect,
    int dir,
    struct NasrColor color1,
    struct NasrColor color2
)
{
    if ( num_o_graphics < max_graphics )
    {
        graphics[ num_o_graphics ].type = NASR_GRAPHIC_RECT_GRADIENT;
        graphics[ num_o_graphics ].data.gradient.rect = rect;
        switch ( dir )
        {
            case ( NASR_DIR_UP ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color2;
                graphics[ num_o_graphics ].data.gradient.color2 = color2;
                graphics[ num_o_graphics ].data.gradient.color3 = color1;
                graphics[ num_o_graphics ].data.gradient.color4 = color1;
            }
            break;
            case ( NASR_DIR_UPRIGHT ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color1;
                graphics[ num_o_graphics ].data.gradient.color2 = color2;
                graphics[ num_o_graphics ].data.gradient.color3 = color1;
                graphics[ num_o_graphics ].data.gradient.color4 = color1;
            }
            break;
            case ( NASR_DIR_RIGHT ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color1;
                graphics[ num_o_graphics ].data.gradient.color2 = color2;
                graphics[ num_o_graphics ].data.gradient.color3 = color1;
                graphics[ num_o_graphics ].data.gradient.color4 = color2;
            }
            break;
            case ( NASR_DIR_DOWNRIGHT ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color1;
                graphics[ num_o_graphics ].data.gradient.color2 = color1;
                graphics[ num_o_graphics ].data.gradient.color3 = color2;
                graphics[ num_o_graphics ].data.gradient.color4 = color1;
            }
            break;
            case ( NASR_DIR_DOWN ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color1;
                graphics[ num_o_graphics ].data.gradient.color2 = color1;
                graphics[ num_o_graphics ].data.gradient.color3 = color2;
                graphics[ num_o_graphics ].data.gradient.color4 = color2;
            }
            break;
            case ( NASR_DIR_DOWNLEFT ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color1;
                graphics[ num_o_graphics ].data.gradient.color2 = color1;
                graphics[ num_o_graphics ].data.gradient.color3 = color2;
                graphics[ num_o_graphics ].data.gradient.color4 = color1;
            }
            break;
            case ( NASR_DIR_LEFT ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color2;
                graphics[ num_o_graphics ].data.gradient.color2 = color1;
                graphics[ num_o_graphics ].data.gradient.color3 = color2;
                graphics[ num_o_graphics ].data.gradient.color4 = color1;
            }
            break;
            case ( NASR_DIR_UPLEFT ):
            {
                graphics[ num_o_graphics ].data.gradient.color1 = color2;
                graphics[ num_o_graphics ].data.gradient.color2 = color1;
                graphics[ num_o_graphics ].data.gradient.color3 = color1;
                graphics[ num_o_graphics ].data.gradient.color4 = color1;
            }
            break;
        }
        return num_o_graphics++;
    }
    else
    {
        return -1;
    }
};

int NasrGraphicsAddSprite
(
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
    if ( num_o_graphics < max_graphics )
    {
        graphics[ num_o_graphics ].type = NASR_GRAPHIC_SPRITE;
        graphics[ num_o_graphics ].data.sprite.texture = texture;
        graphics[ num_o_graphics ].data.sprite.src = src;
        graphics[ num_o_graphics ].data.sprite.dest = dest;
        graphics[ num_o_graphics ].data.sprite.flip_x = flip_x;
        graphics[ num_o_graphics ].data.sprite.flip_y = flip_y;
        graphics[ num_o_graphics ].data.sprite.rotation_x = rotation_x;
        graphics[ num_o_graphics ].data.sprite.rotation_y = rotation_y;
        graphics[ num_o_graphics ].data.sprite.rotation_z = rotation_z;
        graphics[ num_o_graphics ].data.sprite.opacity = opacity;
        return num_o_graphics++;
    }
    else
    {
        return -1;
    }
};

int NasrAddTexture( unsigned char * data, unsigned int width, unsigned int height )
{
    if ( texture_count >= max_textures )
    {
        NasrLog( "¡No mo’ space for textures!" );
        return -1;
    }

    textures[ texture_count ].width = width;
    textures[ texture_count ].height = height;
    glBindTexture( GL_TEXTURE_2D, texture_ids[ texture_count ] );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    return texture_count++;
};

void NasrClearTextures( void )
{
    texture_count = 0;
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