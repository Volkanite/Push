#include <sl.h>
#include <wchar.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
//#include <psdk\psdk.h>

//like wcslen
UINT32 String_GetLength( WCHAR* String )
{
    const wchar_t *p;

    if (!String)
        return NULL;

    p = String;

    while (*p)
        p++;

    return p - String;
}


INT32 String_GetLengthN( WCHAR* String, UINT32 MaxLen )
{
    return wcsnlen(String, MaxLen);
}


INT32 String_GetSize( WCHAR* Buffer )
{
    return (String_GetLength(Buffer) + 1) * sizeof(WCHAR);
}


INT32 String_Compare( WCHAR* Source, WCHAR* dst )
{
    int ret = 0 ;

    if (!Source)
        return -1;

    while( ! (ret = (int)(*Source - *dst)) && *dst)
            ++Source, ++dst;

    if ( ret < 0 )
            ret = -1 ;
    else if ( ret > 0 )
            ret = 1 ;

    return( ret );
}


INT32 String_CompareN( WCHAR* s1, WCHAR* s2, UINT_B n )
{
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++) {
            /* XXX assumes wchar_t = int */
            return (*(const unsigned int *)s1 -
                *(const unsigned int *)--s2);
        }
        if (*s1++ == 0)
            break;
    } while (--n != 0);
    return (0);
}


WCHAR* String_Copy( WCHAR* dst, WCHAR* Source )
{
    wchar_t * cp = dst;

    if (!Source)
        return NULL;

    while( *cp++ = *Source++ )
            ;               /* Copy src over dst */

    return( dst );
}


WCHAR* String_CopyN( WCHAR* Destination, WCHAR* Source, UINT32 n )
{
    if (!Source)
        return NULL;

    //Destination[n] = L'\0';

    if (n != 0) {
        wchar_t *d = Destination;
        const wchar_t *s = Source;

        do {
            if ((*d++ = *s++) == L'\0') {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                    *d++ = L'\0';
                break;
            }
        } while (--n != 0);
    }

    return (Destination);
}


WCHAR* String_Concatenate( WCHAR* dst, WCHAR* src )
{
    wchar_t * cp = dst;

    while (*cp)
        cp++;                   /* find end of dst */

    while (*cp++ = *src++);       /* Copy src to end of dst */

    return(dst);                  /* return dst */
}


WCHAR* String_FindFirstChar(
    WCHAR *s,
    WCHAR c
    )
{
    while (*s != c && *s != L'\0')
        s++;
    if (*s == c)
        return ((WCHAR *)s);
    return (NULL);
}


CHAR* SlStringFindCharAnsi(
    CHAR* String,
    CHAR Character
    )
{
    while (*String != Character && *String != '\0')
        String++;

    if (*String == Character)
        return ((CHAR*)String);

    return (NULL);
}


WCHAR* String_FindLastChar(
    WCHAR* s,
    WCHAR c
    )
{
    const wchar_t *last;

    last = NULL;
    for (;;) {
        if (*s == c)
            last = s;
        if (*s == L'\0')
            break;
        s++;
    }

    return ((wchar_t *)last);
}

int bigstring(FILE  *stream, const wchar_t *format, va_list argptr);
typedef int(*WOUTPUTFN)(FILE *, const wchar_t *, va_list);
int __cdecl bigstring_helper(
    WOUTPUTFN woutfn,
    wchar_t *string,
    size_t count,
    const wchar_t *format,
    va_list ap
    );
INT32 String_Format( wchar_t* String, UINT32 Count, const wchar_t* Format, ... )
{
#ifdef GCC
    return 0;
#else
    va_list _Arglist;
    int _Ret;
    _crt_va_start(_Arglist, Format);
    _Ret = _vswprintf_c_l(String, Count, Format, NULL, _Arglist);
    //_Ret = bigstring_helper(bigstring, String, Count, Format, _Arglist);
    _crt_va_end(_Arglist);
    return _Ret;
#endif // GCC
}


INT32 String_ToInteger( WCHAR* String )
{
#ifdef GCC
    return 0;
#else
    return _wtoi(String);
#endif // GCC
}


VOID UnicodeString_Init( UNICODE_STRING* DestinationString, WCHAR* SourceString )
{
    DestinationString->Buffer = SourceString;
    DestinationString->MaximumLength = DestinationString->Length = String_GetLength(SourceString) * sizeof(WCHAR);
}


VOID UTF8ToWchar(
    WCHAR* WcharStringDestination,
    ULONG WcharStringMaxWCharCount,
    CHAR* UTF8StringSource,
    ULONG UTF8StringByteCount
    )
{
    ULONG actualByteCount;

    RtlUTF8ToUnicodeN(
        WcharStringDestination,
        WcharStringMaxWCharCount * sizeof(WCHAR),
        &actualByteCount,
        UTF8StringSource,
        UTF8StringByteCount
        );

    WcharStringDestination[actualByteCount / sizeof(WCHAR)] = L'\0';
}