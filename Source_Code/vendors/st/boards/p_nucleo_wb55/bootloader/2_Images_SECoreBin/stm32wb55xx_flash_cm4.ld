/**
*****************************************************************************
**
**  File        : stm32wb55xx_flash_cm4.ld
**
**  Abstract    : System Workbench Minimal System calls file
**
** 		          For more information about which c-functions
**                need which of these lowlevel functions
**                please consult the Newlib libc-manual
**
**  Environment : System Workbench for MCU
**
**  Distribution: The file is distributed “as is,” without any warranty
**                of any kind.
**
*****************************************************************************
**
** <h2><center>&copy; COPYRIGHT(c) 2019 Ac6</center></h2>
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**   1. Redistributions of source code must retain the above copyright notice,
**      this list of conditions and the following disclaimer.
**   2. Redistributions in binary form must reproduce the above copyright notice,
**      this list of conditions and the following disclaimer in the documentation
**      and/or other materials provided with the distribution.
**   3. Neither the name of Ac6 nor the names of its contributors
**      may be used to endorse or promote products derived from this software
**      without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
*****************************************************************************
*/


/* Entry Point */
ENTRY(SE_CallGate)

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0;      /* required amount of heap  */
_Min_Stack_Size = 0;     /* specific stack provided inside secured area */

INCLUDE mapping_fwimg.ld
INCLUDE mapping_sbsfu.ld

/* Define output sections */
SECTIONS
{
  .SE_CallGate_Code : { KEEP(*(.SE_CallGate_Code)) } > SE_Entry_Secure_ROM_Region

  .SE_Key_Data : { KEEP(*(.SE_Key_Data)) } > SE_Key_region_ROM
  .SE_Startup_Code : { 
	KEEP ( *se_startup.o (.text .text*))
	KEEP ( *se_startup.obj (.text .text*))
	KEEP ( *se_startup.c.obj (.text .text*))
	KEEP ( *se_startup.c.o (.text .text*))    
  } > SE_Startup_region_ROM
  .text :
  {
    . = ALIGN(8);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(8);
    _etext = .;        /* define a global symbols at end of code */
  } >SE_ROM_region

  .isr_vector :
  {
    . = ALIGN(0x200);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(8);
  } >SE_ROM_region

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(8);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(8);
  } >SE_ROM_region

  .ARM.extab   : 
  { 
  . = ALIGN(8);
  *(.ARM.extab* .gnu.linkonce.armextab.*)
  . = ALIGN(8);
  } >SE_ROM_region
  .ARM : {
	. = ALIGN(8);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
	. = ALIGN(8);
  } >SE_ROM_region

  .preinit_array     :
  {
	. = ALIGN(8);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
	. = ALIGN(8);
  } >SE_ROM_region
  
  .init_array :
  {
	. = ALIGN(8);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
	. = ALIGN(8);
  } >SE_ROM_region
  .fini_array :
  {
	. = ALIGN(8);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
	. = ALIGN(8);
  } >SE_ROM_region

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : 
  {
    . = ALIGN(8);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(8);
    _edata = .;        /* define a global symbol at data end */
  } >SE_SRAM1_region AT>SE_ROM_region 

  
  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >SE_SRAM1_region

  BOOTINFO_DATA : 
  {
    . = ALIGN(8);
    _sBOOTINFO_DATA = .;        /* create a global symbol at BOOTINFO_DATA start */
    *(BOOTINFO_DATA)          /* BOOTINFO_DATA* sections */
    *(BOOTINFO_DATA*)
    . = ALIGN(8);
    _eBOOTINFO_DATA = .;        /* define a global symbol at BOOTINFO_DATA end */
  
  } >SE_SRAM1_region AT> SE_ROM_region
  /* User_heap_stack section, used to check that there is enough RAM left */
   
  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }

  MAPPING_TABLE (NOLOAD) : { *(MAPPING_TABLE) } >RAM_SHARED
  MB_MEM1 (NOLOAD)       : { *(MB_MEM1) } >RAM_SHARED
  MB_MEM2 (NOLOAD)       : { *(MB_MEM2) } >RAM_SHARED  

}

