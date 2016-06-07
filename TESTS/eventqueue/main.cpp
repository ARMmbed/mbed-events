#include "events.h"
#include "mbed.h"
#include "rtos.h"
#include "greentea-client/test_env.h"
#include "unity.h"
#include "utest.h"

using namespace utest::v1;


// flag for called
volatile bool touched = false;

// static functions
void func5(int a0, int a1, int a2, int a3, int a4) { 
    touched = true;
    TEST_ASSERT_EQUAL(a0 | a1 | a2 | a3 | a4, 0x1f);
}

void func4(int a0, int a1, int a2, int a3) {
    touched = true;
    TEST_ASSERT_EQUAL(a0 | a1 | a2 | a3, 0xf); 
}

void func3(int a0, int a1, int a2) {
    touched = true;
    TEST_ASSERT_EQUAL(a0 | a1 | a2, 0x7);
}

void func2(int a0, int a1) {
    touched = true;
    TEST_ASSERT_EQUAL(a0 | a1, 0x3);
}

void func1(int a0) {
    touched = true;
    TEST_ASSERT_EQUAL(a0, 0x1);
}

void func0() {
    touched = true;
}

#define SIMPLE_POSTS_TEST(i, ...)                           \
void simple_posts_test##i() {                               \
    EventQueue queue(32, sizeof &func##i + i*sizeof(int));  \
                                                            \
    touched = false;                                        \
    queue.post(func##i,##__VA_ARGS__);                      \
    queue.dispatch(0);                                      \
    TEST_ASSERT(touched);                                   \
                                                            \
    touched = false;                                        \
    queue.post_in(func##i,##__VA_ARGS__, 1);                \
    queue.dispatch(2);                                      \
    TEST_ASSERT(touched);                                   \
                                                            \
    touched = false;                                        \
    queue.post_every(func##i,##__VA_ARGS__, 1);             \
    queue.dispatch(2);                                      \
    TEST_ASSERT(touched);                                   \
}

SIMPLE_POSTS_TEST(5, 0x01, 0x02, 0x04, 0x08, 0x010)
SIMPLE_POSTS_TEST(4, 0x01, 0x02, 0x04, 0x08)
SIMPLE_POSTS_TEST(3, 0x01, 0x02, 0x04)
SIMPLE_POSTS_TEST(2, 0x01, 0x02)
SIMPLE_POSTS_TEST(1, 0x01)
SIMPLE_POSTS_TEST(0)


void time_func(Timer *t, int ms) {
    TEST_ASSERT(abs(t->read_ms() - ms) <= 1);
    t->reset();
}

template <int N>
void post_in_test() {
    Timer tickers[N];

    EventQueue queue(N, sizeof &time_func + sizeof(Timer *) + sizeof(int));

    for (int i = 0; i < N; i++) {
        queue.post_in(time_func, &tickers[i], (i+1)*100, (i+1)*100);
        tickers[i].start();
    }

    queue.dispatch(N*100);
}

template <int N>
void post_every_test() {
    Timer tickers[N];

    EventQueue queue(N, sizeof &time_func + sizeof(Timer *) + sizeof(int));

    for (int i = 0; i < N; i++) {
        queue.post_every(time_func, &tickers[i], (i+1)*100, (i+1)*100);
        tickers[i].start();
    }

    queue.dispatch(N*100);
}

void event_loop_test() {
    EventLoop loop;
    loop.start();

    touched = false;
    loop.post(func0);
    Thread::yield();
    TEST_ASSERT(touched);

    loop.stop();
}



// Test setup
utest::v1::status_t test_setup(const size_t number_of_cases) {
    //GREENTEA_SETUP(5, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Testing posts with 5 args", simple_posts_test5),
    Case("Testing posts with 4 args", simple_posts_test4),
    Case("Testing posts with 3 args", simple_posts_test3),
    Case("Testing posts with 2 args", simple_posts_test2),
    Case("Testing posts with 1 args", simple_posts_test1),
    Case("Testing posts with 0 args", simple_posts_test0),

    Case("Testing post_in",    post_in_test<20>),
    Case("Testing post_every", post_every_test<20>),

    Case("Testing event loop", event_loop_test),
};

Specification specification(test_setup, cases);

int main() {
    return !Harness::run(specification);
}

