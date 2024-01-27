/*
 * bsp-start.h -- BSP startcode definitions
 *
 * Copyright (c) 1998 Cygnus Support
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */
#ifndef __BSP_START_H__
#define __BSP_START_H__ 1

#include <bsp-trap.h>
#include <bsp/bsp.h>
#include <bsp_if.h>
#include <bsp/cpu.h>

#ifdef ADD_DTORS
extern void          (*__do_global_dtors)(void);
#endif /* ADD_DTORS */
extern void          _hardware_init_hook(void);
extern void          _software_init_hook(void);
extern void          _bsp_initvectors(void);

extern int           main(int argc, char *argv[], char *envp[]);

extern unsigned long _rom_data_start;
extern unsigned long _ram_data_start;
extern unsigned long _ram_data_end;
extern unsigned long _bss_start;
extern unsigned long _bss_end;

extern void          *_bsp_ram_info_ptr;
extern unsigned long *_initial_stack;

/*
 * Shared app and bsp startup code.
 * This gets run before any app or bsp specific code gets run
 */
static inline void c_crt0_begin(void)
{
    /*
     * Copy initialized data from ROM to RAM if necessary
     */
    if (&_ram_data_start != &_rom_data_start)
    {
        unsigned long *ram_data = &_ram_data_start;
        unsigned long *rom_data = &_rom_data_start;
        while (ram_data < &_ram_data_end)
            *(ram_data++) = *(rom_data++);
    }

    /*
     * Zero initialize the bss section
     */
    if (&_bss_start != &_bss_end)
    {
        unsigned long *bss_data = &_bss_start;
        while (bss_data < &_bss_end)
            *(bss_data++) = 0;
    }

    /*
     * Call _software_init_hook() and _hardware_init_hook() if they exist
     */
    if (&_hardware_init_hook != 0) _hardware_init_hook();
    if (&_software_init_hook != 0) _software_init_hook();

    /*
     * put __do_global_dtors in the atexit list so the destructors get run
     */
#ifdef ADD_DTORS
    atexit(__do_global_dtors);
#endif /* ADD_DTORS */
}

/*
 * Shared app and bsp startup code.
 * This gets run after all app or bsp specific code gets run
 */
static inline void c_crt0_end(void)
{
    static char *nullstring = "\0";

    /*
     * Enable interrupts
     */
   __cli();

    exit(main(0, &nullstring, &nullstring));

    /*
     * Never reached, but just in case...
     */
    while (1) ;
}

/*
 * Application startup code.  This is not run by the BSP start file
 */
static inline void c_crt0_app_specific()
{
    _bsp_trap(BSP_GET_SHARED, &bsp_shared_data);
}

/*
 * BSP startup code.  This is not run by the Application start file
 */
static inline void c_crt0_bsp_specific()
{
    /*
     * Finish setup of RAM description. Put a
     * physical pointer to the top of RAM in _bsp_ram_info_ptr.
     */
    _bsp_ram_info_ptr = (void *)&_initial_stack;
    
    /*
     * Do the generic BSP initialization
     */
    _bsp_init();
    
    /*
     * Let the BSP setup the interrupt vectors
     */
    _bsp_initvectors();
}
#endif /* __BSP_START_H__ */
