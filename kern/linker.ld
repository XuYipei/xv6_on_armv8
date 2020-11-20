OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS
{
    . = 0xFFFF000000080000;
    .text : {
        KEEP(*(.text.boot))
        *(.text .stub .gnu.linkonce.t.*)
    }
    PROVIDE(etext = .);
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);
    PROVIDE(data = .);
    .data : { *(.data) }
    PROVIDE(edata = .);
    .bss : { *(.bss) }
    PROVIDE(end = .);
    /DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
<<<<<<< HEAD
}
=======
}
<<<<<<< HEAD
=======
SECTIONS
{
    . = 0xFFFF000000080000;
    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }
    PROVIDE(etext = .);
    .rodata : { *(.rodata) }
    PROVIDE(data = .);
    .data : { *(.data) }
    PROVIDE(edata = .);
    .bss : { *(.bss) }
    PROVIDE(end = .);
}
>>>>>>> c46d735... blabla
>>>>>>> 4007444... lab5
=======
>>>>>>> f5c6bc2... lab5
