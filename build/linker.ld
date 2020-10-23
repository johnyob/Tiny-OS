OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x80000000, LENGTH = 0x8000000
}

SECTIONS
{

    .text :
    {
        PROVIDE(__TEXT_START = .);
        *(.text)
        . =  ALIGN(0x1000);
        PROVIDE(__TEXT_END = .);
    } >ram

	.rodata :
    {
        PROVIDE(__RODATA_START = .);
        *(.rodata)
        . =  ALIGN(0x1000);
        PROVIDE(__RODATA_END = .);
    } >ram

    .data :
    {
	    PROVIDE(__DATA_START = .);
	    *(.data)
	    . =  ALIGN(0x1000);
	    PROVIDE(__DATA_END = .);
    } >ram

    .bss :
    {
        PROVIDE(__BSS_START = .);
        *(.bss)
        . =  ALIGN(0x1000);
        PROVIDE(__BSS_END = .);
    } >ram

    .stack :
    {
        PROVIDE(__STACK_START = .);
        *(.stack)
        . =  ALIGN(0x1000);
        PROVIDE(__STACK_END = .);
    } >ram


    PROVIDE(__MEMORY_START = ORIGIN(ram));
    PROVIDE(__MEMORY_END = ORIGIN(ram) + LENGTH(ram));

    PROVIDE(__HEAP_START = __STACK_END);
    PROVIDE(__HEAP_SIZE = __MEMORY_END - __HEAP_START);
}
