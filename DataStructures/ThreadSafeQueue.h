#include<iostream>
#include<mutex>
#include<queue>
#include<condition_variable>
template<typename T>
class ThreadSafeQueue{
    std::condition_variable cond;
    std::mutex mu;
    std::queue<T> q;
public:
    ThreadSafeQueue();
    ThreadSafeQueue(const ThreadSafeQueue&);
    ThreadSafeQueue(const ThreadSafeQueue&&);
    ThreadSafeQueue& operator=(const ThreadSafeQueue&)=delete;

    void push(T&);
    std::shared_ptr<T> try_pop();
    std::shared_ptr<T> wait_and_pop();

    bool empty() const;

    ~ThreadSafeQueue();
};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(){};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue& other){
    std::lock_guard<std::mutex> lk(mu);
    q=other.q;
}

template<typename T>
void ThreadSafeQueue<T>::push(T& value){
    std::lock_guard<std::mutex> lck(mu);
    q.push(value);
    cond.notify_one();
}

template<typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::try_pop(){
    std::unique_lock<std::mutex> lck(mu);
    if(q.empty())
        return std::shared_ptr<T>();
    std::shared_ptr<T> res=std::make_shared<T>(q.front());
    q.pop();
    return res;
}

template<typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop(){
    std::unique_lock<std::mutex> lck(mu);
    cond.wait(lck, [this](){return !q.empty();});
    std::shared_ptr<T> res(std::make_shared<T>(q.front()));
    q.pop();
    return res;
}

template<typename T>
bool ThreadSafeQueue<T>::empty() const{
    std::lock_guard<std::mutex> lck(mu);
    return q.empty();
}

template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue(){
}