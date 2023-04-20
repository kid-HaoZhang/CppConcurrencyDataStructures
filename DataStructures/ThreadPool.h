#include<future>
#include<mutex>
#include<vector>
#include<functional>
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

class ThreadPool{
private:
    using Task = std::function<void()>;
    std::vector<std::thread> threads;
    threads_joiner joiner;
    std::atomic_bool done;
    ThreadSafeQueue<Task> work_queue;
    unsigned int threads_num;

    void work_thread();

public:
    ThreadPool();
    ~ThreadPool();

    template<typename FunctionType, typename... Args>
    auto submit(FunctionType&&, Args&&...);
};

void ThreadPool::work_thread(){
    while(!done){
        Task task;
        if(work_queue.try_pop(task)){
            task();
        }
        else{
            std::this_thread::yield();
        }
    }
}

ThreadPool::ThreadPool():done(false),joiner(threads){
    threads_num=std::thread::hardware_concurrency();
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

template<typename FunctionType, typename... Args>
auto  ThreadPool::submit(FunctionType&& f, Args&&... args){
    using result_type = std::result_of<FunctionType(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<result_type()>>(std::bind(std::forward<FunctionType>(f), std::forward<Args>(args)...));
    std::future<result_type> res(task->get_future());
    work_queue.push([task](){(*task)();});
    return res;
}