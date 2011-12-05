
	/*
	 * A stupid crt0.S for "freestanding" stuff on gnu/linux
	 * Alessandro Rubini for CERN, 2011 -- GNU GPL v2 or later
	 */
	.section .boot, "ax"
	.extern pproto_main

	call pproto_clear_bss /* In C, lazy me */
	call pproto_main
	.end