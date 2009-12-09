#ifndef _PARSER_PROPERTIES_H
#define _PARSER_PROPERTIES_H

#include <stdio.h>

#define MAX_PROPERTY_LEN 512

#ifdef __cplusplus
extern "C"{
#endif
//The struct is allocated by bs.
//All the string should be utf8, and should end with '\0'

struct BookProperties {
    char filename[MAX_PROPERTY_LEN]; 
    long filesize;                   //Unit is byte.
    char filedate[MAX_PROPERTY_LEN]; //Printable file date. for example 08/22/2007
    char name[MAX_PROPERTY_LEN];
    char author[MAX_PROPERTY_LEN];
    char series[MAX_PROPERTY_LEN];  // e.g. "Harry Potter #2"
    char isbn[MAX_PROPERTY_LEN];
    char publisher[MAX_PROPERTY_LEN];
    char publish_date[MAX_PROPERTY_LEN];
    char translator[MAX_PROPERTY_LEN];
    char original_name[MAX_PROPERTY_LEN]; // name of book in original language, for translation
    char original_author[MAX_PROPERTY_LEN]; // author(s) in original language, for translation
    char original_series[MAX_PROPERTY_LEN]; // author(s) in original language, for translation
    // more properties follow here...
	char reserved[MAX_PROPERTY_LEN*6];
};

//localLanguage is current language that user have set. 
//more details can refer to the parser-viewer-interface.h

int GetBookProperties(char *name,  struct BookProperties* pBookProps, int localLanguage);

#ifdef __cplusplus
}
#endif
#endif
