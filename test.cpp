#include<thread>
#include<chrono>
#include<numeric>
#include<sys/time.h>
#include<mutex>
#include<time.h>

#include"./DataStructures/LockFreeStack.h"
#include"./DataStructures/ThreadSafeShared_ptr.h"
#include"./DataStructures/ThreadPool.h"
#include"./DataStructures/LockFreePipe.h"

static int TEST_TIME=1000;

int64_t get_current_millisecond()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
}

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

/*void test_LockFreeStack_atomic(){
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
}*/

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
template<typename Iterator,typename T>
struct accumulate_block
{
  T operator()(Iterator first,Iterator last)
  {
    T res; 
    std::accumulate(first,last,res);
    return res;
  }
};

template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init)
{
  unsigned long const length=std::distance(first,last);
  
  if(!length)
    return init;

  unsigned long const block_size=25;
  unsigned long const num_blocks=(length+block_size-1)/block_size;  // 1

  std::vector<std::future<T>> futures(num_blocks-1);
  ThreadPool pool;

  Iterator block_start=first;
  for(unsigned long i=0;i<(num_blocks-1);++i)
  {
    Iterator block_end=block_start;
    std::advance(block_end,block_size);
    futures[i]=pool.submit([block_start,block_end]{
      accumulate_block<Iterator,T>()(block_start,block_end);
    }); // 2
    block_start=block_end;
  }
  T last_result=accumulate_block<Iterator,T>()(block_start,last);
  T result=init;
  for(unsigned long i=0;i<(num_blocks-1);++i)
  {
    result+=futures[i].get();
  }
  result += last_result;
  return result;
}
void test_ThreadPool(){
    std::vector<int> a(10000,0);
    std::iota(a.begin(),a.end(),0);
    std::cout<<parallel_accumulate<std::vector<int>::iterator, int>
        (a.begin(),a.end(),0)<<std::endl;
}

void test_LockFreePipe(){
    std::atomic<int> count(0);
    LockFreePipe<int, 128> lckfq;
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
            }
            else{
                std::this_thread::yield();
            }
            if(count.load()>=TEST_TIME)
                break;
        }
    };
    int64_t start = get_current_millisecond();
    
}

int main(){
    // test_ThreadSafeQueue();
    // test_LockFreeStack_atomic();
    // test_theardSafeShared_ptr();
    test_ThreadPool();
    return 0;
}