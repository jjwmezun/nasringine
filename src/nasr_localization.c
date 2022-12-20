#include "json/json.h"
#include "nasr_io.h"
#include "nasr_localization.h"
#include "nasr_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NasrTranslation
{
    char * original;
    char * translation;
    char * context;
    char * translation_plural;
} NasrTranslation;

typedef struct NasrTranslationContext
{
    NasrHashKey key;
    NasrTranslation value;
} NasrTranslationContext;

typedef struct NasrTranslationEntry
{
    NasrHashKey key;
    size_t capacity;
    NasrTranslationContext * contexts;
} NasrTranslationEntry;

static NasrTranslationEntry * translations = 0;
static size_t capacity = 0;

static NasrTranslationEntry * NasrTranslationFindEntry( const char * needle_string, hash_t needle_hash );
static NasrTranslationEntry * NasrTranslationGenEntry( const char * key );
static NasrTranslationContext * NasrTranslationFindContext( NasrTranslationEntry * entry, const char * needle_string, hash_t needle_hash );
static NasrTranslationContext * NasrTranslationGenContext( NasrTranslationEntry * entry, const char * key );
static char * GenString( const char * in );

int NasrSetLanguage( const char * filename, const char * domain )
{
    char * text = NasrReadFile( filename );
    if ( !text )
    {
        return -1;
    }
    json_char * json = ( json_char * )( text );
    json_value * root = json_parse( json, strlen( text ) + 1 );
    free( text );
    if ( !root || root->type != json_object )
    {
        NasrLog( "¡Localization file failed to load!\n" );
        return -1;
    }

    for ( unsigned int i = 0; i < root->u.object.length; ++i )
    {
        const json_object_entry root_entry = root->u.object.values[ i ];
        if ( strcmp( "localization", root_entry.name ) == 0 )
        {
            if ( root_entry.value->type != json_array )
            {
                NasrLog( "Localization file malformed: localization object is not an array." );
                return -1;
            }

            int transcount = 0;
            NasrTranslation trans[ root_entry.value->u.array.length ];
            const char * keys[ root_entry.value->u.array.length ];
            int keyentrycounts[ root_entry.value->u.array.length ];
            int keycount = 0;

            for ( unsigned int j = 0; j < root_entry.value->u.array.length; ++j )
            {
                keys[ j ] = keyentrycounts[ j ] = 0;
            }

            for ( unsigned int j = 0; j < root_entry.value->u.array.length; ++j )
            {
                const json_value * char_item = root_entry.value->u.array.values[ j ];
                if ( char_item->type != json_object )
                {
                    NasrLog( "Charset file malformed: localization entry not an object." );
                    return -1;
                }

                trans[ j ].original = 0;
                trans[ j ].translation = 0;
                trans[ j ].context = 0;
                trans[ j ].translation_plural = 0;

                for ( unsigned int k = 0; k < char_item->u.object.length; ++k )
                {
                    const json_object_entry char_entry = char_item->u.object.values[ k ];
                    if ( strcmp( "original", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            NasrLog( "Charset file malformed: original value is not a string." );
                            return -1;
                        }
                        trans[ j ].original = char_entry.value->u.string.ptr;
                        int alreadyexists = 0;
                        for ( int l = 0; l < keycount; ++l )
                        {
                            if ( strcmp( char_entry.value->u.string.ptr, keys[ l ] ) == 0 )
                            {
                                ++keyentrycounts[ l ];
                                alreadyexists = 1;
                                break;
                            }
                        }

                        if ( !alreadyexists )
                        {
                            keys[ keycount ] = char_entry.value->u.string.ptr;
                            keyentrycounts[ keycount++ ] = 1;
                        }
                    }
                    else if ( strcmp( "translation", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            NasrLog( "Charset file malformed: translation value is not a string." );
                            return -1;
                        }
                        trans[ j ].translation = char_entry.value->u.string.ptr;
                    }
                    else if ( strcmp( "translationPlural", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            NasrLog( "Charset file malformed: translation value is not a string." );
                            return -1;
                        }
                        trans[ j ].translation_plural = char_entry.value->u.string.ptr;
                    }
                    else if ( strcmp( "context", char_entry.name ) == 0 )
                    {
                        if ( char_entry.value->type != json_string )
                        {
                            NasrLog( "Charset file malformed: context value is not a string." );
                            return -1;
                        }
                        trans[ j ].context = char_entry.value->u.string.ptr;
                    }
                }

                ++transcount;
            }

            capacity = NasrGetNextPrime( keycount );

            translations = calloc( capacity, sizeof( NasrTranslationEntry ) );
            for ( int t = 0; t < root_entry.value->u.array.length; ++t )
            {
                if ( trans[ t ].original )
                {
                    hash_t needle_hash = NasrHashString( trans[ t ].original, capacity );
                    NasrTranslationEntry * entry = NasrTranslationFindEntry( trans[ t ].original, needle_hash );
                    if ( !entry->key.string )
                    {
                        for ( int u = 0; u < keycount; ++u )
                        {
                            if ( strcmp( keys[ u ], trans[ t ].original ) == 0 )
                            {
                                entry->key.string = ( char * )( malloc( strlen( trans[ t ].original ) + 1 ) );
                                strcpy( entry->key.string, trans[ t ].original );
                                entry->key.hash = needle_hash;
                                entry->capacity = NasrGetNextPrime( keyentrycounts[ u ] );
                                entry->contexts = calloc( entry->capacity, sizeof( NasrTranslationContext ) );
                                break;
                            }
                        }
                    }

                    const char * contextname = "";
                    if ( trans[ t ].context )
                    {
                        contextname = trans[ t ].context;
                    }


                    NasrTranslationContext * context = NasrTranslationGenContext( entry, contextname );
                    if ( trans[ t ].translation )
                    {
                        context->value.translation = GenString( trans[ t ].translation );
                    }
                    if ( trans[ t ].translation_plural )
                    {
                        context->value.translation_plural = GenString( trans[ t ].translation_plural );
                    }
                }
            }
        }
    }

    json_value_free( root );
    return 0;
};

const char * Nasr__( const char * string, const char * domain )
{
    char * out = string;
    if ( translations )
    {
        hash_t needle_hash = NasrHashString( string, capacity );
        NasrTranslationEntry * entry = NasrTranslationFindEntry( string, needle_hash );
        if ( entry->contexts && entry->capacity )
        {
            needle_hash = NasrHashString( "", entry->capacity );
            NasrTranslationContext * contextobj = NasrTranslationFindContext( entry, "", needle_hash );
            if ( contextobj )
            {
                if ( contextobj->value.translation )
                {
                    return contextobj->value.translation;
                }
            }
        }
    }
    return out;
};

const char * Nasr_x( const char * string, const char * context, const char * domain )
{
    char * out = string;
    if ( translations )
    {
        hash_t needle_hash = NasrHashString( string, capacity );
        NasrTranslationEntry * entry = NasrTranslationFindEntry( string, needle_hash );
        if ( entry->contexts && entry->capacity )
        {
            needle_hash = NasrHashString( context, entry->capacity );
            NasrTranslationContext * contextobj = NasrTranslationFindContext( entry, context, needle_hash );
            if ( contextobj )
            {
                if ( contextobj->value.translation )
                {
                    return contextobj->value.translation;
                }
            }
        }
    }
    return out;
};

const char * Nasr_n( const char * singular, const char * plural, int count, const char * domain )
{
    if ( translations )
    {
        hash_t needle_hash = NasrHashString( singular, capacity );
        NasrTranslationEntry * entry = NasrTranslationFindEntry( singular, needle_hash );
        if ( entry->contexts && entry->capacity )
        {
            needle_hash = NasrHashString( "", entry->capacity );
            NasrTranslationContext * contextobj = NasrTranslationFindContext( entry, "", needle_hash );
            if ( contextobj )
            {
                if ( count == 1 && contextobj->value.translation )
                {
                    return contextobj->value.translation;
                }
                else if ( count != 1 && contextobj->value.translation_plural )
                {
                    return contextobj->value.translation_plural;
                }
            }
        }
    }
    return count == 1 ? singular : plural;
};

static NasrTranslationEntry * NasrTranslationFindEntry( const char * needle_string, hash_t needle_hash )
{
    while ( 1 )
    {
        NasrTranslationEntry * entry = &translations[ needle_hash ];
        if ( entry->key.string == NULL || strcmp( entry->key.string, needle_string ) == 0 )
        {
            return entry;
        }
        needle_hash = ( needle_hash + 1 ) % capacity;
    }
};

static NasrTranslationEntry * NasrTranslationGenEntry( const char * key )
{
    hash_t needle_hash = NasrHashString( key, capacity );
    NasrTranslationEntry * entry = NasrTranslationFindEntry( key, needle_hash );
    if ( !entry->key.string )
    {
        entry->key.string = ( char * )( malloc( strlen( key ) + 1 ) );
        strcpy( entry->key.string, key );
        entry->key.hash = needle_hash;
    }
    return entry;
};

static NasrTranslationContext * NasrTranslationFindContext( NasrTranslationEntry * entry, const char * needle_string, hash_t needle_hash )
{
    for ( int i = 0; i < entry->capacity; ++i )
    {
        NasrTranslationContext * context = &entry->contexts[ needle_hash ];
        if ( context->key.string == NULL || strcmp( context->key.string, needle_string ) == 0 )
        {
            return context;
        }
        needle_hash = ( needle_hash + 1 ) % entry->capacity;
    }
    return 0;
};

static NasrTranslationContext * NasrTranslationGenContext( NasrTranslationEntry * entry, const char * key )
{
    hash_t needle_hash = NasrHashString( key, entry->capacity );
    NasrTranslationContext * context = NasrTranslationFindContext( entry, key, needle_hash );
    if ( !context->key.string )
    {
        context->key.string = ( char * )( malloc( strlen( key ) + 1 ) );
        strcpy( context->key.string, key );
        context->key.hash = needle_hash;
    }
    return context;
};

static char * GenString( const char * in )
{
    char * out = malloc( strlen( in ) + 1 );
    strcpy( out, in );
    return out;
};