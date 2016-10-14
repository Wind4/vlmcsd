/*
 * <sys/capability.h>
 *
 * Copyright (C) 1997   Aleph One
 * Copyright (C) 1997-8,2008 Andrew G. Morgan <morgan@kernel.org>
 *
 * defunct POSIX.1e Standard: 25.2 Capabilities           <sys/capability.h>
 */

#ifndef _SYS_CAPABILITY_H
#define _SYS_CAPABILITY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file complements the kernel file by providing prototype
 * information for the user library.
 */

#include <sys/types.h>
#include <stdint.h>
#include <linux/types.h>

#ifndef __user
#define __user
#endif
#include <linux/capability.h>
#include <sys/xattr.h>
#include <linux/xattr.h>

/*
 * POSIX capability types
 */

/*
 * Opaque capability handle (defined internally by libcap)
 * internal capability representation
 */
typedef struct _cap_struct *cap_t;

/* "external" capability representation is a (void *) */

/*
 * This is the type used to identify capabilities
 */

typedef int cap_value_t;

/*
 * Set identifiers
 */
typedef enum {
    CAP_EFFECTIVE=0,                        /* Specifies the effective flag */
    CAP_PERMITTED=1,                        /* Specifies the permitted flag */
    CAP_INHERITABLE=2                     /* Specifies the inheritable flag */
} cap_flag_t;

/*
 * These are the states available to each capability
 */
typedef enum {
    CAP_CLEAR=0,                            /* The flag is cleared/disabled */
    CAP_SET=1                                    /* The flag is set/enabled */
} cap_flag_value_t;

/*
 * User-space capability manipulation routines
 */

/* libcap/cap_alloc.c */
extern cap_t   cap_dup(cap_t);
extern int     cap_free(void *);
extern cap_t   cap_init(void);

/* libcap/cap_flag.c */
extern int     cap_get_flag(cap_t, cap_value_t, cap_flag_t, cap_flag_value_t *);
extern int     cap_set_flag(cap_t, cap_flag_t, int, const cap_value_t *,
			    cap_flag_value_t);
extern int     cap_clear(cap_t);
extern int     cap_clear_flag(cap_t, cap_flag_t);

/* libcap/cap_file.c */
extern cap_t   cap_get_fd(int);
extern cap_t   cap_get_file(const char *);
extern int     cap_set_fd(int, cap_t);
extern int     cap_set_file(const char *, cap_t);

/* libcap/cap_proc.c */
extern cap_t   cap_get_proc(void);
extern cap_t   cap_get_pid(pid_t);
extern int     cap_set_proc(cap_t);

extern int     cap_get_bound(cap_value_t);
extern int     cap_drop_bound(cap_value_t);

#define CAP_IS_SUPPORTED(cap)  (cap_get_bound(cap) >= 0)

/* libcap/cap_extint.c */
extern ssize_t cap_size(cap_t);
extern ssize_t cap_copy_ext(void *, cap_t, ssize_t);
extern cap_t   cap_copy_int(const void *);

/* libcap/cap_text.c */
extern cap_t   cap_from_text(const char *);
extern char *  cap_to_text(cap_t, ssize_t *);
extern int     cap_from_name(const char *, cap_value_t *);
extern char *  cap_to_name(cap_value_t);

#define CAP_DIFFERS(result, flag)  (((result) & (1 << (flag))) != 0)
extern int     cap_compare(cap_t, cap_t);

/* system calls - look to libc for function to system call mapping */
extern int capset(cap_user_header_t header, cap_user_data_t data);
extern int capget(cap_user_header_t header, const cap_user_data_t data);

/* deprecated - use cap_get_pid() */
extern int capgetp(pid_t pid, cap_t cap_d);

/* not valid with filesystem capability support - use cap_set_proc() */
extern int capsetp(pid_t pid, cap_t cap_d);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_CAPABILITY_H */
