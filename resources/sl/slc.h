#ifdef __cplusplus
extern "C" {
#endif

VOID SlInitUnicodeString( 
    UNICODE_STRING* DestinationString, 
    WCHAR* SourceString 
    );
    
INT32 SlStringCompare( 
    WCHAR* s1, 
    WCHAR* s2, 
    int n
    );

WCHAR* SlStringCopy(
    WCHAR* dst,
    WCHAR* src, 
    UINT32 n
    );
    
WCHAR* SlStringFindChar(
    WCHAR* s, 
    WCHAR c
    );
    
    
/**
* Locate first occurrence of character in ansi string.
*
* \param String The string.
* \param Character Character to be located.
* \return-success A pointer to the first occurrence of Character in String.
* \return-fail Null pointer.
*/  

CHAR*
SlStringFindCharAnsi(
    CHAR* String,
    CHAR Character
    );

WCHAR* SlStringFindLastChar(
    WCHAR* s, 
    WCHAR c
    );

INT32 
SlStringGetLength(
    WCHAR* s 
    );
    
#ifdef __cplusplus
}
#endif

FLOAT SlStringToFloat( 
    WCHAR* String 
    );

VOID SlFormatString( 
    WCHAR* buf, 
    const WCHAR* fmt, 
    ...
    );