
	/*
	 * A stupid crt0.S for "freestanding" stuff on gnu/linux
	 * Alessandro Rubini for CERN, 2011 -- GNU GPL v2 or later
	 */
	.section .boot, "ax"
	.extern ppsi_main

        call ppsi_clear_bss /* In C, lazy me */

	/* inspired from uClib */
	
	/* Pop argc and place it in the first parameter-passing register. */
	popq %rdi 
	/* Place argv in the second parameter-passing register. */
	movq %rsp, %rsi
	/* Align the stack at a 16-byte boundary. */
	andq $~15, %rsp

	call ppsi_main
	.end