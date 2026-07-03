/* 💡 [완전 종결] QEMU requested regions overlap 억까 원천 차단 링커 스크립트 */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 256K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;        /* 실행 코드 및 읽기 전용 데이터가 끝난 최종 위치 */
  } >FLASH

  /* 💡 [핵심 교정] 링커가 헛짓거리 못하도록 코드 끝단 위치를 강제로 정렬하여 바인딩 */
  . = ALIGN(4);
  _sidata = .;

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH     /* RAM에 배치하되, FLASH 상에서는 _sidata 뒷자리부터 순서대로 적재 */

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM AT> RAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + 0x2000;
    . = . + 0x1000;
    . = ALIGN(8);
  } >RAM AT> RAM
}