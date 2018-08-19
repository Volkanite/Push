#include <push.h>


struct atom_common_table_header
{
    UINT16 structuresize;
    UINT8  format_revision;
    UINT8  content_revision;                              
};

typedef struct atom_firmware_info_v3_1
{
    struct atom_common_table_header table_header;
    UINT32 firmware_revision;
    UINT32 bootup_sclk_in10khz;
    UINT32 bootup_mclk_in10khz;
}atom_firmware_info_v3_1;

/****************************************************************************/
// Structures used in Command.mtb 
/****************************************************************************/
typedef struct _ATOM_MASTER_LIST_OF_COMMAND_TABLES{
    WORD ASIC_Init;                              //Function Table, used by various SW components,latest version 1.1
    WORD GetDisplaySurfaceSize;                  //Atomic Table,  Used by Bios when enabling HW ICON
    WORD ASIC_RegistersInit;                     //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
    WORD VRAM_BlockVenderDetection;              //Atomic Table,  used only by Bios
    WORD DIGxEncoderControl;                                         //Only used by Bios
    WORD MemoryControllerInit;                   //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
    WORD EnableCRTCMemReq;                       //Function Table,directly used by various SW components,latest version 2.1
    WORD MemoryParamAdjust;                                          //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock if needed
    WORD DVOEncoderControl;                      //Function Table,directly used by various SW components,latest version 1.2
    WORD GPIOPinControl;                                                 //Atomic Table,  only used by Bios
    WORD SetEngineClock;                         //Function Table,directly used by various SW components,latest version 1.1
    WORD SetMemoryClock;                         //Function Table,directly used by various SW components,latest version 1.1
    WORD SetPixelClock;                          //Function Table,directly used by various SW components,latest version 1.2  
    WORD EnableDispPowerGating;                  //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
    WORD ResetMemoryDLL;                         //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
    WORD ResetMemoryDevice;                      //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
    WORD MemoryPLLInit;                          //Atomic Table,  used only by Bios
    WORD AdjustDisplayPll;                                           //Atomic Table,  used by various SW componentes. 
    WORD AdjustMemoryController;                 //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock                
    WORD EnableASIC_StaticPwrMgt;                //Atomic Table,  only used by Bios
    WORD SetUniphyInstance;                      //Atomic Table,  only used by Bios   
    WORD DAC_LoadDetection;                      //Atomic Table,  directly used by various SW components,latest version 1.2  
    WORD LVTMAEncoderControl;                    //Atomic Table,directly used by various SW components,latest version 1.3
    WORD HW_Misc_Operation;                      //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD DAC1EncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1  
    WORD DAC2EncoderControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD DVOOutputControl;                       //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD CV1OutputControl;                       //Atomic Table,  Atomic Table,  Obsolete from Ry6xx, use DAC2 Output instead 
    WORD GetConditionalGoldenSetting;            //Only used by Bios
    WORD TVEncoderControl;                       //Function Table,directly used by various SW components,latest version 1.1
    WORD PatchMCSetting;                         //only used by BIOS
    WORD MC_SEQ_Control;                         //only used by BIOS
    WORD Gfx_Harvesting;                         //Atomic Table,  Obsolete from Ry6xx, Now only used by BIOS for GFX harvesting
    WORD EnableScaler;                           //Atomic Table,  used only by Bios
    WORD BlankCRTC;                              //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD EnableCRTC;                             //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD GetPixelClock;                          //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD EnableVGA_Render;                       //Function Table,directly used by various SW components,latest version 1.1
    WORD GetSCLKOverMCLKRatio;                   //Atomic Table,  only used by Bios
    WORD SetCRTC_Timing;                         //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD SetCRTC_OverScan;                       //Atomic Table,  used by various SW components,latest version 1.1 
    WORD SetCRTC_Replication;                    //Atomic Table,  used only by Bios
    WORD SelectCRTC_Source;                      //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD EnableGraphSurfaces;                    //Atomic Table,  used only by Bios
    WORD UpdateCRTC_DoubleBufferRegisters;           //Atomic Table,  used only by Bios
    WORD LUT_AutoFill;                           //Atomic Table,  only used by Bios
    WORD EnableHW_IconCursor;                    //Atomic Table,  only used by Bios
    WORD GetMemoryClock;                         //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD GetEngineClock;                         //Atomic Table,  directly used by various SW components,latest version 1.1 
    WORD SetCRTC_UsingDTDTiming;                 //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD ExternalEncoderControl;                 //Atomic Table,  directly used by various SW components,latest version 2.1
    WORD LVTMAOutputControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD VRAM_BlockDetectionByStrap;             //Atomic Table,  used only by Bios
    WORD MemoryCleanUp;                          //Atomic Table,  only used by Bios    
    WORD ProcessI2cChannelTransaction;           //Function Table,only used by Bios
    WORD WriteOneByteToHWAssistedI2C;            //Function Table,indirectly used by various SW components 
    WORD ReadHWAssistedI2CStatus;                //Atomic Table,  indirectly used by various SW components
    WORD SpeedFanControl;                        //Function Table,indirectly used by various SW components,called from ASIC_Init
    WORD PowerConnectorDetection;                //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD MC_Synchronization;                     //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
    WORD ComputeMemoryEnginePLL;                 //Atomic Table,  indirectly used by various SW components,called from SetMemory/EngineClock
    WORD MemoryRefreshConversion;                //Atomic Table,  indirectly used by various SW components,called from SetMemory or SetEngineClock
    WORD VRAM_GetCurrentInfoBlock;               //Atomic Table,  used only by Bios
    WORD DynamicMemorySettings;                  //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
    WORD MemoryTraining;                         //Atomic Table,  used only by Bios
    WORD EnableSpreadSpectrumOnPPLL;             //Atomic Table,  directly used by various SW components,latest version 1.2
    WORD TMDSAOutputControl;                     //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD SetVoltage;                             //Function Table,directly and/or indirectly used by various SW components,latest version 1.1
    WORD DAC1OutputControl;                      //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD DAC2OutputControl;                      //Atomic Table,  directly used by various SW components,latest version 1.1
    WORD ComputeMemoryClockParam;                //Function Table,only used by Bios, obsolete soon.Switch to use "ReadEDIDFromHWAssistedI2C"
    WORD ClockSource;                            //Atomic Table,  indirectly used by various SW components,called from ASIC_Init
    WORD MemoryDeviceInit;                       //Atomic Table,  indirectly used by various SW components,called from SetMemoryClock
    WORD GetDispObjectInfo;                      //Atomic Table,  indirectly used by various SW components,called from EnableVGARender
    WORD DIG1EncoderControl;                     //Atomic Table,directly used by various SW components,latest version 1.1
    WORD DIG2EncoderControl;                     //Atomic Table,directly used by various SW components,latest version 1.1
    WORD DIG1TransmitterControl;                 //Atomic Table,directly used by various SW components,latest version 1.1
    WORD DIG2TransmitterControl;                   //Atomic Table,directly used by various SW components,latest version 1.1 
    WORD ProcessAuxChannelTransaction;                   //Function Table,only used by Bios
    WORD DPEncoderService;                                           //Function Table,only used by Bios
    WORD GetVoltageInfo;                         //Function Table,only used by Bios since SI
}ATOM_MASTER_LIST_OF_COMMAND_TABLES;

typedef struct _ATOM_MASTER_LIST_OF_DATA_TABLES
{
    WORD   UtilityPipeLine;         // Offest for the utility to get parser info,Don't change this position!
    WORD   MultimediaCapabilityInfo; // Only used by MM Lib,latest version 1.1, not configuable from Bios, need to include the table to build Bios
    WORD   MultimediaConfigInfo;     // Only used by MM Lib,latest version 2.1, not configuable from Bios, need to include the table to build Bios
    WORD   StandardVESA_Timing;      // Only used by Bios
    WORD   FirmwareInfo;             // Shared by various SW components,latest version 1.4
    WORD   PaletteData;              // Only used by BIOS
    WORD   LCD_Info;                 // Shared by various SW components,latest version 1.3, was called LVDS_Info
    WORD   DIGTransmitterInfo;       // Internal used by VBIOS only version 3.1
    WORD   AnalogTV_Info;            // Shared by various SW components,latest version 1.1
    WORD   SupportedDevicesInfo;     // Will be obsolete from R600
    WORD   GPIO_I2C_Info;            // Shared by various SW components,latest version 1.2 will be used from R600
    WORD   VRAM_UsageByFirmware;     // Shared by various SW components,latest version 1.3 will be used from R600
    WORD   GPIO_Pin_LUT;             // Shared by various SW components,latest version 1.1
    WORD   VESA_ToInternalModeLUT;   // Only used by Bios
    WORD   ComponentVideoInfo;       // Shared by various SW components,latest version 2.1 will be used from R600
    WORD   PowerPlayInfo;            // Shared by various SW components,latest version 2.1,new design from R600
    WORD   CompassionateData;        // Will be obsolete from R600
    WORD   SaveRestoreInfo;          // Only used by Bios
    WORD   PPLL_SS_Info;             // Shared by various SW components,latest version 1.2, used to call SS_Info, change to new name because of int ASIC SS info
    WORD   OemInfo;                  // Defined and used by external SW, should be obsolete soon
    WORD   XTMDS_Info;               // Will be obsolete from R600
    WORD   MclkSS_Info;              // Shared by various SW components,latest version 1.1, only enabled when ext SS chip is used
    WORD   Object_Header;            // Shared by various SW components,latest version 1.1
    WORD   IndirectIOAccess;         // Only used by Bios,this table position can't change at all!!
    WORD   MC_InitParameter;         // Only used by command table
    WORD   ASIC_VDDC_Info;                      // Will be obsolete from R600
    WORD   ASIC_InternalSS_Info;            // New tabel name from R600, used to be called "ASIC_MVDDC_Info"
    WORD   TV_VideoMode;                            // Only used by command table
    WORD   VRAM_Info;                               // Only used by command table, latest version 1.3
    WORD   MemoryTrainingInfo;              // Used for VBIOS and Diag utility for memory training purpose since R600. the new table rev start from 2.1
    WORD   IntegratedSystemInfo;            // Shared by various SW components
    WORD   ASIC_ProfilingInfo;              // New table name from R600, used to be called "ASIC_VDDCI_Info" for pre-R600
    WORD   VoltageObjectInfo;               // Shared by various SW components, latest version 1.1
    WORD    PowerSourceInfo;                    // Shared by various SW components, latest versoin 1.1
}ATOM_MASTER_LIST_OF_DATA_TABLES;

#define GetIndexIntoMasterTable(MasterOrData, FieldName) (((char*)(&((ATOM_MASTER_LIST_OF_##MasterOrData##_TABLES*)0)->FieldName)-(char*)0)/sizeof(WORD))

int amdgpu_atom_execute_table(struct atom_context *ctx, int index, UINT32 * params);
BOOLEAN amdgpu_atom_parse_data_header(struct atom_context *ctx, int index,
    UINT16 * size, UINT8 * frev, UINT8 * crev,
    UINT16 * data_start);
#define uint8_t UINT8
#define uint16_t UINT16
#define uint32_t UINT32
#define uint64_t UINT64

struct atom_context {
    struct card_info *card;
    //  struct mutex mutex;
    uint8_t  *bios;
    uint32_t cmd_table, data_table;
    uint16_t *iio;

    uint16_t data_block;
    uint32_t fb_base;
    uint32_t divmul[2];
    uint16_t io_attr;
    uint16_t reg_block;
    uint8_t shift;
    int cs_equal, cs_above;
    int io_mode;
    uint32_t *scratch;
    int scratch_size_bytes;
};


typedef struct GET_ENGINE_CLOCK_PS_ALLOCATION
{
    ULONG ulReturnEngineClock;
}GET_ENGINE_CLOCK_PS_ALLOCATION;

extern struct atom_context ctxlol;


UINT32 AtomBios_GetEngineClock()
{
    atom_firmware_info_v3_1 *args;

    int index = (int)GetIndexIntoMasterTable(DATA, FirmwareInfo);
    UINT16 data_offset;
    UINT8 frev, crev;

    if (!amdgpu_atom_parse_data_header(&ctxlol, index, NULL, &frev, &crev, &data_offset))
        return 0;

    args = (atom_firmware_info_v3_1 *)(ctxlol.bios + data_offset);

    return args->bootup_sclk_in10khz / 100;
}


UINT32 AtomBios_GetMemoryClock()
{
    atom_firmware_info_v3_1 *args;

    int index = (int)GetIndexIntoMasterTable(DATA, FirmwareInfo);
    UINT16 data_offset;
    UINT8 frev, crev;

    if (!amdgpu_atom_parse_data_header(&ctxlol, index, NULL, &frev, &crev, &data_offset))
        return 0;

    args = (atom_firmware_info_v3_1 *)(ctxlol.bios + data_offset);

    return args->bootup_mclk_in10khz / 100;
}


/*UINT32 radeon_atom_get_engine_clock()
{
    GET_ENGINE_CLOCK_PS_ALLOCATION args;
    int index = GetIndexIntoMasterTable(COMMAND, GetEngineClock);

    amdgpu_atom_execute_table(&ctxlol, index, (UINT32 *)&args);
    //return le32_to_cpu(args.ulReturnEngineClock);
    return args.ulReturnEngineClock;
}*/