#ifndef BATCH_H
#define BATCH_H


#include "game.h"

    FILE_LIST FileList;
    WCHAR* BatchFileName;
    UINT64 BatchSize;
    BfBatchFile( PUSH_GAME* Game );
	BOOLEAN BatchFile_IsBatchedFile(FILE_LIST_ENTRY* File);
    UINT64 BatchFile_GetBatchSize();
	VOID BatchFile_SaveBatchFile();
	VOID BatchFile_AddItem(FILE_LIST_ENTRY* File);
	VOID BatchFile_RemoveItem(FILE_LIST_ENTRY* File);
	FILE_LIST BatchFile_GetBatchList();

#endif //BATCH_H