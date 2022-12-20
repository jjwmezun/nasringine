#ifndef NASR_LOCALIZATION_H
#define NASR_LOCALIZATION_H

int NasrSetLanguage( const char * filename, const char * domain );
const char * Nasr__( const char * string, const char * domain );
const char * Nasr_x( const char * string, const char * context, const char * domain );
const char * Nasr_n( const char * singular, const char * plural, int count, const char * domain );

#endif // NASR_LOCALIZATION_H