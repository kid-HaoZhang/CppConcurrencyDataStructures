#include<iostream>
#include<mutex>
#include<queue>
#include<condition_variable>
template<typename T>
class ThreadSafeQueue{
    struct node{
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::condition_variable cond;
    std::mutex mu, head_mutex, tail_mutex;
    std::unique_ptr<node> head;
    node* tail;

    node* get_tail();
    std::unique_ptr<node> pop_head();
    std::unique_lock<std::mutex> wait_for_data();
    std::unique_ptr<node> wait_pop();
    std::unique_ptr<node> wait_pop(T& value);

    std::unique_ptr<node> try_pop_head();
    std::unique_ptr<node> try_pop_head(T&);

public:
    ThreadSafeQueue();
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(const ThreadSafeQueue&&);
    ThreadSafeQueue& operator=(const ThreadSafeQueue&)=delete;

    void push(T&);
    void push(T&&);

    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);
    std::shared_ptr<T> wait_and_pop();
    void wait_and_pop(T&);

    bool empty() const;

    ~ThreadSafeQueue();
};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue():head(new node), tail(head.get()){};

template<typename T>
void ThreadSafeQueue<T>::push(T& value){
    auto new_data = std::make_shared<T>(std::move(value));
    std::unique_ptr<node> p(new node);
    node* new_tail = p.get();
    {
        std::lock_guard<std::mutex> tail_lck(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
    }
    cond.notify_one();
}

template<typename T>
void ThreadSafeQueue<T>::push(T&& value){
    auto new_data = std::make_shared<T>(std::forward<T>(value));
    std::unique_ptr<node> p(new node);
    node* new_tail = p.get();
    {
        std::lock_guard<std::mutex> tail_lck(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
    }
    cond.notify_one();
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::node> ThreadSafeQueue<T>::try_pop_head(){
    std::lock_guard<std::mutex> lck(head_mutex);
    if(head.get()==get_tail())
        return std::unique_ptr<node>();
    return pop_head();
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::node> ThreadSafeQueue<T>::try_pop_head(T& value){
    std::lock_guard<std::mutex> lck(head_mutex);
    if(head.get()==get_tail())
        return std::unique_ptr<node>();
    value = std::move(*head->data);
    return pop_head();
}

template<typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::try_pop(){
    auto old_head = try_pop_head();
    return old_head?old_head->data:std::shared_ptr<T>();
}

template<typename T>
bool ThreadSafeQueue<T>::try_pop(T& value){
    auto old_head = try_pop_head(value);
    return old_head?true:false;
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::node> ThreadSafeQueue<T>::pop_head(){
    std::unique_ptr<node> old = std::move(head);
    head = std::move(old->next);
    return old;
}

template<typename T>
ThreadSafeQueue<T>::node* ThreadSafeQueue<T>::get_tail(){
    std::lock_guard<std::mutex> lck(tail_mutex);
    return tail;
}

template<typename T>
std::unique_lock<std::mutex> ThreadSafeQueue<T>::wait_for_data(){
    std::unique_lock<std::mutex> lck(head_mutex);
    cond.wait(lck, [&]{return head.get()!=get_tail();});
    return std::move(lck);
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::node> ThreadSafeQueue<T>::wait_pop(){
    std::unique_lock<std::mutex> lck(std::move(wait_for_data()));
    return pop_head();
}

template<typename T>
std::unique_ptr<typename ThreadSafeQueue<T>::node> ThreadSafeQueue<T>::wait_pop(T& value){
    std::unique_lock<std::mutex> lck(std::move(wait_for_data()));
    value = std::move(*head->data);
    return pop_head();
}

template<typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop(){
    std::unique_ptr<node> n(std::move(wait_pop()));
    return n->data;
}

template<typename T>
void ThreadSafeQueue<T>::wait_and_pop(T& value){
    wait_pop(value);
}

template<typename T>
bool ThreadSafeQueue<T>::empty() const{
    std::lock_guard<std::mutex> lck(head_mutex);
    return head.get()==get_tail();
}

template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue(){
}