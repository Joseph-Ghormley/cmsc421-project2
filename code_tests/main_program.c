#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_APPLICATION_CREATE_MAILBOX 600
#define SYS_APPLICATION_DESTROY_MAILBOX 601
#define SYS_APPLICATION_ADD_MESSAGE 602
#define SYS_APPLICATION_GET_MESSAGE 603

typedef struct message_421 {
    struct message_421 *next;
    int mid;
    char data[256];
} message_421_t;

static long application_create_mailbox(int bid) {
    return syscall(SYS_APPLICATION_CREATE_MAILBOX, bid);
}

static long application_destroy_mailbox(int bid) {
    return syscall(SYS_APPLICATION_DESTROY_MAILBOX, bid);
}

static long application_add_message(int bid, message_421_t *msg) {
    return syscall(SYS_APPLICATION_ADD_MESSAGE, bid, msg);
}

static long application_get_message(message_421_t *dest) {
    return syscall(SYS_APPLICATION_GET_MESSAGE, dest);
}

static void fill_message(message_421_t *msg, int mid, const char *text) {
    memset(msg, 0, sizeof(*msg));
    msg->next = NULL;
    msg->mid = mid;
    strncpy(msg->data, text, sizeof(msg->data) - 1);
    msg->data[sizeof(msg->data) - 1] = '\0';
}

int main(void) {
    message_421_t msg;
    message_421_t out;
    long ret;

    printf("CMSC 421 Project 2 Demo\n\n");

    ret = application_create_mailbox(5);
    printf("create mailbox 5 -> %ld\n", ret);

    ret = application_create_mailbox(1);
    printf("create mailbox 1 -> %ld\n", ret);

    fill_message(&msg, 100, "message from mailbox 5");
    ret = application_add_message(5, &msg);
    printf("add message to mailbox 5 -> %ld\n", ret);

    fill_message(&msg, 200, "message from mailbox 1");
    ret = application_add_message(1, &msg);
    printf("add message to mailbox 1 -> %ld\n", ret);

    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    printf("get_message -> %ld\n", ret);
    if (ret == 0) {
        printf("received mid=%d data=%s\n", out.mid, out.data);
    }

    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    printf("get_message -> %ld\n", ret);
    if (ret == 0) {
        printf("received mid=%d data=%s\n", out.mid, out.data);
    }

    ret = application_destroy_mailbox(1);
    printf("destroy mailbox 1 -> %ld\n", ret);

    ret = application_destroy_mailbox(5);
    printf("destroy mailbox 5 -> %ld\n", ret);

    return 0;
}
