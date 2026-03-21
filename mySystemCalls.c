#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include "421structs.h"

mail_app_context_421_t application;

/**
 * Syscall to create a mailbox.
 * Parameters:
 * - bid:     the Box ID.
 *
 * Return Values:
 *
 * - EPERM:   the supplied Box ID is less than zero OR the given Box ID is already in use.
 * - EIO:     memory was unable to be allocated for the new mailbox.
 * - 0:       the mailbox was successfully added to the application.
 */
SYSCALL_DEFINE1(application_create_mailbox, int, bid) {
  return EPERM;
}

/**
 * Syscall to destroy a mailbox.
 * Parameters:
 * - bid:     the Box ID.
 *
 * Return Values:
 *
 * - ENOENT:  the supplied Box ID is less than zero OR there is no mailbox with the given Box ID.
 * - EPERM:   assuming a mailbox was found, the mailbox still has messages within it.
 * - 0:       the mailbox was successfully added to the application.
 */
SYSCALL_DEFINE1(application_destroy_mailbox, int, bid) {
  return EPERM;
}

/**
 * Syscall to add a message to a given mailbox. 
 * Parameters:
 * - bid:     the Box ID to add the given message to.
 * - message: the ENTIRE message structure, as a void pointer, from the user-space caller; to be added.
 *
 * Return Values:
 *
 * - ENOENT:  the supplied Box ID is less than zero OR there is no mailbox with the given Box ID.
 * - EACCES:  the kernel is unable to access the user-space memory.
 * - ENOMEM:  the kernel is unable to allocate space to copy the message into.
 * - EPERM:   assuming a mailbox was found, the mailbox already has a message with the messages' ID (mid) 
 * - EIO:     the message has no data associated with it.
 * - 0:       the new message was successfully added to the desired mailbox.
 */
SYSCALL_DEFINE2(application_add_message, int, bid, void* __user, message) {
  return EPERM;
}


/**
 * Syscall to destroy a mailbox.
 * Parameters:
 * - dest: the user-space pointer to copy the found message into. 
 *
 * Return Values:
 *
 * - ENOENT:  there are no mailboxes in the application context OR there are no messages within any box.
 * - EPERM:   the kernel was unable to copy the message to user-space.
 * - 0:       a message was found and copied to user-space.
 */
SYSCALL_DEFINE1(application_get_message, void* __user, dest) {
  return EPERM;
}
