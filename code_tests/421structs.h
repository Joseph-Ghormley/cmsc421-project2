#ifndef MSGQ_UMBC_H
#define MSGQ_UMBC_H

/**
 * The Message structure for the project.
 * next - the next message in the list.
 * mid - the Message ID, a unique identifier for the given mailbox.
 * data - the data of the message, no more than 256 bytes (chars) in length.
 */
typedef struct message_421 {
  struct message_421* next;
  int mid;
  char data[256];
} message_421_t;

/**
 * The Mailbox structure for the project.
 * bid - the Box ID, a unique identifier. Lower ID's represent a higher priority.
 * head - the head message in the mailbox, as they are a linked-list, processed in FIFO priority.
 */
typedef struct mailbox_421 {
  int bid;
  message_421_t* head;
  struct mailbox_421* next;
} mailbox_421_t;

/**
 * The Mail Application Context structure for the project.
 * mailboxes - a linked-list of mailboxes in the application.
 */
typedef struct mail_app_context_421 {
  mailbox_421_t* mailboxes;
} mail_app_context_421_t;

/**
 * Mailbox-Related Functions
 * NOTE: refernce the Project 2 Document for how these functions should be implemented.
 *
 * mailbox_create:  attempts to create a mailbox with the given Box ID.
 * mailbox_destroy: attempts to destroy the mailbox with the given Box ID. 
 * mailbox_add:     adds a given message into the supplied mailbox.
 * mailbox_get:     gets the head message from the supplied mailbox, removing it from the list.
 */
int             mailbox_create(int box_id);
int             mailbox_destroy(int box_id);
void            mailbox_add(mailbox_421_t* box, message_421_t* message);
message_421_t*  mailbox_get(mailbox_421_t* box);

#endif
