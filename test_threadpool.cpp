#include<numeric>
#include<future>
#include<vector>
#include "./DataStructures/ThreadPool.h"


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
      return accumulate_block<Iterator,T>()(block_start,block_end);
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

int mul(int a, int b){
    return a*b;
}

void test_ThreadPool1(){
    ThreadPool pool;
    std::vector<std::future<int>> futures(100);
    for(int i = 0; i < 100; ++i)
        futures[i] = pool.submit(mul, 1, 2);
    for(int i = 0; i < 100; ++i)
        std::cout << futures[i].get()<< std::endl;
}

int main(){
    test_ThreadPool();
    test_ThreadPool1();
}