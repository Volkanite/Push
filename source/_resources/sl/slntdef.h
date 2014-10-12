//
// Structure to represent a system wide processor number. It contains a
// group number and relative processor number within the group.
//

typedef struct _PROCESSOR_NUMBER {
    UINT16 Group;
    UINT8 Number;
    UINT8 Reserved;
} PROCESSOR_NUMBER;