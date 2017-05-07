#ifndef BATCH_H
#define BATCH_H


#include "game.h"

    FILE_LIST FileList;
    UINT64 BatchSize;
    BfBatchFile( PUSH_GAME* Game );
    BOOLEAN BatchFile_IsBatchedFile(FILE_LIST_ENTRY* File);
    UINT64 BatchFile_GetBatchSize();
    VOID BatchFile_SaveBatchFile(PUSH_GAME* Game);
    VOID BatchFile_AddItem(FILE_LIST_ENTRY* File);
    VOID BatchFile_RemoveItem(FILE_LIST_ENTRY* File);
    FILE_LIST BatchFile_GetBatchList();
    VOID BatchFile_Initialize(PUSH_GAME* Game);

#endif //BATCH_H