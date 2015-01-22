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
    BOOLEAN defaultValue,
	WCHAR* File
    );

WCHAR* IniReadString(
    WCHAR* section,
    WCHAR* key,
    WCHAR* defaultValue,
	WCHAR* File
    );

WCHAR*
IniReadSubKey(
    WCHAR *section,
    WCHAR *masterKey,
    WCHAR *subKey,
	WCHAR* File
    );

BOOLEAN IniReadSubKeyBoolean(
    WCHAR *section,
    WCHAR *masterKey,
    WCHAR *subKey,
    BOOLEAN defaultValue,
	WCHAR* File
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
    BOOLEAN bolValue,
	WCHAR* File
    );

VOID IniWriteSubKey(
    WCHAR *pszSection,
    WCHAR *pszMasterKey,
    WCHAR *pszSubKey,
    WCHAR *pszValue,
	WCHAR* File
	);

BOOLEAN IniWriteString( 
    WCHAR* section, 
    WCHAR* entry, 
    WCHAR* string, 
    WCHAR* filename 
    );

#ifdef __cplusplus
}
#endif

 #endif //INI_H
