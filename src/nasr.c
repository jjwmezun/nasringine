#include <stdio.h>
#include <cglm/cglm.h>
#include <cglm/call.h>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "nasr.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define VERTEX_SIZE 8

#define BASE_MATRIX {\
    { 1.0f, 0.0f, 0.0f, 0.0f },\
    { 0.0f, 1.0f, 0.0f, 0.0f },\
    { 0.0f, 0.0f, 1.0f, 0.0f },\
    { 0.0f, 0.0f, 0.0f, 1.0f }\
}

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

static int magnification = 1;
static GLFWwindow * window;
static unsigned int VAO;
static unsigned int rect_shader;
static NasrGraphic * graphics = NULL;
static int max_graphics = 0;
static int num_o_graphics = 0;
static NasrRect camera = { 0.0f, 0.0f, 0.0f, 0.0f };
static NasrRect prev_camera = { 0.0f, 0.0f, 0.0f, 0.0f };
static NasrRect canvas = { 0.0f, 0.0f, 0.0f, 0.0f };

static void FramebufferSizeCallback( GLFWwindow * window, int width, int height );
static unsigned int GenerateShaderProgram( const NasrShader * shaders, int shadersnum );
static void BufferVertices();
static void DrawBox( const NasrRect * rect, const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesColors( const NasrColor * top_left_color, const NasrColor * top_right_color, const NasrColor * bottom_left_color, const NasrColor * bottom_right_color );
static void SetVerticesView( float x, float y );
static void SetupVertices();
static void UpdateShaderOrtho( void );

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

int NasrInit( const char * program_title, float canvas_width, float canvas_height, int init_max_graphics )
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

    // Turn on blending.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

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
    NasrShader shaders[] =
    {
        {
            NASR_SHADER_VERTEX,
            "#version 330 core\n layout ( location = 0 ) in vec2 in_position;\n layout ( location = 1 ) in vec2 in_texture_coords;\n layout ( location = 2 ) in vec4 in_color;\n \n out vec2 texture_coords;\n out vec4 out_color;\n out vec2 out_position;\n \n uniform mat4 model;\n uniform mat4 view;\n uniform mat4 ortho;\n \n void main()\n {\n out_position = in_position;\n gl_Position = ortho * view * model * vec4( in_position, 0.0, 1.0 );\n texture_coords = in_texture_coords;\n out_color = in_color;\n }"
        },
        {
            NASR_SHADER_FRAGMENT,
            "#version 330 core\nout vec4 final_color;\n\nin vec4 out_color;\nin vec2 out_position;\n\nvoid main()\n{\n    final_color = out_color;\n}"
        }
    };
    rect_shader = GenerateShaderProgram( shaders, 2 );

    // Init camera
    NasrResetCamera();
    UpdateShaderOrtho();

    max_graphics = init_max_graphics;
    graphics = calloc( max_graphics, sizeof( NasrGraphic ) );

    return 0;
};

void NasrClose( void )
{
    free( graphics );
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
    glUseProgram( rect_shader );
    mat4 ortho =
    {
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f }
    };
    glm_ortho_rh_no( camera.x, camera.w, camera.h, camera.y, -1.0f, 1.0f, ortho );
    unsigned int ortho_location = glGetUniformLocation( rect_shader, "ortho" );
    glUniformMatrix4fv( ortho_location, 1, GL_FALSE, ( float * )( ortho ) );
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

void NasrGraphicsAddCanvas( NasrColor color )
{
    NasrGraphicsAddRect(
        canvas,
        color
    );
};

void NasrGraphicsAddRect(
    NasrRect rect,
    NasrColor color
)
{
    if ( num_o_graphics < max_graphics )
    {
        graphics[ num_o_graphics ].type = NASR_GRAPHIC_RECT;
        graphics[ num_o_graphics ].data.rect.rect = rect;
        graphics[ num_o_graphics ].data.rect.color = color;
        ++num_o_graphics;
    }
};