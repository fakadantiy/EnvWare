ENTRY(_start)

SECTIONS
{
  . = 1M;

  .text :
  {
    *(.multiboot)
    *(.text*)
  } :text

  .rodata :
  {
    *(.rodata*)
  } :rodata

  .data :
  {
    *(.data*)
  } :data

  .bss :
  {
    *(COMMON)
    *(.bss*)
  } :bss
}

PHDRS
{
  text   PT_LOAD FLAGS(5); /* R + X */
  rodata PT_LOAD FLAGS(4); /* R */
  data   PT_LOAD FLAGS(6); /* R + W */
  bss    PT_LOAD FLAGS(6); /* R + W */
}
