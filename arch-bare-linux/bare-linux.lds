OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_pptp_start)

SECTIONS
{
	. = 0x10000000; /* A random address, non-standard by choice*/
	.text : {
		_pptp_start = .;
		*(.boot)
		*(.text)
	}
	.rodata : { *(.rodata) }
	.data : { *(.data) }

	.bss : {
		. = ALIGN(16);
		__bss_start = .;
		*(.bss);
		. = ALIGN(16);
		__bss_end = .;
	}
}
