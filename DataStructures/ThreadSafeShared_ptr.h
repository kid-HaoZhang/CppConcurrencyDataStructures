#include<atomic>

template <typename T>
class ThreadSafeShared_ptr{
private:
    std::atomic<long>* count;
    T* ptr;
public:
    ThreadSafeShared_ptr(T* ptr):ptr(ptr),count(new std::atomic<long>(1)){};

    ThreadSafeShared_ptr():ptr(nullptr),count(new std::atomic<long>(0)){};

    ThreadSafeShared_ptr(ThreadSafeShared_ptr<T>&& other):ptr(other.ptr),count(other.count){
        other.ptr=nullptr;
        other.count=nullptr;
    }

    ThreadSafeShared_ptr(ThreadSafeShared_ptr<T>& other){
        count=other.count;
        ptr=other.ptr;
        (*count).fetch_add(1);
    };

    ~ThreadSafeShared_ptr(){
        if(ptr!=nullptr)
            if((*count).fetch_sub(1)==1){
                delete count;
                delete ptr;
            }
    }

    ThreadSafeShared_ptr& operator=(ThreadSafeShared_ptr& other){
        if(*this==other)
            return *this;
        if((*count).fetch_sub(1)==1){
            delete count;
            delete ptr;
        }
        count=other.count;
        ptr=other.ptr;
        (*count).fetch_add(1);
        return *this;
    }

    operator bool() const{
        return ptr!=nullptr;
    }

    T* operator->() const{
        return ptr;
    }

    T& operator*() const{
        return *ptr;
    }

    int use_count() const{
        return (*count).load();
    }

    T* get() const{
        return ptr;
    }

    void swap(ThreadSafeShared_ptr<T>& other){
        std::swap(ptr, other.ptr);
        std::swap(count, other.count);
    }
};