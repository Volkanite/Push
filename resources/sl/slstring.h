class String
{
public:
    static INT32 GetLength( WCHAR* String );
	static INT32 GetLengthN( WCHAR* String, UINT32 MaxLen );
	static INT32 GetSize( WCHAR* Buffer );
    static WCHAR* Copy( WCHAR* dst, WCHAR* Source );
    static WCHAR* CopyN( WCHAR* Destination, WCHAR* src, UINT32 n );
    static WCHAR* Concatenate( WCHAR* dst, WCHAR* src );
    static WCHAR* FindFirstChar( WCHAR *s, WCHAR c );
    static WCHAR* FindLastChar( WCHAR* s, WCHAR c );
    static INT32 Compare( WCHAR* Source, WCHAR* dst );
    static INT32 CompareN( WCHAR* s1, WCHAR* s2, UINT_B n );
    static INT32 Format( WCHAR* String, UINT32 count, const wchar_t *format, ... );
    static INT32 ToInteger( WCHAR* String );
};


class UnicodeString
{
public:
    static VOID Init( UNICODE_STRING* DestinationString, WCHAR* SourceString );
};


class AnsiString
{
public:
    static INT32 GetLength( CHAR* String );
    static VOID Concatenate( CHAR* Destination, CHAR* Source );
    static VOID Format( CHAR* Destination, CHAR* Format, ... );
};
