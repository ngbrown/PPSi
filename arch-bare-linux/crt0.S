
	/*
	 * A stupid crt0.S for "freestanding" stuff on gnu/linux
	 * Alessandro Rubini for CERN, 2011 -- GNU GPL v2 or later
	 */
	.section .boot, "ax"
	.extern ppsi_main

	call ppsi_clear_bss /* In C, lazy me */
	call ppsi_main
	.end