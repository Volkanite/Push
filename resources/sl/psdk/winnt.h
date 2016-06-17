#define FILE_READ_DATA            0x00000001
#define FILE_WRITE_DATA            0x00000002
#define MEM_RESERVE          0x2000
#define WRITE_DAC    0x40000L
#define SECURITY_DESCRIPTOR_REVISION 1
#define DACL_SECURITY_INFORMATION        (0x00000004L)
#define READ_CONTROL                     (0x00020000L)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))
                                 
typedef VOID* PSECURITY_DESCRIPTOR;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;

    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;

    INT64 QuadPart;

} LARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    QWORD QuadPart;
} ULARGE_INTEGER;

typedef struct _GUID {
    DWORD   Data1;
    WORD    Data2;
    WORD    Data3;
    BYTE    Data4[ 8 ];
} GUID;

typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY  *Flink;
  struct _LIST_ENTRY  *Blink;
} LIST_ENTRY;

typedef struct _RTL_CRITICAL_SECTION_DEBUG
{
    UINT16 Type;
    UINT16 CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[2];
} RTL_CRITICAL_SECTION_DEBUG;

typedef struct _RTL_CRITICAL_SECTION
{
    RTL_CRITICAL_SECTION_DEBUG* DebugInfo;
    INT32                       LockCount;
    INT32                       RecursionCount;
    VOID*                       OwningThread;
    VOID*                       LockSemaphore;
    UINT_B                SpinCount;
} RTL_CRITICAL_SECTION;

typedef enum _EXCEPTION_DISPOSITION
{
    ExceptionContinueExecution = 0,
    ExceptionContinueSearch = 1,
    ExceptionNestedException = 2,
    ExceptionCollidedUnwind = 3
} EXCEPTION_DISPOSITION;

typedef struct _EXCEPTION_REGISTRATION_RECORD EXCEPTION_REGISTRATION_RECORD;
typedef struct _EXCEPTION_REGISTRATION_RECORD
{
    EXCEPTION_REGISTRATION_RECORD *Next;
    EXCEPTION_DISPOSITION *Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef struct _NT_TIB NT_TIB;
typedef struct _NT_TIB
{
     EXCEPTION_REGISTRATION_RECORD *ExceptionList;
     VOID *StackBase;
     VOID *StackLimit;
     VOID *SubSystemTib;
     union
     {
          VOID *FiberData;
          UINT32 Version;
     };
     VOID *ArbitraryUserPointer;
     NT_TIB *Self;
} NT_TIB;

//
// Structure to represent a system wide processor number. It contains a
// group number and relative processor number within the group.
//

typedef struct _PROCESSOR_NUMBER {
    UINT16 Group;
    UINT8 Number;
    UINT8 Reserved;
} PROCESSOR_NUMBER;