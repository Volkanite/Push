#include <sl.h>
#include <string.h>
#include <RTSSSharedMemory.h>
#include <push.h>


LPRTSS_SHARED_MEMORY RTSSSharedMemory;


NTSTATUS __stdcall NtOpenSection(
    HANDLE* SectionHandle,
    DWORD DesiredAccess,
    OBJECT_ATTRIBUTES* ObjectAttributes
    );


VOID Initialize()
{
    HANDLE sectionHandle;
    OBJECT_ATTRIBUTES objAttrib;
    UNICODE_STRING sectionName;
    VOID* viewBase;
    SIZE_T viewSize;

    UnicodeString_Init(&sectionName, L"RTSSSharedMemoryV2");

    objAttrib.Length = sizeof(OBJECT_ATTRIBUTES);
    objAttrib.RootDirectory = PushBaseGetNamedObjectDirectory();
    objAttrib.ObjectName = &sectionName;
    objAttrib.Attributes = OBJ_OPENIF;
    objAttrib.SecurityDescriptor = NULL;
    objAttrib.SecurityQualityOfService = NULL;

    NtOpenSection(&sectionHandle, SECTION_ALL_ACCESS, &objAttrib);

    //must be NULL or will fail
    viewBase = NULL;
    viewSize = 0;

    NtMapViewOfSection(sectionHandle, NtCurrentProcess(), &viewBase, 0, 0, NULL, &viewSize, ViewShare, 0, PAGE_READWRITE);

    RTSSSharedMemory = (LPRTSS_SHARED_MEMORY) viewBase;
}


VOID RTSS_Update( OSD_ITEM* OsdItems )
{
    UINT8 i;
    OSD_ITEM *osdItem;
    osdItem = OsdItems;
    WCHAR osdText[256];
    CHAR ansiText[256];
    BOOLEAN stuffToDraw = FALSE;

    for (i = 0; i < PushSharedMemory->NumberOfOsdItems; i++, osdItem++)
    {
        if (!osdItem->Flag //draw if no flag, could be somebody just wants to display stuff on-screen
            || PushSharedMemory->OSDFlags & osdItem->Flag //if it has a flag, is it set?
            || (osdItem->Threshold && osdItem->Value > osdItem->Threshold)) //is the item's value > it's threshold?
        {
            if (i == 0)
                String_Copy(osdText, osdItem->Text);
            else
                String_Concatenate(osdText, osdItem->Text);

            stuffToDraw = TRUE;
        }
    }

    if (!stuffToDraw) return;
    if (!RTSSSharedMemory) Initialize();
    if (!RTSSSharedMemory) return;

    WideCharToMultiByte(CP_ACP, 0, osdText, -1, ansiText, 256, NULL, NULL);

    for (DWORD dwPass = 0; dwPass<2; dwPass++)
        //1st pass : find previously captured OSD slot
        //2nd pass : otherwise find the first unused OSD slot and capture it
    {
        for (DWORD dwEntry = 0; dwEntry<RTSSSharedMemory->dwOSDArrSize; dwEntry++)
        {
            RTSS_SHARED_MEMORY_OSD_ENTRY *pEntry = (RTSS_SHARED_MEMORY_OSD_ENTRY*)((BYTE*)RTSSSharedMemory + RTSSSharedMemory->dwOSDArrOffset + dwEntry * RTSSSharedMemory->dwOSDEntrySize);

            if (dwPass)
            {
                if (!strlen(pEntry->szOSDOwner)) strcpy(pEntry->szOSDOwner, "RTSSPush");
            }

            if (!strcmp(pEntry->szOSDOwner, "RTSSPush"))
            {
                strncpy(pEntry->szOSD, ansiText, sizeof(pEntry->szOSD) - 1);

                RTSSSharedMemory->dwOSDFrame++;

                break;
            }
        }

    }
}