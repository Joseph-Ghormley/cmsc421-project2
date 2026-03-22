#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

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

static int tests_run = 0;
static int tests_passed = 0;

static void print_result(const char *name, int passed) {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("[PASS] %s\n", name);
    } else {
        printf("[FAIL] %s\n", name);
    }
}

static int expect_ret(const char *name, long actual, long expected) {
    int passed = (actual == expected);
    if (!passed) {
        printf("  expected: %ld, actual: %ld\n", expected, actual);
    }
    print_result(name, passed);
    return passed;
}

static int expect_message(const char *name, message_421_t *msg, int expected_mid, const char *expected_data) {
    int passed = 1;

    if (msg->mid != expected_mid) {
        printf("  wrong mid: expected %d, got %d\n", expected_mid, msg->mid);
        passed = 0;
    }

    if (strcmp(msg->data, expected_data) != 0) {
        printf("  wrong data: expected \"%s\", got \"%s\"\n", expected_data, msg->data);
        passed = 0;
    }

    print_result(name, passed);
    return passed;
}

static void fill_message(message_421_t *msg, int mid, const char *text) {
    memset(msg, 0, sizeof(*msg));
    msg->next = NULL;
    msg->mid = mid;
    strncpy(msg->data, text, sizeof(msg->data) - 1);
    msg->data[sizeof(msg->data) - 1] = '\0';
}

static void cleanup_mailbox_if_possible(int bid) {
    message_421_t tmp;
    while (application_get_message(&tmp) == 0) {
    }
    application_destroy_mailbox(bid);
}

int main(void) {
    long ret;
    message_421_t msg;
    message_421_t out;

    printf("Running CMSC 421 Project 2 tests...\n\n");

    /*
     * Start by cleaning up anything leftover from previous runs.
     * This is best-effort cleanup.
     */
    application_destroy_mailbox(1);
    application_destroy_mailbox(2);
    application_destroy_mailbox(5);
    application_destroy_mailbox(10);

    /*
     * 1. Create mailbox
     */
    ret = application_create_mailbox(10);
    expect_ret("create mailbox 10", ret, 0);

    /*
     * 2. Duplicate mailbox create should fail with -EPERM
     */
    ret = application_create_mailbox(10);
    expect_ret("duplicate create mailbox 10", ret, -EPERM);

    /*
     * 3. Destroy nonexistent mailbox should fail with -ENOENT
     */
    ret = application_destroy_mailbox(999);
    expect_ret("destroy nonexistent mailbox 999", ret, -ENOENT);

    /*
     * 4. Add message to nonexistent mailbox should fail with -ENOENT
     */
    fill_message(&msg, 1, "hello");
    ret = application_add_message(999, &msg);
    expect_ret("add message to nonexistent mailbox", ret, -ENOENT);

    /*
     * 5. Empty message data should fail with -EIO
     */
    fill_message(&msg, 2, "");
    ret = application_add_message(10, &msg);
    expect_ret("add empty message data", ret, -EIO);

    /*
     * 6. Add first valid message to mailbox 10
     */
    fill_message(&msg, 1, "first");
    ret = application_add_message(10, &msg);
    expect_ret("add first valid message to mailbox 10", ret, 0);

    /*
     * 7. Duplicate message ID in same mailbox should fail with -EPERM
     */
    fill_message(&msg, 1, "duplicate-mid");
    ret = application_add_message(10, &msg);
    expect_ret("duplicate message id in same mailbox", ret, -EPERM);

    /*
     * 8. Add second message to same mailbox
     */
    fill_message(&msg, 2, "second");
    ret = application_add_message(10, &msg);
    expect_ret("add second valid message to mailbox 10", ret, 0);

    /*
     * 9. Destroy non-empty mailbox should fail with -EPERM
     */
    ret = application_destroy_mailbox(10);
    expect_ret("destroy non-empty mailbox 10", ret, -EPERM);

    /*
     * 10. FIFO test: first get should return mid=1, data=first
     */
    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    expect_ret("get first message from mailbox 10", ret, 0);
    expect_message("verify FIFO first message", &out, 1, "first");

    /*
     * 11. FIFO test: second get should return mid=2, data=second
     */
    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    expect_ret("get second message from mailbox 10", ret, 0);
    expect_message("verify FIFO second message", &out, 2, "second");

    /*
     * 12. No messages left should fail with -ENOENT
     */
    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    expect_ret("get message when all mailboxes empty", ret, -ENOENT);

    /*
     * 13. Mailbox can now be destroyed
     */
    ret = application_destroy_mailbox(10);
    expect_ret("destroy empty mailbox 10", ret, 0);

    /*
     * 14. Priority test setup:
     * create mailbox 5 and mailbox 1
     * lower bid has higher priority
     */
    ret = application_create_mailbox(5);
    expect_ret("create mailbox 5", ret, 0);

    ret = application_create_mailbox(1);
    expect_ret("create mailbox 1", ret, 0);

    /*
     * Add message to lower priority mailbox first
     */
    fill_message(&msg, 10, "from mailbox 5");
    ret = application_add_message(5, &msg);
    expect_ret("add message to mailbox 5", ret, 0);

    /*
     * Add message to higher priority mailbox second
     */
    fill_message(&msg, 20, "from mailbox 1");
    ret = application_add_message(1, &msg);
    expect_ret("add message to mailbox 1", ret, 0);

    /*
     * 15. application_get_message should return mailbox 1 message first
     * because bid 1 is higher priority than bid 5
     */
    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    expect_ret("get highest priority mailbox message first", ret, 0);
    expect_message("verify priority returned mailbox 1 message", &out, 20, "from mailbox 1");

    /*
     * 16. Next get should now return mailbox 5 message
     */
    memset(&out, 0, sizeof(out));
    ret = application_get_message(&out);
    expect_ret("get remaining mailbox 5 message", ret, 0);
    expect_message("verify mailbox 5 message returned second", &out, 10, "from mailbox 5");

    /*
     * 17. Now both should destroy cleanly
     */
    ret = application_destroy_mailbox(1);
    expect_ret("destroy empty mailbox 1", ret, 0);

    ret = application_destroy_mailbox(5);
    expect_ret("destroy empty mailbox 5", ret, 0);

    /*
     * 18. Invalid pointer test for get_message
     * This should return -EPERM if your kernel code handles copy_to_user failure correctly.
     * Comment this out if your environment does not like invalid user pointers.
     */
    ret = application_get_message((message_421_t *)0x1);
    expect_ret("get_message with invalid destination pointer", ret, -ENOENT);

    printf("\nSummary: %d/%d tests passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
