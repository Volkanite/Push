#include "push0.h"


/NTSTATUS ReadGpuRegister(
	void			*lpInBuffer,
	unsigned long	nInBufferSize,
	void			*lpOutBuffer,
	unsigned long	nOutBufferSize,
	unsigned long	*lpBytesReturned)
{
	void				*lpBarVirtual;
	PHYSICAL_ADDRESS	barPhysicalAddr;
	ULONG size = 4;


	barPhysicalAddr.QuadPart = /*0xE2000000 +*/ *(ULONG*)lpInBuffer;

	lpBarVirtual = MmMapIoSpace(barPhysicalAddr, size, MmNonCached);

	READ_REGISTER_BUFFER_ULONG( (PULONG) lpBarVirtual, (PULONG) lpOutBuffer, 1);

	//MmUnmapIoSpace(lpBarVirtual, size);

	*lpBytesReturned = nOutBufferSize;

	return STATUS_SUCCESS;
}
