#ifndef INI_H
#define INI_H



INT32
IniReadInteger(
    char* szSection,
    char* szKey,
    INT32 iDefaultValue
    );

FLOAT
IniReadFloat(
    CHAR* szSection,
    CHAR* szKey,
    FLOAT fltDefaultValue
    );

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN IniReadBoolean(
    WCHAR* section,
    WCHAR* key,
    BOOLEAN defaultValue
    );

WCHAR* IniReadString(
    WCHAR* section,
    WCHAR* key,
    WCHAR* defaultValue
    );

WCHAR*
IniReadSubKey(
    WCHAR *section,
    WCHAR *masterKey,
    WCHAR *subKey
    );

BOOLEAN IniReadSubKeyBoolean(
    WCHAR *section,
    WCHAR *masterKey,
    WCHAR *subKey,
    BOOLEAN defaultValue
    );

VOID
IniWriteInteger(
    CHAR* szSection,
    CHAR* szKey,
    INT32 iValue
    );

VOID IniWriteFloat(
    CHAR* szSection,
    CHAR* szKey,
    FLOAT fltValue );

VOID IniWriteBoolean(
    WCHAR* szSection,
    WCHAR* szKey,
    BOOLEAN bolValue
    );

VOID IniWriteSubKey(
    WCHAR *pszSection,
    WCHAR *pszMasterKey,
    WCHAR *pszSubKey,
    WCHAR *pszValue );

#ifdef __cplusplus
}
#endif

 #endif //INI_H
