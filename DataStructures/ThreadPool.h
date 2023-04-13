#include<future>
#include<mutex>
#include<vector>
#include"ThreadSafeQueue.h"

class threads_joiner{ // 防止线程泄漏的joiner
    std::vector<std::thread>& threads;
public:
    explicit threads_joiner(std::vector<std::thread>& threads_):threads(threads_){}
    ~threads_joiner(){
        for(int i=0;i<threads.size();++i){
            if(threads[i].joinable())
                threads[i].join();
        }
    }
};

class FunctionWarpper{
    struct impl_base{
        virtual void call()=0;
        virtual ~impl_base(){}
    };

    std::unique_ptr<impl_base> impl;
    template<typename F>
    struct impl_type: impl_base{
        F f;
        impl_type(F&& f_): f(std::move(f_)){}
        void call(){f();}
    };
public:
    template<typename F>
    FunctionWarpper(F&& f):impl(std::make_unique<impl_type<F>>(std::move(f))){}
    FunctionWarpper()=default;
    void operator()(){impl->call();}
    FunctionWarpper(FunctionWarpper&& other):impl(std::move(other.impl)){}
    FunctionWarpper& operator=(FunctionWarpper&& other){
        impl=std::move(other.impl);
        return *this;
    }
    FunctionWarpper(const FunctionWarpper&)=delete;
    FunctionWarpper& operator=(const FunctionWarpper&)=delete;
};

class ThreadPool{
private:
    std::vector<std::thread> threads;
    threads_joiner joiner;
    std::atomic_bool done;
    ThreadSafeQueue<FunctionWarpper> work_queue;
    unsigned int threads_num;

    void work_thread();

public:
    ThreadPool();
    ~ThreadPool();

    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType);
};

void ThreadPool::work_thread(){
    while(!done){
        FunctionWarpper task;
        if(work_queue.try_pop(task)){
            task();
        }
        else{
            std::this_thread::yield();
        }
    }
}

ThreadPool::ThreadPool():done(false),joiner(threads){
    threads_num = std::thread::hardware_concurrency()-1;
    try{
        for(unsigned int i=0;i<threads_num;++i){
            threads.push_back(std::thread(&ThreadPool::work_thread, this));
        }
    }
    catch(...){
        done=true;
        throw;
    }
}

ThreadPool::~ThreadPool(){
    done=true;
}

template<typename FunctionType>
std::future<typename std::result_of<FunctionType()>::type>  ThreadPool::submit(FunctionType f){
    using result_type = std::result_of<FunctionType()>::type;
    std::packaged_task<result_type()> task(std::move(f));
    std::future<result_type> res(task.get_future());
    work_queue.push(std::move(task));
    return res;
}