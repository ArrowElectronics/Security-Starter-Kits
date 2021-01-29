/*
*****************************************************************************
**
**  File        : mapping_sbsfu.ld
**
**  Abstract    : Code/Ram sections definition
**                Common to SECoreBin, SBSFU and UserApp projects
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
**  (c)Copyright Ac6.
**  You may use this file as-is or modify it according to the needs of your
**  project. Distribution of this file (unmodified or modified) is not
**  permitted. Ac6 permit registered System Workbench for MCU users the
**  rights to distribute the assembled, compiled & linked contents of this
**  file as part of an application binary file, provided that it is built
**  using the System Workbench for MCU toolchain.
**
*****************************************************************************
*/

/* SE Code region */
VECTOR_SIZE = 0x200;
__ICFEDIT_SE_Code_region_ROM_start__       = 0x08000000 + VECTOR_SIZE;
__ICFEDIT_SE_CallGate_region_ROM_start__   = __ICFEDIT_SE_Code_region_ROM_start__ + 0x4;
__ICFEDIT_SE_CallGate_region_ROM_end__     = __ICFEDIT_SE_Code_region_ROM_start__ + 0x1FF;

/* SE key region */
__ICFEDIT_SE_Key_region_ROM_start__        = __ICFEDIT_SE_CallGate_region_ROM_end__ + 0x1;
__ICFEDIT_SE_Key_region_ROM_end__          = __ICFEDIT_SE_Key_region_ROM_start__ + 0xFF;

/* SE Startup */
__ICFEDIT_SE_Startup_region_ROM_start__    = __ICFEDIT_SE_Key_region_ROM_end__ + 0x1;
__ICFEDIT_SE_Code_nokey_region_ROM_start__ = __ICFEDIT_SE_Startup_region_ROM_start__ + 0x100;
__ICFEDIT_SE_Code_region_ROM_end__         = __ICFEDIT_SE_Startup_region_ROM_start__ + 0x2FFF;

/* SE IF ROM */
__ICFEDIT_SE_IF_region_ROM_start__         = __ICFEDIT_SE_Code_region_ROM_end__ + 1;
__ICFEDIT_SE_IF_region_ROM_end__           = __ICFEDIT_SE_IF_region_ROM_start__ + 0x5FF;

/* SBSFU Code region */
__ICFEDIT_SB_region_ROM_start__            = __ICFEDIT_SE_IF_region_ROM_end__ + 0x1;
__ICFEDIT_SB_region_ROM_end__              = __ICFEDIT_SB_region_ROM_start__ + 0x82FF;

/* LOADER code region : alignment on 0x200 (VECTOR_SIZE) for LOADER vector table */
__ICFEDIT_LOADER_region_ROM_start__        = ((__ICFEDIT_SB_region_ROM_end__ / VECTOR_SIZE) + 1) * VECTOR_SIZE;
__ICFEDIT_LOADER_region_ROM_end__          = 0x0800FFFF;

SE_Entry_Secure_ROM_Region_Length = __ICFEDIT_SE_CallGate_region_ROM_end__ - __ICFEDIT_SE_CallGate_region_ROM_start__ + 1;
SE_Key_region_ROM_Length          = __ICFEDIT_SE_Key_region_ROM_end__ - __ICFEDIT_SE_Key_region_ROM_start__ + 1;
SE_Startup_region_ROM_Length      = __ICFEDIT_SE_Code_nokey_region_ROM_start__ - __ICFEDIT_SE_Startup_region_ROM_start__ ;
SE_ROM_region_Length              = __ICFEDIT_SE_Code_region_ROM_end__ - __ICFEDIT_SE_Code_nokey_region_ROM_start__ + 1;
SE_IF_region_ROM_Length           = __ICFEDIT_SE_IF_region_ROM_end__ - __ICFEDIT_SE_IF_region_ROM_start__ + 1;
SB_ROM_region_Length              = __ICFEDIT_SB_region_ROM_end__ - __ICFEDIT_SB_region_ROM_start__ + 1;
LOADER_ROM_region_Length          = __ICFEDIT_LOADER_region_ROM_end__ - __ICFEDIT_LOADER_region_ROM_start__ + 1;
FW_0_region_length				  = __ICFEDIT_region_SLOT_0_end__ - __ICFEDIT_region_SLOT_0_start__ + 1;

/* RAM section */
/* SE stack is placed 1st in RAM, stack overflow does not write on other RAM area */
__ICFEDIT_SE_region_SRAM1_start__     = 0x20000000;
__ICFEDIT_SE_region_SRAM1_stack_top__ = 0x20000400;
__ICFEDIT_SE_region_SRAM1_end__       = 0x20000FFF;

/* LOADER communication region when bypass mode activated */
__ICFEDIT_LOADER_COM_region_SRAM1_start__ = __ICFEDIT_SE_region_SRAM1_end__ + 1;
__ICFEDIT_LOADER_COM_region_SRAM1_end__   = __ICFEDIT_LOADER_COM_region_SRAM1_start__ + 0x0F;

/* LOADER RAM1 region */
__ICFEDIT_LOADER_region_SRAM1_start__ = __ICFEDIT_LOADER_COM_region_SRAM1_end__ + 1;
__ICFEDIT_LOADER_region_SRAM1_end__   = __ICFEDIT_LOADER_region_SRAM1_start__ + 0x1FFF;

/* SBSFU RAM1 region */
__ICFEDIT_SB_region_SRAM1_start__     = __ICFEDIT_LOADER_region_SRAM1_end__ + 1;
__ICFEDIT_SB_region_SRAM1_end__       = 0x2002FFFF;

SE_SRAM1_region_Length = __ICFEDIT_SE_region_SRAM1_end__ - __ICFEDIT_SE_region_SRAM1_stack_top__ + 1;
LOADER_SRAM1_region_Length = __ICFEDIT_LOADER_region_SRAM1_end__ - __ICFEDIT_LOADER_region_SRAM1_start__ + 1;
SB_SRAM1_region_Length = __ICFEDIT_SB_region_SRAM1_end__ - __ICFEDIT_SB_region_SRAM1_start__ + 1;

MEMORY
{
 SE_Entry_Secure_ROM_Region (rx)     : ORIGIN = __ICFEDIT_SE_CallGate_region_ROM_start__, LENGTH = SE_Entry_Secure_ROM_Region_Length
 SE_Key_region_ROM (rx)              : ORIGIN = __ICFEDIT_SE_Key_region_ROM_start__, LENGTH = SE_Key_region_ROM_Length
 SE_Startup_region_ROM (rx)          : ORIGIN = __ICFEDIT_SE_Startup_region_ROM_start__, LENGTH = SE_Startup_region_ROM_Length
 SE_ROM_region (rx)                  : ORIGIN = __ICFEDIT_SE_Code_nokey_region_ROM_start__, LENGTH = SE_ROM_region_Length
 SE_IF_region_ROM (rx)               : ORIGIN = __ICFEDIT_SE_IF_region_ROM_start__, LENGTH = SE_IF_region_ROM_Length
 SB_ROM_region (rx)                  : ORIGIN = __ICFEDIT_SB_region_ROM_start__, LENGTH = SB_ROM_region_Length
 LOADER_ROM_region (rx)              : ORIGIN = __ICFEDIT_LOADER_region_ROM_start__, LENGTH = LOADER_ROM_region_Length
 FW_0_region (rx)                    : ORIGIN = __ICFEDIT_region_SLOT_0_start__, LENGTH = FW_0_region_length
 SE_SRAM1_region (xrw)               : ORIGIN = __ICFEDIT_SE_region_SRAM1_stack_top__, LENGTH = SE_SRAM1_region_Length
 LOADER_SRAM1_region (xrw)           : ORIGIN = __ICFEDIT_LOADER_region_SRAM1_start__, LENGTH = LOADER_SRAM1_region_Length
 SB_SRAM1_region (xrw)               : ORIGIN = __ICFEDIT_SB_region_SRAM1_start__, LENGTH = SB_SRAM1_region_Length
 RAM_SHARED (xrw)                     : ORIGIN = 0x20030000, LENGTH = 10K
}

