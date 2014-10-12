#include "push0.h"


NTSTATUS ReadMsr(void *lpInBuffer, ULONG nInBufferSize, void *lpOutBuffer, ULONG nOutBufferSize, ULONG *lpBytesReturned)
{
	__try
	{
		ULONGLONG data = __readmsr(*(UINT32*)lpInBuffer);
		
		memcpy((UINT32*)lpOutBuffer, &data, 8);
		*lpBytesReturned = 8;
		return STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		*lpBytesReturned = 0;
		return STATUS_UNSUCCESSFUL;
	}
}