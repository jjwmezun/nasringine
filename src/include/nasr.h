typedef struct NasrColor
{
    float r;
    float g;
    float b;
    float a;
} NasrColor;

void NasrColorPrint( const struct NasrColor * c );

typedef struct NasrRect
{
    float x;
    float y;
    float w;
    float h;
} NasrRect;

float NasrRectRight( const struct NasrRect * r );
float NasrRectBottom( const struct NasrRect * r );
int NasrRectEqual( const struct NasrRect * a, const struct NasrRect * b );
void NasrRectPrint( const struct NasrRect * r );

typedef enum NasrShaderType
{
    NASR_SHADER_VERTEX,
    NASR_SHADER_FRAGMENT
} NasrShaderType;

typedef struct NasrShader
{
    NasrShaderType type;
    const char * code;
} NasrShader;

typedef enum NasrGraphicType
{
    NASR_GRAPHIC_RECT
} NasrGraphicType;

typedef struct NasrGraphicRect
{
    NasrRect rect;
    NasrColor color;
} NasrGraphicRect;

typedef union NasrGraphicData
{
    NasrGraphicRect rect;
} NasrGraphicData;

typedef struct NasrGraphic
{
    NasrGraphicType type;
    NasrGraphicData data;
} NasrGraphic;

int NasrInit( const char * program_title, float canvas_width, float canvas_height, int max_graphics );
void NasrClose( void );
void NasrUpdate( void );

int NasrHasClosed( void );
void NasrLog( const char * message );

void NasrResetCamera( void );
void NasrAdjustCamera( struct NasrRect * target, float max_w, float max_h );
void NasrMoveCamera( float x, float y, float max_w, float max_h );

void NasrGraphicsAddCanvas( struct NasrColor color );
void NasrGraphicsAddRect(
    struct NasrRect rect,
    struct NasrColor color
);