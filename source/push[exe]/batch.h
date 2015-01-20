#ifndef BATCH_H
#define BATCH_H


#include "game.h"


class BfBatchFile{
    FILE_LIST FileList;
    WCHAR* BatchFileName;
    UINT64 BatchSize;

public:
    BfBatchFile( PushGame* Game );
    ~BfBatchFile();
    BOOLEAN IsBatchedFile( FILE_LIST_ENTRY* File );
    UINT64 GetBatchSize();
    VOID SaveBatchFile();
    VOID AddItem( FILE_LIST_ENTRY* File );
    VOID RemoveItem( FILE_LIST_ENTRY* File );
    FILE_LIST GetBatchList();
};

#endif //BATCH_H