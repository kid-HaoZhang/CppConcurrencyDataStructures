#include<thread>
#include<chrono>
#include<sys/time.h>
#include<mutex>
#include<time.h>

#include"./DataStructures/LockFreeStack.h"
#include"./DataStructures/ThreadSafeShared_ptr.h"
#include"./DataStructures/ThreadPool.h"
#include"./DataStructures/LockFreePipe.h"
#include"./DataStructures/LockFreeQueue.h"
#include"./DataStructures/BlockQueue.h"

// #define DEBUG

static int TEST_TIME=2000000;
static int THREAD_NUM = 3;

int64_t get_current_millisecond()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
}

void test_ThreadSafeQueue(){
    ThreadSafeQueue<int> q;
    auto a=[&q](){
        for(int i=0;i<TEST_TIME;++i){
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            q.push(i);
        }
    };
    auto c=[&q]{
        // std::atomic<int> a(0);
        for(int i=0;i<TEST_TIME;++i){
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // q.try_pop();
            // a.fetch_add(1);
            q.wait_and_pop();
#ifdef DEBUG
            std::cout<< *(q.wait_and_pop())<<' '<<std::flush;
#endif
        }
    };
    auto d=[&q]{
        std::vector<int> r;
        for(int i=0;i<1000;++i){
            r.push_back(*q.wait_and_pop());
        }
#ifdef DEBUG
        for(auto i:r)
            std::cout<<i<<' ';
#endif
    };
    int64_t start = get_current_millisecond();
    std::thread t1[THREAD_NUM];
    std::thread t3[THREAD_NUM];
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i] = std::thread(a);
        t3[i] = std::thread(c);
    }
    // std::thread t4=std::thread(d);
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i].join();
        t3[i].join();
    }
    // t4.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

void test_ThreadSafeQueue1(){
    std::queue<int> q;
    std::mutex mu;
    auto a=[&q, &mu](){
        for(int i=0;i<TEST_TIME;++i){
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> lck(mu);
            q.push(i);
        }
    };
    auto c=[&q, &mu]{
        std::atomic<int> a(0);
        for(int i=0;i<TEST_TIME;++i){
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // q.try_pop();
            std::lock_guard<std::mutex> lck(mu);
            // a.fetch_add(1);
            q.pop();
#ifdef DEBUG
            std::cout<< *(q.wait_and_pop())<<' '<<std::flush;
#endif
        }
    };
    int64_t start = get_current_millisecond();
    std::thread t1[THREAD_NUM];
    std::thread t3[THREAD_NUM];
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i] = std::thread(a);
        t3[i] = std::thread(c);
    }
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i].join();
        t3[i].join();
    }
    // t4.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

void test_theardSafeShared_ptr(){
    ThreadSafeShared_ptr<int> s(new int(1));
    std::vector<ThreadSafeShared_ptr<int>> s1;
    std::vector<ThreadSafeShared_ptr<int>> s2;
    auto t1=[&s, &s1](){
        for(int i=0;i<TEST_TIME;++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ThreadSafeShared_ptr<int> t(s);
            s1.emplace_back(t);
        }
    };
    auto t2=[&s, &s2](){
        for(int i=0;i<TEST_TIME;++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            ThreadSafeShared_ptr<int> t(s);
            s2.emplace_back(t);
        }
    };
    std::thread th1=std::thread(t1);
    std::thread th2=std::thread(t2);
    th1.join();
    th2.join();
    return;
}

void test_LockFreePipe(){
    std::atomic<int> count(0);
    LockFreePipe<int, 100> lckfq;
    auto pro = [&lckfq](){
        for(int i = 0; i < TEST_TIME; ++i){
            lckfq.write(i, false);
            lckfq.flush();
        }
    };
    auto con = [&lckfq, &count](){
        int last_value = 0;
        while(true){        
            int value = 0;
            if(lckfq.read(&value)){
                count.fetch_add(1);
                last_value = value;
#ifdef DEBUG
                std::cout << last_value << ' '<<std::flush;
#endif
            }
            else{
                std::this_thread::yield();
            }
            if(count.load()>=TEST_TIME)
                break;
        }
    };
    int64_t start = get_current_millisecond();
    std::thread t1 = std::thread(pro);
    std::thread t2 = std::thread(con);
    t1.join();
    t2.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

void test_LockPipe_cond(){
    std::mutex mu;
    std::condition_variable cond;
    LockFreePipe<int, 100> lckfq;

    auto pro = [&lckfq, &cond, &mu]{
        for(int i = 0; i < TEST_TIME; ++i){
            lckfq.write(i, false);
            if(!lckfq.flush()){
                std::unique_lock<std::mutex> lck(mu);
                cond.notify_one();
            }
        }
        std::unique_lock<std::mutex> lck(mu);
        cond.notify_one();
    };
    auto con = [&lckfq, &cond, &mu]{
        int vl;
        std::atomic<int> count(0);
        while(true){
            int v = 0;
            if(lckfq.read(&v)){
                vl = v;
#ifdef DEBUG
                std::cout<<"read:"<<vl;
#endif
                count.fetch_add(1);
            }
            else{
                std::unique_lock<std::mutex> lck(mu);
                cond.wait(lck);
            }
            if(count >= TEST_TIME)
                break;
        }
    };
    int64_t start = get_current_millisecond();
    std::thread t1 = std::thread(pro);
    std::thread t2 = std::thread(con);
    t1.join();
    t2.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

void test_LockFreePipe_batch(){
    LockFreePipe<int, 100> lckfq;
    auto con = [&lckfq]{
        std::atomic<int> count(0);
        int vl = 0;
        while(true){
            int v = 0;
            if(lckfq.read(&v)){
                count.fetch_add(1);
#ifdef DEBUG
                std::cout << v << ' '<<std::flush;
#endif
            }
            else{
                std::this_thread::yield();
            }
            if(count>=TEST_TIME)
                break;
        }
    };
    auto pro = [&lckfq]{
        int n = TEST_TIME/10;
        for(int i = 0; i < n; ++i){
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, true);
            lckfq.write(i, false);
            lckfq.flush();
        }
    };
    int64_t start = get_current_millisecond();
    std::thread t1 = std::thread(pro);
    std::thread t2 = std::thread(con);
    t1.join();
    t2.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

void test_LockFreeRingbufferQueue(){
    LockFreeQueueCpp11<int> lckfq(TEST_TIME);
    auto a = [&lckfq]{
        for(int i = 0; i < TEST_TIME; ++i)
            lckfq.push(i);
    };
    auto c = [&lckfq]{
        int value;
        for(int i = 0; i < TEST_TIME; ++i)
            lckfq.pop(value);
    };
    int64_t start = get_current_millisecond();
    std::thread t1[THREAD_NUM];
    std::thread t3[THREAD_NUM];
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i] = std::thread(a);
        t3[i] = std::thread(c);
    }
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i].join();
        t3[i].join();
    }
    // t4.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

void test_BlockQueue(){
    BlockQueue q;
    auto a=[&q](){
        for(int i=0;i<TEST_TIME;++i){
            q.Enqueue(i);
        }
    };
    auto c=[&q]{
        int value;
        for(int i=0;i<TEST_TIME;++i){
            q.Dequeue(value);
#ifdef DEBUG
            std::cout<< *(q.wait_and_pop())<<' '<<std::flush;
#endif
        }
    };
    int64_t start = get_current_millisecond();
    std::thread t1[THREAD_NUM];
    std::thread t3[THREAD_NUM];
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i] = std::thread(a);
        t3[i] = std::thread(c);
    }
    // std::thread t4=std::thread(d);
    for(int i = 0; i < THREAD_NUM; ++i){
        t1[i].join();
        t3[i].join();
    }
    // t4.join();
    int64_t end = get_current_millisecond();
    printf("spend time : %ldms\t, push:%d, pop:%d\n", (end - start), TEST_TIME, TEST_TIME);
}

int main(){
    test_ThreadSafeQueue();
    // test_ThreadSafeQueue1();
    // test_LockFreeStack_atomic();
    // test_theardSafeShared_ptr();
    // std::cout << "pip" << std::endl;
    // test_LockFreePipe();
    // std::cout << "pip_cond" << std::endl;
    // test_LockPipe_cond();
    // std::cout << "pip_batch" << std::endl;
    // test_LockFreePipe_batch();
    test_LockFreeRingbufferQueue();
    test_BlockQueue();
    return 0;
}