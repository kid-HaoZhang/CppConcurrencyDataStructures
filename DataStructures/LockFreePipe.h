#ifndef __Data_Chunk_Queue__
#define __Data_Chunk_Queue__
#include"DataChunkQueue.h"
#include"ErrorAssert.h"
#include <atomic>

template <typename T, int N>
class LockFreePipe{
public:

    LockFreePipe();

    LockFreePipe(const LockFreePipe&) = delete;
    const LockFreePipe &operator=(const LockFreePipe&) = delete;

    void write(const T&, bool);
    bool unwrite(T*);
    bool flush();
    bool check_read();
    bool read(T*);
    bool probe();

private:
    zmqQueue<T, N> queue;
    T *w;
    T* r;
    T* f;
    std::atomic<T*> c;
};

template<typename T, int N>
LockFreePipe<T, N>::LockFreePipe(){
    queue.push();
    r = w = f = &queue.back();
    c.store(&queue.back());
}

template<typename T, int N>
void LockFreePipe<T, N>::write(const T& value, bool imcomplete){
    queue.back() = value;
    queue.push();

    if(!imcomplete)
        f = &queue.back();
}

template<typename T, int N>
bool LockFreePipe<T, N>::unwrite(T *value){
    if(f == &queue.back())
        return false;
    queue.unpush();
    *value = queue.back();
    return true;
}

template<typename T, int N>
bool LockFreePipe<T, N>::flush(){
    if(w == f)
        return true;
    if(!c.compare_exchange_weak(w, f)){
        w = f;
        c.store(f);
        return false;
    }
    else{
        w = f;
        return true;
    }
}

template<typename T, int N>
bool LockFreePipe<T, N>::check_read(){
    if(&queue.front() != r && r)
        return true;
    r = c.load();
    T* p_front = &queue.front();
    c.compare_exchange_weak(p_front, nullptr);
    if(&queue.front() == r || !r)
        return false;
    return true;
}

template<typename T, int N>
bool LockFreePipe<T, N>::read(T *value){
    if(!check_read())
        return false;
    *value = queue.front();
    queue.pop();
    return true;
}

#endif
