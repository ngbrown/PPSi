
	/*
	 * A stupid crt0.S for "freestanding" stuff on gnu/linux
	 *
	 * Copyright (C) 2011 CERN (www.cern.ch)
	 * Author: Alessandro Rubini
	 *
	 * Released according to the GNU LGPL, version 2.1 or any later version.
	 */
	.section .boot, "ax"
	.extern ppsi_main

	call ppsi_clear_bss /* In C, lazy me */

	/* inspired from uClib */
	
	popl %ecx	/* Store argc into %ecx */
	movl %esp,%ebx  /* Store argv into ebx */
	movl %esp,%eax  /* Store argv into eax as well*/
	movl %ecx,%edx	/* Stick argc into %edx so we can do some math in a sec */
	leal 4(%eax,%edx,4),%eax
	pushl %eax	/* Environment pointer */
	pushl %ebx	/* Argument pointer */
	pushl %ecx	/* And the argument count */
	
	call ppsi_main
	.end