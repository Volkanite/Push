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

BOOLEAN Ini_ReadBoolean(
    WCHAR* section,
    WCHAR* key,
    BOOLEAN defaultValue,
    WCHAR* FileName
    );

DWORD Ini_GetString(
    wchar_t* section, 
    wchar_t* entry, 
    wchar_t* def_val, 
    wchar_t* Buffer, 
    DWORD Length,
    wchar_t* FileName
    );

VOID Ini_ReadSubKey(
    WCHAR* section,
    WCHAR* MasterKey,
    WCHAR* subKey,
    WCHAR* DefaultString,
    WCHAR* Buffer,
    DWORD Length,
    WCHAR* FileName
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
    BOOLEAN bolValue
    );

VOID SlIniWriteSubKey(
    WCHAR *pszSection,
    WCHAR *pszMasterKey,
    WCHAR *pszSubKey,
    WCHAR *pszValue
    );

BOOLEAN Ini_WriteString(
    WCHAR* section,
    WCHAR* entry,
    WCHAR* string,
    WCHAR* FileName
    );

#ifdef __cplusplus
}
#endif

 #endif //INI_H
