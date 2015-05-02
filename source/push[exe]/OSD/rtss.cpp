#include <sl.h>
#include <string.h>
#include <RTSSSharedMemory.h>


LPRTSS_SHARED_MEMORY RTSSSharedMemory;


void UpdateOSD( CHAR* lpText )
{
	for (DWORD dwPass = 0; dwPass<2; dwPass++)
		//1st pass : find previously captured OSD slot
		//2nd pass : otherwise find the first unused OSD slot and capture it
	{
		for (DWORD dwEntry = 0; dwEntry<RTSSSharedMemory->dwOSDArrSize; dwEntry++)
		{
			RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((BYTE*)RTSSSharedMemory + RTSSSharedMemory->dwOSDArrOffset + dwEntry * RTSSSharedMemory->dwOSDEntrySize);

			if (dwPass)
			{
				if (!strlen(pEntry->szOSDOwner)) strcpy(pEntry->szOSDOwner, "RTSSPush");
			}

			if (!strcmp(pEntry->szOSDOwner, "RTSSPush"))
			{
				strncpy(pEntry->szOSD, lpText, sizeof(pEntry->szOSD) - 1);

				RTSSSharedMemory->dwOSDFrame++;

				break;
			}
		}

	}
}