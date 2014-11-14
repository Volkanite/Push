typedef unsigned char       BYTE;   /*8 bit*/  /* 0 to 255 */
typedef unsigned short      WORD;   /*16 bit*/ /* 0 to 65535 */
typedef unsigned long       DWORD;  /*32 bit*/ /* 0 to 4294967295 */
typedef unsigned long long  QWORD;  /*64 bit*/ /* 0 to 18,446,744,073,709,551,615 */

typedef signed long         SDWORD; /*32 bit*/ /* –2147483648 to 2147483647 */
typedef signed long long    SQWORD; /*64 bit*/ /* –9,223,372,036,854,775,808 to 9,223,372,036,854,775,807 */


typedef BYTE    BOOLEAN;
typedef BYTE    UINT8;
typedef WORD    UINT16;
typedef DWORD   UINT32;
typedef QWORD   UINT64;
typedef SDWORD  INT32;
typedef SQWORD  INT64;
typedef SDWORD  LONG;
typedef DWORD ULONG;
typedef char    CHAR;
typedef BYTE    UCHAR;
//typedef void    VOID;
#define VOID void
typedef float   FLOAT;
typedef double  DOUBLE;
typedef int  INTBOOL;
typedef LONG NTSTATUS;
typedef unsigned long long ULONGLONG;
typedef LONG KPRIORITY;
typedef VOID* HANDLE;


#ifdef __cplusplus
typedef wchar_t WCHAR;
#else
typedef unsigned short wchar_t;
typedef wchar_t WCHAR;
#endif


#if defined(_AMD64_)
 typedef unsigned __int64 UINT_B;
 typedef __int64 INT_B;
 typedef unsigned __int64 SIZE_B;
 typedef unsigned __int64 ULONG_PTR;
#else
 typedef unsigned long UINT_B;
 typedef long INT_B;
 typedef unsigned long SIZE_B;
 typedef unsigned long ULONG_PTR;
#endif

typedef ULONG_PTR SIZE_T;

#define TRUE    1
#define FALSE   0

#define NULL 0
