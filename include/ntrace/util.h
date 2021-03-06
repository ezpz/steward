/**
 * @file util.h
 * @note Do not define any structures here. Use ntrace.h instead
 */

#ifndef NTRACE_UTIL__H__
#define NTRACE_UTIL__H__

#include <stdio.h>
#include <ntrace.h>

/* Process control */
proc_t *    attach_to_process ();


/* initialization and cleanup functions */
void        do_initialize (proc_t *);
void        do_cleanup (proc_t *);


/* General purpose utilities */
pid_t       gettid ();
void        save_proc (proc_t *);
proc_t      *load_proc (const char *);
void        exit_fun ();
void        signal_handler (int);
key_t       hash_key (proc_t *, int);
void        associate_fd (proc_t *, int, fd_t);
void        release_fd (proc_t *, int);
const char  *call2str (call_t);
const char  *type2str (fd_t);
const char  *flow2str (flow_t *);

#endif /*NTRACE_UTIL__H__*/
