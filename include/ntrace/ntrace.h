/**
 * @file ntrace.h
 * @brief Global definitions and types
 */

#ifndef NTRACE_NTRACE__H__
#define NTRACE_NTRACE__H__

#include <sys/time.h>
#include <sys/types.h>
#include <inttypes.h> /* PRIu64 */
#include <stdint.h>
#include <stdio.h>

#include "steward.h"

#define FLOWS_MAX       255 /**< 255 fd per PID seems reasonable */
#define FNAME_SIZE      512 /**< length of file name buffers */
#define MARSHAL_FILE    "/tmp/steward/.%d.state" /**< proc_t marshalled data file */
#define MARSHAL_FMT     "%d %d %s\n" /**< marshalled output fmt */

typedef int key_t; /**< type used to index the flows array in a proc_t */

/**
 * @struct flow_t
 * Contains the details of a flow of traffic over a particular file descriptor.
 */
typedef struct {
    int     fd;  /**< file descriptor or socket */
    fd_t    type; /**< data association */
    int     active; /**< has this fd been used yet ? */
} flow_t;

/**
 * @struct proc_t
 * Entire set of details necessary to track a process through it's execution
 * and monitir its fd activity.
 */
typedef struct {

    pid_t       pid; /**< pid of the process */
    FILE        * log; /**< logfile */
    FILE        * trace; /**< debugging output */
    int         initialized; /**< true iff that data here has been set up */
    int         exited; /**< true iff exit has been called */
    flow_t      flows[FLOWS_MAX]; /**< open fds for this pid */

} proc_t;

#endif /*NTRACE_NTRACE__H__*/
