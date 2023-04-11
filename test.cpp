#include<thread>
#include<chrono>
#include"./DataStructures/ThreadSafeQueue.h"
#include"./DataStructures/LockFreeStack.h"

void test_ThreadSafeQueue(){
    ThreadSafeQueue<int> q;
    auto a=[&q](){
        for(int i=0;i<1000;++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            q.push(i);
        }
    };
    auto b=[&q]{
        for(int i=0;i<1000;++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            q.push(i);
        }
    };
    auto c=[&q]{
        for(int i=0;i<1000;++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            q.try_pop();
        }
    };
    auto d=[&q]{
        std::vector<int> r;
        for(int i=0;i<1000;++i){
            r.push_back(*q.wait_and_pop());
        }
        for(auto i:r)
            std::cout<<i<<' ';
    };
    std::thread t1=std::thread(a);
    std::thread t2=std::thread(b);
    std::thread t3=std::thread(c);
    std::thread t4=std::thread(d);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

void test_LockFreeStack_atomic(){
    LockFreeStack_atomic<int> s;
    auto a=[&s](){
        for(int i=0;i<1000;++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            s.push(i);
        }
    };
    auto b=[&s](){
        std::vector<int>r;
        for(int i=0;i<1000;++i){
            std::shared_ptr<int> t=s.pop();
            if(t)
                r.push_back(*t);
        }
        for(auto i:r)
            std::cout<<i<<' ';
    };
    std::thread t1[3],t2[3];
    for(int i=0;i<3;++i){
        t1[i]=std::thread(a);
        t2[i]=std::thread(b);
        t1[i].join();
        t2[i].join();
    }
}

int main(){
    // test_ThreadSafeQueue();
    // test_LockFreeStack_atomic();
    return 0;
}