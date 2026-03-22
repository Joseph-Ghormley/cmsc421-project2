#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include "421structs.h"

/* Global application context.
 * application.mailboxes points to the head of the mailbox linked list.
 */
static mail_app_context_421_t application = {.mailboxes = NULL};

/*
 * mailbox_find
 *
 * Searches the mailbox linked list for a mailbox with the
 * given Box ID (bid).
 *
 * Because mailboxes are stored in sorted order by bid,
 * the search can stop early if we pass the desired bid.
 *
 * Parameters:
 *   bid - mailbox ID to search for
 *
 * Returns:
 *   pointer to the mailbox if found
 *   NULL if no mailbox with that ID exists
 */
static mailbox_421_t* mailbox_find(int bid){

mailbox_421_t* cur = application.mailboxes;

	/* Traverse the mailbox linked list */
	while (cur != NULL){

		/* If we find the mailbox, return it */
		if(cur->bid == bid)
			return cur;
		/* Because the list is sorted in ascending order,
		 * if the current bidis great then the target,
		 * the mailbox cannot exist later in the list.
		 */
		if(cur->bid > bid)
			return NULL;
		cur = cur->next;
	}
	/* Mailbox not found */
	return NULL;
}

/*
 * mail_insert_sorted
 *
 * Inserts mail box node into the mailbox linked list
 * while maintaining ascending order by mailbox ID (bid).
 * 
 * Uses a pointer- to pointer technique to simpleify insertion
 * at the head or middle of the list without speical cases.
 *
 * Parameters:
 * new_box _ newly allocated mailbox to insert
 */
static void mailbox_insert_sorted(mailbox_421_t* new_box)
{
	mailbox_421_t **link = &application.mailboxes;
	
	/* 
	 * Move through the list until we find the correct
	 * postion where new_box->bid should be inserted
	 */
	while (*link != NULL && (*link)->bid < new_box->bid)
		link = &((*link)->next);
	/*
	 * Insert the mailbox into the list
	 *
	 * new_box -> next now points to the current mode
	 * link now points to new_box
	 */
	 new_box->next = *link;
	*link = new_box;
}

/*
 * message_enqueue
 *
 * Adds a message to the End of the mailbox queue
 *
 * Mailbox operate as FIFO queue (First- In-First-Out),
 * so new messages must be appended to the tail.
 *
 * Parameters:
 *	box - mailbox receiving the message
 *	msg - message node to insert
 */

static void message_enqueue(mailbox_421_t* box, message_421_t* msg){

	message_421_t *cur;

	if (box == NULL || msg == NULL)
		return;

	/* Ensure the new message does not link with the old data */
	msg->next = NULL;

	/*
	 * If the mailbox currently has no messages,
	 * this message becomes the head of the queue.
	 */
	if (box->head == NULL){
		box->head =msg;
		return;
	}

	/*
	 * Otherwise, walk to the end of the queue
	 * and append the message
	 */
	cur = box->head;
	while (cur->next != NULL)
		cur = cur->next;
	cur->next =msg;
}

/*
 * message_dequeue
 *
 * Removes and returns the FIRST message in the mailbox queue.
 * 
 * Because the mailbox is FIFO, the oldest message
 * is alway stored at the head of the list.
 *
 * Parameters:
 *	box-mailbox to remove the message from
 *
 * Return :
 *	pointer to remove message
 *	NULL if mailbox has no messages
 *
 * NOTE:
 * The caller is responsible for freeing the message
 * with kfree() after use.
 */
static message_421_t* message_dequeue(mailbox_421_t* box){

	message_421_t *msg;

	if (box == NULL || box->head == NULL)
	return NULL;

	/* Save the head message */
	msg = box->head;

	/* Advance the head pointer */
	box->head = msg->next;

	/* Disconnect message from the list */
	msg->next = NULL;

	return msg;

}

/*
 * mailbox_find_best_nonempty
 *
 * Finds the mailbox with the highest priority
 * that currently contains at least one message.
 *
 * Mailbox priority rule:
 *	Lower mailbox ID (bid) = higher priority
 *
 * Because the mailbox list is sorted by bid,
 * the first mailbox with a message is the correct one.
 *
 * Returns:
 * 	pointer to the mailbox
 * 	NULL if no mailbox contains messages
 */
static mailbox_421_t* mailbox_find_best_nonempty(void){
	mailbox_421_t *cur = application.mailboxes;

	while (cur != NULL) {

        /* Return the first mailbox that has messages */
        if (cur->head != NULL)
            return cur;

        cur = cur->next;
    }

    return NULL;
}

/*
 * message_id_exists
 *
 * Checks whether a message with the given message ID (mid)
 * already exists inside a mailbox.
 *
 * Message IDs must be unique within a mailbox.
 *
 * Parameters:
 * 	box - mailbox to search
 * 	mid - message ID to check
 *
 * Returns:
 * 	1 if message ID already exists
 * 	0 otherwise
 */
static int message_id_exists(mailbox_421_t *box, int mid){
	message_421_t *cur;

	if (box == NULL)
		return 0;

    	cur = box->head;

    	/* Traverse message queue */
	while (cur != NULL) {

        	if (cur->mid == mid)
            		return 1;

        cur = cur->next;
    }

    return 0;
}

/*
 * mailbox_create
 *
 * Allocates a new mailbox and inserts it into
 * application mailbox list while preserving sort order.
 *
 * Rules enforced:
 * - box_id must be non_negative
 * - box_id must not already exist
 *
 * Return:
 * 	0	on success
 * 	-EPERM 	invalid ID or dupicate ID
 * 	-EIO	memory allocation failure
 */
int mailbox_create(int box_id){

	mailbox_421_t *new_box;

	/* Validate mailbox ID */
	if (box_id < 0) { return - EPERM; }

	/* Ensure mailbox does not already exist */
	if (mailbox_find(box_id) != NULL){ return -EPERM;}

	/* Allocate memory for mailbox structure */
	new_box = kmalloc(sizeof(*new_box), GFP_KERNEL);

	if (new_box == NULL){ return -EIO; }

	/* Initialize mailbox fields */
	new_box->bid = box_id;
	new_box->head = NULL;
	new_box->next = NULL;

	/* Insert mailbox list into sort by bid */
        mailbox_insert_sorted(new_box);

        return 0;
}

/*
 * mailbox_destroy
 *
 * Removes a mailbox from the system.
 *
 * Conditions:
 *  - mailbox must exist
 *  - mailbox must be empty
 *
 * Returns:
 *   0        success
 *  -ENOENT   mailbox not found or invalid ID
 *  -EPERM    mailbox still contains messages
 */

int mailbox_destroy(int box_id){

	mailbox_421_t *cur = application.mailboxes;
	mailbox_421_t *prev = NULL;

	if (box_id < 0)
		return -ENOENT;

	while (cur != NULL) {

		if (cur->bid == box_id) {

			/* Cannot destroy mailbox with messages */
			if (cur->head != NULL)
				return -EPERM;

			/* Remove mailbox from linked list */
			if (prev == NULL)
				application.mailboxes = cur->next;
			else
				prev->next = cur->next;

			/* Free mailbox memory */
			kfree(cur);

			return 0;
		}

		if (cur->bid > box_id)
		return -ENOENT;

		prev = cur;
		cur = cur->next;
	}

    return -ENOENT;
}

/*
 * mailbox_add
 *
 * Public wrapper used by system calls to add a message
 * to a mailbox queue.
 */
void mailbox_add(mailbox_421_t *box, message_421_t *message)
{
	message_enqueue(box, message);
}


/*
 * mailbox_get
 *
 * Public wrapper used by system calls to retrieve
 * the next message from a mailbox.
 */
message_421_t *mailbox_get(mailbox_421_t *box)
{
	return message_dequeue(box);
}

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
  return mailbox_create(bid);
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

	return mailbox_destroy(bid);
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
SYSCALL_DEFINE2(application_add_message, int, bid, void __user *, message){
	/* TODO:
	 * validate bid > 0
	 * copy_from_user(&tmp, user_msg, sizeof(tmp))
	 * validate tmp.mid > 0
	 * validate tmp.data not empty
	 * find mailbox
	 * check message_id_exsits
	 * allocate kernel msg, copy tmp into it, enqueue
	 * return success / correct error codes
	 */
	mailbox_421_t *box;
	message_421_t tmp;
	message_421_t *msg;

	if (bid < 0)
		return -ENOENT;

	/* copy message from user space */
	if (copy_from_user(&tmp, message, sizeof(tmp)))
		return -EACCES;

	/* ensure message has data */
	if (tmp.data[0] == '\0')
		return -EIO;

	/* find mailbox */
	box = mailbox_find(bid);
	if (box == NULL)
		return -ENOENT;

	/* ensure message id is unique */
	if (message_id_exists(box, tmp.mid))
		return -EPERM;

	/* allocate kernel message */
	msg = kmalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL)
		return -ENOMEM;

	/* copy data into kernel message */
	msg->mid = tmp.mid;
	memcpy(msg->data, tmp.data, sizeof(msg->data));
	msg->next = NULL;

	/* enqueue message */
	mailbox_add(box, msg);

	return 0;
}


/**
 * Syscall to destroy a mailbox. * Parameters:
 * - dest: the user-space pointer to copy the found message into. 
 *
 * Return Values:
 *
 * - ENOENT:  there are no mailboxes in the application context OR there are no messages within any box.
 * - EPERM:   the kernel was unable to copy the message to user-space.
 * - 0:       a message was found and copied to user-space.
 */
SYSCALL_DEFINE1(application_get_message, void __user *, dest){
	/* TODO:
         * find best mailbox with messages (lowest bid)
         * deque a message
         * copy_to user(user_dest, msg, sizeof(*msg))
         * free msg
         * return success / correct error codes
         */
	mailbox_421_t *box;
    	message_421_t *msg;
	message_421_t tmp;
    	message_421_t *removed;
	
	/* find highest priority mailbox with messages */
    	box = mailbox_find_best_nonempty();
    	if (box == NULL)
        	return -ENOENT;

    	msg = box->head;
    	if (msg == NULL)
        	return -ENOENT;

    	tmp.next = NULL;
    	tmp.mid = msg->mid;
    	memcpy(tmp.data, msg->data, sizeof(tmp.data));

    	if (copy_to_user(dest, &tmp, sizeof(tmp)))
        	return -EPERM;

    	removed = mailbox_get(box);
    	if (removed != NULL)
        	kfree(removed);

    return 0;
}
