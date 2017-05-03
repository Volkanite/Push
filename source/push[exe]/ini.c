#include <sl.h>
#include <slfile.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>


static const char bom_utf8[] = {0xEF,0xBB,0xBF};


typedef struct tagPROFILEKEY
{
    WCHAR                 *Value;
    struct tagPROFILEKEY  *next;
    WCHAR                  Name[1];
} PROFILEKEY;


typedef struct tagPROFILESECTION
{
    struct tagPROFILEKEY       *Key;
    struct tagPROFILESECTION   *next;
    WCHAR                       Name[1];
} PROFILESECTION;


typedef struct
{
    INTBOOL     changed;
    PROFILESECTION  *section;
    WCHAR           *FileName;
    FILETIME LastWriteTime;
    //ENCODING encoding;
} PROFILE;


#define N_CACHED_PROFILES 10
#define INVALID_FILE_SIZE   0xFFFFFFFF
#define IS_TEXT_UNICODE_SIGNATURE 8
#define IS_TEXT_UNICODE_REVERSE_SIGNATURE 128
#define IS_TEXT_UNICODE_ODD_LENGTH 512
#define ERROR_FILE_NOT_FOUND   2
#define RtlUshortByteSwap(_x) _byteswap_ushort((WORD)(_x))

/* Cached profile files */
static PROFILE *MRUProfile[N_CACHED_PROFILES]={NULL};

#define CurrentProfile (MRUProfile[0])

/* Check for comments in profile */
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')

static const WCHAR wininiW[] = { 'w','i','n','.','i','n','i',0 };

extern RTL_CRITICAL_SECTION PROFILE_CritSect;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &PROFILE_CritSect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, 0
};
 RTL_CRITICAL_SECTION PROFILE_CritSect = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef enum _RTL_PATH_TYPE
{
    RtlPathTypeUnknown,
    RtlPathTypeUncAbsolute,
    RtlPathTypeDriveAbsolute,
    RtlPathTypeDriveRelative,
    RtlPathTypeRooted,
    RtlPathTypeRelative,
    RtlPathTypeLocalDevice,
    RtlPathTypeRootLocalDevice,
} RTL_PATH_TYPE;


    RTL_PATH_TYPE
    __stdcall
    RtlDetermineDosPathNameType_U(
    WCHAR* DosFileName
    );
    UINT32 __stdcall GetWindowsDirectoryW(
        WCHAR* lpBuffer,
        UINT32 uSize
        );
    DWORD __stdcall GetFullPathNameW(
        WCHAR* lpFileName,
        DWORD nBufferLength,
        WCHAR* lpBuffer,
        WCHAR** lpFilePart
        );

    BOOLEAN __stdcall RtlIsTextUnicode  (
        VOID*   buf,
        INT32   len,
        INT32 *     pf
        );

        NTSTATUS
        __stdcall
        RtlLeaveCriticalSection(
        RTL_CRITICAL_SECTION* CriticalSection
        );
    INTBOOL __stdcall GetFileTime(
        VOID* hFile,
        FILETIME* lpCreationTime,
        FILETIME* lpLastAccessTime,
        FILETIME* lpLastWriteTime
        );
    VOID __stdcall GetSystemTimeAsFileTime(
        FILETIME* lpSystemTimeAsFileTime
        );



/***********************************************************************
 *           PROFILE_Save
 *
 * Save a profile tree to a file.
 */
VOID
PROFILE_Save( VOID* FileHandle, PROFILESECTION* Section )
{
    PROFILEKEY *key;
    IO_STATUS_BLOCK isb;
    WCHAR *buffer, *p, bom;

    //write UTF16-LE encoding marker to the file
    //PROFILE_WriteMarker(FileHandle);
    bom = 0xFEFF;

    NtWriteFile(
        FileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        &bom,
        sizeof(bom),
        NULL,
        NULL
        );

    for ( ; Section; Section = Section->next)
    {
        int length = 0;

        if (Section->Name[0]) length += String_GetLength( Section->Name ) + 4;

        for (key = Section->Key; key; key = key->next)
        {
            length += String_GetLength(key->Name) + 2;

            if (key->Value) length += String_GetLength(key->Value) + 1;
        }

        buffer = (WCHAR*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, length * sizeof(WCHAR));

        if (!buffer) return;

        p = buffer;
        if (Section->Name[0])
        {
            *p++ = '[';

            String_Copy(p, Section->Name);

            p += String_GetLength(p);
            *p++ = ']';
            *p++ = '\r';
            *p++ = '\n';
        }

        for (key = Section->Key; key; key = key->next)
        {
            String_Copy(p, key->Name);

            p += String_GetLength(p);

            if (key->Value)
            {
                *p++ = '=';

                String_Copy(p, key->Value);

                p += String_GetLength(p);
            }
            *p++ = '\r';
            *p++ = '\n';
        }

        NtWriteFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            length * sizeof(WCHAR),
            NULL,
            NULL
            );

        RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, buffer);
    }
}


/***********************************************************************
 *           PROFILE_FlushFile
 *
 * Flush the current profile to disk if changed.
 */
BOOLEAN
PROFILE_FlushFile(void)
{
    VOID* fileHandle = NULL;
    FILETIME LastWriteTime;
    NTSTATUS status;

    if(!CurrentProfile)
    {
        return FALSE;
    }

    if (!CurrentProfile->changed) return TRUE;

    status = File_Create(
        &fileHandle,
        CurrentProfile->FileName,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    PROFILE_Save( fileHandle, CurrentProfile->section );

    if(GetFileTime(fileHandle, NULL, NULL, &LastWriteTime))
       CurrentProfile->LastWriteTime=LastWriteTime;

    NtClose(fileHandle);

    CurrentProfile->changed = FALSE;

    return TRUE;
}


/***********************************************************************
 *
 * Compares a file time with the current time. If the file time is
 * at least 2.1 seconds in the past, return true.
 *
 * Intended as cache safety measure: The time resolution on FAT is
 * two seconds, so files that are not at least two seconds old might
 * keep their time even on modification, so don't cache them.
 */
BOOLEAN
is_not_current(FILETIME * ft)
{
    FILETIME Now;
    INT64 ftll, nowll;

    GetSystemTimeAsFileTime(&Now);

    ftll = ((INT64)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
    nowll = ((INT64)Now.dwHighDateTime << 32) + Now.dwLowDateTime;

    //TRACE("%08x;%08x\n",(unsigned)ftll+21000000,(unsigned)nowll);

    return ftll + 21000000 < nowll;
}


/***********************************************************************
 *           PROFILE_Free
 *
 * Free a profile tree.
 */
static
VOID
PROFILE_Free( PROFILESECTION *Section )
{
    PROFILESECTION *next_section;
    PROFILEKEY *key, *next_key;

    for ( ; Section; Section = next_section)
    {
        for (key = Section->Key; key; key = next_key)
        {
            next_key = key->next;

            RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, key->Value);
            RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, key);
        }
        next_section = Section->next;

        RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, Section);
    }
}


WCHAR
*memrchrW(const WCHAR *ptr, WCHAR ch, size_t n)
{
    const WCHAR *end;
    WCHAR *ret = NULL;
    for (end = ptr + n; ptr < end; ptr++)
        if (*ptr == ch)
            ret = (WCHAR *)(UINT_B)ptr;
    return ret;
}


/* returns 1 if a character white space else 0 */
static
INT32
PROFILE_isspaceW(WCHAR c)
{
    /* ^Z (DOS EOF) is a space too  (found on CD-ROMs) */
    return iswspace(c) || c == 0x1a;
}


/***********************************************************************
 *           PROFILE_Load
 *
 * Load a profile tree from a file.
 */
PROFILESECTION*
PROFILE_Load( VOID* FileHandle )
{
    void *bufferBase, *pBuffer;
    WCHAR * file;
    const WCHAR *szLineStart, *szLineEnd;
    const WCHAR *szValueStart, *szEnd, *next_line;
    int line = 0, length;
    PROFILESECTION *section, *firstSection;
    PROFILESECTION **next_section;
    PROFILEKEY *key, *prev_key, **next_key;
    DWORD fileSize;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    FILE_STANDARD_INFORMATION fileInformation;

    //fileSize = GetFileSize(FileHandle, NULL);
    NtQueryInformationFile(
        FileHandle,
        &isb,
        &fileInformation,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    fileSize = fileInformation.EndOfFile.QuadPart;

    if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
        return NULL;

    bufferBase = RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, fileSize);

    if (!bufferBase) return NULL;

    status = NtReadFile(FileHandle, NULL, NULL, NULL, &isb, bufferBase, fileSize, NULL, NULL);

    fileSize = isb.Information;

    if (!NT_SUCCESS(status))
    {
        RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, bufferBase);

        return NULL;
    }

    length = sizeof(WCHAR);

    /* len is set to the number of bytes in the character marker.
     * we want to skip these bytes */
    pBuffer = (char *)bufferBase + length;
    fileSize -= length;
    file = (WCHAR*) pBuffer;
    szEnd = (WCHAR *)((char *)pBuffer + fileSize);
    firstSection = (PROFILESECTION*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, sizeof(*section) );

    if(firstSection == NULL)
    {
        if (file != pBuffer)
            RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, file);

        RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, bufferBase);

        return NULL;
    }

    firstSection->Name[0] = 0;
    firstSection->Key  = NULL;
    firstSection->next = NULL;
    next_section = &firstSection->next;
    next_key     = &firstSection->Key;
    prev_key     = NULL;
    next_line    = file;

    while (next_line < szEnd)
    {
        szLineStart = next_line;
        next_line = Memory_FindFirstChar(szLineStart, '\n', szEnd - szLineStart);

        if (!next_line) next_line = Memory_FindFirstChar(szLineStart, '\r', szEnd - szLineStart);
        if (!next_line) next_line = szEnd;
        else next_line++;
        szLineEnd = next_line;

        line++;

        /* get rid of white space */
        while (szLineStart < szLineEnd && PROFILE_isspaceW(*szLineStart)) szLineStart++;
        while ((szLineEnd > szLineStart) && PROFILE_isspaceW(szLineEnd[-1])) szLineEnd--;

        if (szLineStart >= szLineEnd) continue;

        if (*szLineStart == '[')  /* section start */
        {
            const WCHAR * szSectionEnd;
            if (!(szSectionEnd = memrchrW( szLineStart, ']', szLineEnd - szLineStart )))
            {
                /*WARN("Invalid section header at line %d: %s\n",
                    line, debugstr_wn(szLineStart, (int)(szLineEnd - szLineStart)) );*/
            }
            else
            {
                szLineStart++;
                length = (int)(szSectionEnd - szLineStart);
                /* no need to allocate +1 for NULL terminating character as
                 * already included in structure */
                /*if (!(section = HeapAlloc( GetProcessHeap(), 0, sizeof(*section) + length * sizeof(WCHAR) )))
                    break;*/

                if (!(section = (PROFILESECTION*) RtlAllocateHeap( NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, sizeof(*section) + length * sizeof(WCHAR) )))
                    break;

                memcpy(section->Name, szLineStart, length * sizeof(WCHAR));
                section->Name[length] = '\0';
                section->Key  = NULL;
                section->next = NULL;
                *next_section = section;
                next_section  = &section->next;
                next_key      = &section->Key;
                prev_key      = NULL;

                //TRACE("New section: %s\n", debugstr_w(section->name));

                continue;
            }
        }

        /* get rid of white space after the name and before the start
         * of the value */
        length = szLineEnd - szLineStart;
        if ((szValueStart = Memory_FindFirstChar( szLineStart, '=', szLineEnd - szLineStart )) != NULL)
        {
            const WCHAR *szNameEnd = szValueStart;
            while ((szNameEnd > szLineStart) && PROFILE_isspaceW(szNameEnd[-1])) szNameEnd--;
            length = szNameEnd - szLineStart;
            szValueStart++;
            while (szValueStart < szLineEnd && PROFILE_isspaceW(*szValueStart)) szValueStart++;
        }

        if (length || !prev_key || *prev_key->Name)
        {
            /* no need to allocate +1 for NULL terminating character as
             * already included in structure */

            //if (!(key = HeapAlloc( GetProcessHeap(), 0, sizeof(*key) + length * sizeof(WCHAR) ))) break;
            if (!(key = (PROFILEKEY*) RtlAllocateHeap( NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, sizeof(*key) + length * sizeof(WCHAR) )))
                break;

            memcpy(key->Name, szLineStart, length * sizeof(WCHAR));

            key->Name[length] = '\0';

            if (szValueStart)
            {
                length = (int)(szLineEnd - szValueStart);
                //key->value = HeapAlloc( GetProcessHeap(), 0, (length + 1) * sizeof(WCHAR) );
                key->Value = (WCHAR*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, (length + 1) * sizeof(WCHAR) );

                memcpy(key->Value, szValueStart, length * sizeof(WCHAR));

                key->Value[length] = '\0';
            }
            else
            {
                key->Value = NULL;
            }

           key->next  = NULL;
           *next_key  = key;
           next_key   = &key->next;
           prev_key   = key;
        }
    }

    if (file != pBuffer)
        RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, file);

    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, bufferBase);

    return firstSection;
}


/***********************************************************************
 *           PROFILE_ReleaseFile
 *
 * Flush the current profile to disk and remove it from the cache.
 */
VOID
PROFILE_ReleaseFile(void)
{
    PROFILE_FlushFile();

    PROFILE_Free( CurrentProfile->section );

    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, CurrentProfile->FileName);

    CurrentProfile->changed = FALSE;
    CurrentProfile->section = NULL;
    CurrentProfile->FileName  = NULL;

    memset(&CurrentProfile->LastWriteTime, 0, sizeof(CurrentProfile->LastWriteTime));
}


/***********************************************************************
 *           PROFILE_Open
 *
 * Open a profile file, checking the cached file first.
 */
BOOLEAN
PROFILE_Open( WCHAR* Filename, BOOLEAN WriteAccess )
{
    WCHAR buffer[260];
    VOID* fileHandle = INVALID_HANDLE_VALUE;
  FILETIME LastWriteTime = {0};
    int i,j;
    PROFILE *tempProfile;
  NTSTATUS status;

    /* First time around */

    if(!CurrentProfile)
       for(i=0;i<N_CACHED_PROFILES;i++)
       {
         MRUProfile[i] = (PROFILE*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, sizeof(PROFILE));
          if(MRUProfile[i] == NULL) break;
          MRUProfile[i]->changed=FALSE;
          MRUProfile[i]->section=NULL;
          MRUProfile[i]->FileName=NULL;

        memset(&MRUProfile[i]->LastWriteTime, 0, sizeof(FILETIME));
       }

    if (!Filename)
  Filename = (WCHAR*) wininiW;

    if ((RtlDetermineDosPathNameType_U(Filename) == RtlPathTypeRelative) &&
      !String_FindFirstChar(Filename, '\\') && !String_FindFirstChar(Filename, '/'))
    {
        static const WCHAR separator[] = {'\\', 0};
        WCHAR windir[260];

      GetWindowsDirectoryW( windir, 260 );

      String_Copy(buffer, windir);

      String_Concatenate(buffer, (WCHAR*) separator);
      String_Concatenate(buffer, Filename);
    }
    else
    {
        WCHAR* dummy;

        GetFullPathNameW(Filename, sizeof(buffer)/sizeof(buffer[0]), buffer, &dummy);
    }

  status = File_Create(
    &fileHandle,
    buffer,
    FILE_READ_ATTRIBUTES | SYNCHRONIZE | GENERIC_READ | (WriteAccess ? GENERIC_WRITE : 0),
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    FILE_OPEN,
    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
    NULL
    );

  if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND)
  {
      return FALSE;
  }

    for(i=0;i<N_CACHED_PROFILES;i++)
    {
        if ((MRUProfile[i]->FileName && !wcscmp( buffer, MRUProfile[i]->FileName )))
        {
            if(i)
            {
                PROFILE_FlushFile();

                tempProfile=MRUProfile[i];

                for(j=i;j>0;j--)
                    MRUProfile[j]=MRUProfile[j-1];
                CurrentProfile=tempProfile;
            }

            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                GetFileTime(fileHandle, NULL, NULL, &LastWriteTime);

                if (!memcmp( &CurrentProfile->LastWriteTime, &LastWriteTime, sizeof(FILETIME) ) &&
                    is_not_current(&LastWriteTime))
                {

                }
                else
                {
                    PROFILE_Free(CurrentProfile->section);

                    CurrentProfile->section = PROFILE_Load(fileHandle);
                    CurrentProfile->LastWriteTime = LastWriteTime;
                }

                NtClose(fileHandle);

                return TRUE;
            }
        }
    }

    /* Flush the old current profile */
    PROFILE_FlushFile();

    /* Make the oldest profile the current one only in order to get rid of it */
    if(i==N_CACHED_PROFILES)
      {
       tempProfile=MRUProfile[N_CACHED_PROFILES-1];
       for(i=N_CACHED_PROFILES-1;i>0;i--)
          MRUProfile[i]=MRUProfile[i-1];
       CurrentProfile=tempProfile;
      }

    if(CurrentProfile->FileName) PROFILE_ReleaseFile();

    /* OK, now that CurProfile is definitely free we assign it our new file */
    CurrentProfile->FileName = (WCHAR*) RtlAllocateHeap(
        NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
        0,
        (String_GetLength(buffer)+1) * sizeof(WCHAR)
        );

    String_Copy(CurrentProfile->FileName, buffer);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        CurrentProfile->section = PROFILE_Load(fileHandle);

        GetFileTime(fileHandle, NULL, NULL, &CurrentProfile->LastWriteTime);

        NtClose(fileHandle);
    }

    return TRUE;
}


/***********************************************************************
 *           PROFILE_DeleteSection
 *
 * Delete a section from a profile tree.
 */
static
BOOLEAN
PROFILE_DeleteSection( PROFILESECTION **Section, WCHAR* Name )
{
    while (*Section)
    {
        //if ((*section)->Name[0] && !strcmpiW( (*section)->name, name ))
        if ((*Section)->Name[0] && !wcscmp( (*Section)->Name, Name ))
        {
            PROFILESECTION *to_del = *Section;
            *Section = to_del->next;
            to_del->next = NULL;

            PROFILE_Free( to_del );

            return TRUE;
        }

        Section = &(*Section)->next;
    }

    return FALSE;
}



/***********************************************************************
 *           PROFILE_DeleteKey
 *
 * Delete a key from a profile tree.
 */
static
BOOLEAN
PROFILE_DeleteKey( PROFILESECTION **Section, WCHAR* SectionName, WCHAR* KeyName )
{
    while (*Section)
    {
        if ((*Section)->Name[0] && !wcscmp( (*Section)->Name, SectionName ))
        {
            PROFILEKEY **key = &(*Section)->Key;

            while (*key)
            {
                if (!wcscmp( (*key)->Name, KeyName ))
                {
                    PROFILEKEY *to_del = *key;
                    *key = to_del->next;

                    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, to_del->Value);
                    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, to_del);

                    return TRUE;
                }

                key = &(*key)->next;
            }
        }
        Section = &(*Section)->next;
    }
    return FALSE;
}


/***********************************************************************
 *           PROFILE_Find
 *
 * Find a key in a profile tree, optionally creating it.
 */
static PROFILEKEY *PROFILE_Find(
    PROFILESECTION **Section,
    WCHAR* SectionName,
    WCHAR* KeyName,
    BOOLEAN create,
    BOOLEAN create_always
    )
{
    WCHAR* p;
    int seclen, keylen;

    while (PROFILE_isspaceW(*SectionName)) SectionName++;

    if (*SectionName)
        //p = section_name + strlenW(section_name) - 1;
        p = SectionName + String_GetLength(SectionName) - 1;
    else
        p = SectionName;

    while ((p > SectionName) && PROFILE_isspaceW(*p)) p--;
    seclen = p - SectionName + 1;

    while (PROFILE_isspaceW(*KeyName)) KeyName++;

    if (*KeyName)
        //p = key_name + strlenW(key_name) - 1;
        p = KeyName + String_GetLength(KeyName) - 1;
    else
        p = KeyName;

    while ((p > KeyName) && PROFILE_isspaceW(*p)) p--;
    keylen = p - KeyName + 1;

    while (*Section)
    {
        if ( ((*Section)->Name[0])
             //&& (!(strncmpiW( (*section)->name, SectionName, seclen )))
             && (!(wcsncmp( (*Section)->Name, SectionName, seclen )))
             && (((*Section)->Name)[seclen] == '\0') )
        {
            PROFILEKEY **key = &(*Section)->Key;

            while (*key)
            {
                /* If create_always is FALSE then we check if the keyname
                 * already exists. Otherwise we add it regardless of its
                 * existence, to allow keys to be added more than once in
                 * some cases.
                 */
                if(!create_always)
                {
                    if ( (!(wcsncmp( (*key)->Name, KeyName, keylen )))
                         && (((*key)->Name)[keylen] == '\0') )
                        return *key;
                }
                key = &(*key)->next;
            }
            if (!create) return NULL;

            if (!(*key = (PROFILEKEY*) RtlAllocateHeap(
                NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
                0,
                sizeof(PROFILEKEY) + String_GetLength(KeyName) * sizeof(WCHAR) )))
            {
                return NULL;
            }

            String_Copy((*key)->Name, KeyName);

            (*key)->Value = NULL;
            (*key)->next  = NULL;
            return *key;
        }
        Section = &(*Section)->next;
    }
    if (!create) return NULL;

    *Section = (PROFILESECTION*) RtlAllocateHeap(
        NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
        0,
        sizeof(PROFILESECTION) + String_GetLength(SectionName) * sizeof(WCHAR)
        );

    if(*Section == NULL) return NULL;

    String_Copy((*Section)->Name, SectionName);

    (*Section)->next = NULL;

    if (!((*Section)->Key = (PROFILEKEY*) RtlAllocateHeap(
        NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
        0,
        sizeof(PROFILEKEY) + String_GetLength(KeyName) * sizeof(WCHAR) )))
    {
        RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, *Section);

        return NULL;
    }

    String_Copy((*Section)->Key->Name, KeyName);

    (*Section)->Key->Value = NULL;
    (*Section)->Key->next  = NULL;

    return (*Section)->Key;
}


/***********************************************************************
 *           PROFILE_SetString
 *
 * Set a profile string.
 */
static
BOOLEAN
PROFILE_SetString(
    WCHAR* section_name,
    WCHAR* key_name,
    WCHAR* Value,
    BOOLEAN create_always
    )
{
    if (!key_name)  /* Delete a whole section */
    {
        //TRACE("(%s)\n", debugstr_w(section_name));

        CurrentProfile->changed |= PROFILE_DeleteSection( &CurrentProfile->section, section_name );

        return TRUE;         /* Even if PROFILE_DeleteSection() has failed,
                                this is not an error on application's level.*/
    }
    else if (!Value)  /* Delete a key */
    {
        //TRACE("(%s,%s)\n", debugstr_w(section_name), debugstr_w(key_name) );
        CurrentProfile->changed |= PROFILE_DeleteKey( &CurrentProfile->section, section_name, key_name );
        return TRUE;          /* same error handling as above */
    }
    else  /* Set the key value */
    {
        PROFILEKEY *key = PROFILE_Find(&CurrentProfile->section, section_name,
                                        key_name, TRUE, create_always );
        /*TRACE("(%s,%s,%s):\n",
              debugstr_w(section_name), debugstr_w(key_name), debugstr_w(value) );*/

        if (!key) return FALSE;

        /* strip the leading spaces. We can safely strip \n\r and
         * friends too, they should not happen here anyway. */
        while (PROFILE_isspaceW(*Value)) Value++;

        if (key->Value)
        {
            if (!wcscmp( key->Value, Value ))
            {
                return TRUE;  /* No change needed */
            }

            RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, key->Value );
        }

        key->Value = (WCHAR*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, (String_GetLength(Value) + 1) * sizeof(WCHAR) );

        String_Copy(key->Value, Value);

        CurrentProfile->changed = TRUE;
    }

    return TRUE;
}


static
INT32
PROFILE_GetSectionNames( WCHAR* buffer, DWORD len )
{
    WCHAR* buf;
    UINT32 buflen,tmplen;
    PROFILESECTION *section;

    //TRACE("(%p, %d)\n", buffer, len);

    if (!buffer || !len)
        return 0;
    if (len==1) {
        *buffer='\0';
        return 0;
    }

    buflen=len-1;
    buf=buffer;
    section = CurrentProfile->section;
    while ((section!=NULL)) {
        if (section->Name[0])
        {
            tmplen = String_GetLength(section->Name);

            if (tmplen >= buflen)
            {
                if (buflen > 0) {
                    memcpy(buf, section->Name, (buflen-1) * sizeof(WCHAR));
                    buf += buflen-1;
                    *buf++='\0';
                }
                *buf='\0';
                return len-2;
            }

            memcpy(buf, section->Name, tmplen * sizeof(WCHAR));
            buf += tmplen;
            buflen -= tmplen;
        }
        section = section->next;
    }
    *buf='\0';
    return buf-buffer;
}


/***********************************************************************
 *           PROFILE_CopyEntry
 *
 * Copy the content of an entry into a buffer, removing quotes, and possibly
 * translating environment variables.
 */
static
VOID
PROFILE_CopyEntry(
    WCHAR* Buffer,
    WCHAR* Value,
    UINT32 Length,
    BOOLEAN strip_quote
    )
{
    WCHAR quote = '\0';

    if(!Buffer) return;

    if (strip_quote && ((*Value == '\'') || (*Value == '\"')))
    {
        if (Value[1] && (Value[String_GetLength(Value) - 1] == *Value)) quote = *Value++;
    }

    String_CopyN( Buffer, Value, Length);

    if (quote && (Length >= String_GetLength(Value))) Buffer[String_GetLength(Buffer) - 1] = '\0';
}

#include <string.h>
#include <stdlib.h>
/***********************************************************************
 *           PROFILE_GetSection
 *
 * Returns all keys of a section.
 * If return_values is TRUE, also include the corresponding values.
 */
static
INT32
PROFILE_GetSection(
    PROFILESECTION *Section,
    WCHAR* SectionName,
    WCHAR* Buffer,
    DWORD Length,
    BOOLEAN return_values
    )
{
    PROFILEKEY *key;

    if(!Buffer) return 0;

    while (Section)
    {
        if (Section->Name[0] && !wcscmp( Section->Name, SectionName ))
        {
            UINT32 oldlen = Length;
            UINT32 bufferLength;

            for (key = Section->Key; key; key = key->next)
            {
                if (Length <= 2)
                    break;

                if (!*key->Name)
                    continue;  /* Skip empty lines */

                if (IS_ENTRY_COMMENT(key->Name))
                    continue;  /* Skip comments */

                if (!return_values && !key->Value)
                    continue;  /* Skip lines w.o. '=' */

                PROFILE_CopyEntry( Buffer, key->Name, Length - 1, 0 );

                // Return if buffer too small
                if (String_GetLength(key->Name) > (Length - 1))
                    return oldlen - 2;

                bufferLength = String_GetLengthN( Buffer, Length ) + 1;
                Length -= bufferLength;
                Buffer += bufferLength;

                if (Length < 2)
                    break;

                if (return_values && key->Value)
                {
                    Buffer[-1] = '=';

                    PROFILE_CopyEntry ( Buffer, key->Value, Length - 1, 0 );

                    Length -= String_GetLength( Buffer ) + 1;
                    Buffer += String_GetLength( Buffer ) + 1;
                }
            }

            *Buffer = '\0';

            if (Length <= 1)
                // If either lpszSection or lpszKey is NULL and the supplied destination buffer is too small to hold all the
                // strings, the last string is truncated and followed by two null characters. In this case, the return value is
                // equal to cchReturnBuffer minus two.
            {
        Buffer[-1] = '\0';
                return oldlen - 2;
            }
            return oldlen - Length;
        }
        Section = Section->next;
    }
    Buffer[0] = Buffer[1] = '\0';
    return 0;
}


/***********************************************************************
 *           PROFILE_GetString
 *
 * Get a profile string.
 *
 * Tests with GetPrivateProfileString16, W95a,
 * with filled buffer ("****...") and section "set1" and key_name "1" valid:
 * section  key_name    def_val     res buffer
 * "set1"   "1"     "x"     43  [data]
 * "set1"   "1   "      "x"     43  [data]      (!)
 * "set1"   "  1  "'    "x"     43  [data]      (!)
 * "set1"   ""      "x"     1   "x"
 * "set1"   ""      "x   "      1   "x"     (!)
 * "set1"   ""      "  x   "    3   "  x"       (!)
 * "set1"   NULL        "x"     6   "1\02\03\0\0"
 * "set1"   ""      "x"     1   "x"
 * NULL     "1"     "x"     0   ""      (!)
 * ""       "1"     "x"     1   "x"
 * NULL     NULL        ""      0   ""
 *
 *
 */
static
INT32
PROFILE_GetString(
    WCHAR* section,
    WCHAR* key_name,
    WCHAR* def_val,
    WCHAR* Buffer,
    DWORD len
    )
{
    PROFILEKEY *key = NULL;
    static const WCHAR empty_strW[] = { 0 };

    if(!Buffer || !len) return 0;

    if (!def_val) def_val = (WCHAR*) empty_strW;

    if (key_name)
    {
        if (!key_name[0])
        {
            PROFILE_CopyEntry(Buffer, def_val, len, TRUE);

            //return strlenW(buffer);
            return String_GetLength( Buffer );
        }

        key = PROFILE_Find(
                &CurrentProfile->section,
                section,
                key_name,
                FALSE,
                FALSE
                );

        PROFILE_CopyEntry(
            Buffer,
            (key && key->Value) ? key->Value : def_val,
            len,
            TRUE
            );

        /*TRACE("(%s,%s,%s): returning %s\n",
              debugstr_w(section), debugstr_w(key_name),
              debugstr_w(def_val), debugstr_w(Buffer) );*/

        //return strlenW( Buffer );
        return String_GetLength( Buffer );
    }
    /* no "else" here ! */
    if (section && section[0])
    {
        INT32 ret = PROFILE_GetSection(
                    CurrentProfile->section,
                    section,
                    Buffer,
                    len,
                    FALSE
                    );

        if (!Buffer[0]) /* no luck -> def_val */
        {
            PROFILE_CopyEntry(Buffer, def_val, len, TRUE);

            //ret = strlenW(Buffer);
            ret = String_GetLength(Buffer);
        }

        return ret;
    }

    Buffer[0] = '\0';

    return 0;
}


/**
* Copies a string into the specified section of an initialization file. 
* Synonymous to WritePrivateProfileString().
*
* \param Section Name of the section. Like lpAppName.
* \param Entry Name of the entry. Like lpKeyName.
* \param String The string. Like lpString.
* \param FileName The ini filename. Like lpFileName.
*/

BOOLEAN SlIniWriteString( WCHAR* section, WCHAR* entry, WCHAR* string, WCHAR* filename )
{
  BOOLEAN ret = FALSE;

  RtlEnterCriticalSection( &PROFILE_CritSect );

  if (!section && !entry && !string) /* documented "file flush" case */
  {
      if (!filename || PROFILE_Open( filename, TRUE ))
      {
          if (CurrentProfile) PROFILE_ReleaseFile();  /* always return FALSE in this case */
      }
  }
  else if (PROFILE_Open( filename, TRUE ))
  {
      if (!section) {
          SetLastError(ERROR_FILE_NOT_FOUND);
      } else {
          ret = PROFILE_SetString( section, entry, string, FALSE);
          PROFILE_FlushFile();
      }
  }

  RtlLeaveCriticalSection( &PROFILE_CritSect );

  return ret;
}


DWORD
GetString(
    WCHAR* section,
    WCHAR* entry,
    WCHAR* def_val,
    WCHAR* Buffer,
    DWORD Length,
    WCHAR* filename
    )
{
    int     ret;
    WCHAR*  defval_tmp = NULL;

    /*TRACE("%s,%s,%s,%p,%u,%s\n", debugstr_w(section), debugstr_w(entry),
          debugstr_w(def_val), buffer, len, debugstr_w(filename));*/

    /* strip any trailing ' ' of def_val. */
    if (def_val)
    {
        //WCHAR* p = def_val + strlenW(def_val) - 1;
        WCHAR* p = def_val + String_GetLength(def_val) - 1;

        while (p > def_val && *p == ' ')
            p--;

        if (p >= def_val)
        {
            int length = (int)(p - def_val) + 1;

            defval_tmp = (WCHAR*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, (length + 1) * sizeof(WCHAR));

            memcpy(defval_tmp, def_val, length * sizeof(WCHAR));
            defval_tmp[length] = '\0';
            def_val = defval_tmp;
        }
    }

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, FALSE ))
    {
        if (section == NULL)
            ret = PROFILE_GetSectionNames(Buffer, Length);
        else
        /* PROFILE_GetString can handle the 'entry == NULL' case */
            ret = PROFILE_GetString( section, entry, def_val, Buffer, Length );
    }
    else if (Buffer && def_val)
    {
        String_CopyN(Buffer, def_val, Length);

        ret = String_GetLength( Buffer );
    }
    else
    {
       ret = 0;
    }

    RtlLeaveCriticalSection( &PROFILE_CritSect );
    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, defval_tmp);

    return ret;
}


VOID
SlIniWriteBoolean( WCHAR* Section, WCHAR* Key, BOOLEAN bolValue, WCHAR* File )
{
    WCHAR value[255];

    swprintf(value, 255, L"%ls", bolValue ? L"True" : L"False");
    SlIniWriteString(Section, Key, value, File);
}


VOID SlIniWriteSubKey( WCHAR *Section, WCHAR *pszMasterKey, WCHAR *pszSubKey, WCHAR *Value, WCHAR* File )
{
    WCHAR key[260];

    String_Copy(key, L"(");

    String_Concatenate(key, pszMasterKey);
    String_Concatenate(key, L").");
    String_Concatenate(key, pszSubKey);

    SlIniWriteString(Section, key, Value, File);
}


BOOLEAN
SlIniReadBoolean(WCHAR* Section, WCHAR* Key, BOOLEAN DefaultValue, WCHAR* File)
{
    WCHAR result[255], defaultValue[255];
    BOOLEAN bResult;

    swprintf(defaultValue, 255, L"%ls", DefaultValue? L"True" : L"False");
    GetString(Section, Key, defaultValue, result, 255, File);

    bResult =  (wcscmp(result, L"True") == 0 ||
        wcscmp(result, L"true") == 0) ? TRUE : FALSE;

    return bResult;
}


WCHAR*
SlIniReadString( WCHAR* Section, WCHAR* Key, WCHAR* DefaultValue, WCHAR* File )
{
    INT32 bufferSize = 1000;
    INT32 count = 0;
    DWORD returnValue;
    WCHAR* buffer = (WCHAR*) RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, 4);

    do
    {
        count++;

        buffer = (WCHAR*) RtlReAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, buffer, bufferSize * 2 * count);

        returnValue = GetString(Section, Key, DefaultValue, buffer, bufferSize * count, File);

      if (!returnValue)
      {
          RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, buffer);

          return NULL;
      }

    } while( (returnValue == ((bufferSize * count) - 1))
            || (returnValue == ((bufferSize * count) - 2)) );

    return buffer;
}


WCHAR* SlIniReadSubKey( WCHAR *section, WCHAR* MasterKey, WCHAR *subKey, WCHAR* File )
{
    WCHAR szKey[260];

    if (!MasterKey)
    {
        return NULL;
    }

    String_Copy(szKey, L"(");

    String_Concatenate(szKey, MasterKey);
    String_Concatenate(szKey, L").");
    String_Concatenate(szKey, subKey);

    return SlIniReadString(section, szKey, 0, File);
}


BOOLEAN SlIniReadSubKeyBoolean( WCHAR* Section, WCHAR* MasterKey, WCHAR* subKey, BOOLEAN DefaultValue, WCHAR* File )
{
    WCHAR result[255], defaultValue[255];
    WCHAR key[260];
    BOOLEAN bResult;

    if (!MasterKey)
        return FALSE;

    String_Copy(key, L"(");

    String_Concatenate(key, MasterKey);
    String_Concatenate(key, L").");
    String_Concatenate(key, subKey);

    swprintf(defaultValue, 255, L"%ls", DefaultValue? L"True" : L"False");
    GetString(Section, key, defaultValue, result, 255, File);

    bResult =  (wcscmp(result, L"True") == 0 ||
        wcscmp(result, L"true") == 0) ? TRUE : FALSE;

    return bResult;
}
