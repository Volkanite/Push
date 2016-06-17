UINT32 String_GetLength(WCHAR* String);
INT32 String_GetLengthN(WCHAR* String, UINT32 MaxLen);
INT32 String_GetSize(WCHAR* Buffer);
WCHAR* String_Copy(WCHAR* dst, WCHAR* Source);
WCHAR* String_CopyN(WCHAR* Destination, WCHAR* src, UINT32 n);
WCHAR* String_Concatenate(WCHAR* dst, WCHAR* src);
WCHAR* String_FindFirstChar(WCHAR *s, WCHAR c);
WCHAR* String_FindLastChar(WCHAR* s, WCHAR c);
INT32 String_Compare(WCHAR* Source, WCHAR* dst);
INT32 String_CompareN(WCHAR* s1, WCHAR* s2, UINT_B n);
INT32 String_Format(WCHAR* String, UINT32 count, const wchar_t *format, ...);
INT32 String_ToInteger(WCHAR* String);


VOID UnicodeString_Init(UNICODE_STRING* DestinationString, WCHAR* SourceString);
