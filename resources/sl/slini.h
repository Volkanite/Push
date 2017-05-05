#ifndef INI_H
#define INI_H



INT32
SlIniReadInteger(
    char* szSection,
    char* szKey,
    INT32 iDefaultValue
    );

FLOAT
SlIniReadFloat(
    CHAR* szSection,
    CHAR* szKey,
    FLOAT fltDefaultValue
    );

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN SlIniReadBoolean(
    WCHAR* section,
    WCHAR* key,
    BOOLEAN defaultValue,
    WCHAR* File
    );

DWORD Ini_GetString(
    wchar_t* section, 
    wchar_t* entry, 
    wchar_t* def_val, 
    wchar_t* Buffer, 
    DWORD Length, 
    wchar_t* filename
    );

WCHAR* SlIniReadSubKey(
    WCHAR *section,
    WCHAR *masterKey,
    WCHAR *subKey,
    WCHAR* File
    );

BOOLEAN SlIniReadSubKeyBoolean(
    WCHAR *section,
    WCHAR *masterKey,
    WCHAR *subKey,
    BOOLEAN defaultValue,
    WCHAR* File
    );

VOID SlIniWriteInteger(
    CHAR* szSection,
    CHAR* szKey,
    INT32 iValue
    );

VOID SlIniWriteFloat(
    CHAR* szSection,
    CHAR* szKey,
    FLOAT fltValue );

VOID SlIniWriteBoolean(
    WCHAR* szSection,
    WCHAR* szKey,
    BOOLEAN bolValue,
    WCHAR* File
    );

VOID SlIniWriteSubKey(
    WCHAR *pszSection,
    WCHAR *pszMasterKey,
    WCHAR *pszSubKey,
    WCHAR *pszValue,
    WCHAR* File
    );

BOOLEAN SlIniWriteString( 
    WCHAR* section, 
    WCHAR* entry, 
    WCHAR* string, 
    WCHAR* filename 
    );

#ifdef __cplusplus
}
#endif

 #endif //INI_H
