#ifndef __DATA_CHUNK_QUEUE__
#define __DATA_CHUNK_QUEUE__
#endif
#include"ErrorAssert.h"
#include<atomic>

template<typename T, int N>
class zmqQueue{
public:
    inline zmqQueue();
    inline ~zmqQueue();

    zmqQueue(const zmqQueue&) = delete;
    const zmqQueue& operator=(const zmqQueue&) = delete;

    inline T* front();
    inline T* back();

    inline void push();  // 增加一个元素
    inline void unpush();
    inline void pop();

private:
    struct chunk_t
    {
        T values[N];
        chunk_t* prev;
        chunk_t* next;
    };

    chunk_t* begin_chunk;
    chunk_t* back_chunk;
    chunk_t* end_chunk;
    unsigned int begin_pos;
    unsigned int back_pos;
    unsigned int end_pos;

    std::atomic<chunk_t*> spare_chunk;
};

template <typename T, int N>
inline zmqQueue<T,N>::zmqQueue(){
    begin_chunk = (chunk_t*)malloc(sizeof(chunk_t));
    alloc_assert(begin_chunk);
    back_chunk = nullptr;
    end_chunk = begin_chunk;
    begin_pos = back_pos = end_pos = 0;
    spare_chunk=NULL;
}

template <typename T, int N>
inline zmqQueue<T,N>::~zmqQueue(){
    while (begin_chunk != end_chunk)
    {
        chunk_t* t = begin_chunk;
        begin_chunk = begin_chunk->next;
        free(t);
    }
    free(begin_chunk);

    chunk_t* t = spare_chunk.exchange(nullptr);
    free(t);
}

template <typename T, int N>
inline T* zmqQueue<T,N>::front(){
    return &begin_chunk->values[begin_pos];
}

template <typename T, int N>
inline T* zmqQueue<T,N>::back(){
    return &back_chunk->values[back_pos];
}

template <typename T, int N>
inline void zmqQueue<T,N>::push(){
    back_chunk = end_chunk;
    back_pos = end_pos;

    if(++end_pos == N)
        return;
    
    chunk_t* sc = spare_chunk.exchange(nullptr);
    if(sc){
        end_chunk->next = sc;
        sc->prev = end_chunk;
    }
    else{
        end_chunk->next = (chunk_t*)malloc(sizeof(chunk_t));
        alloc_assert(end_chunk);
        end_chunk->next->prev = end_chunk;
    }
    end_chunk = end_chunk->next;
    end_pos = 0;
}

template <typename T, int N>
inline void zmqQueue<T,N>::unpush(){
    if(back_pos)
        --back_pos;
    else{
        back_pos = N-1;
        back_chunk = back_chunk->prev;
    }
    if(end_pos)
        --end_pos;
    else{
        end_pos = N - 1;
        end_chunk = end_chunk->prev;
        free(end_chunk->next);
        end_chunk->next = nullptr;
    }
}

template <typename T, int N>
inline void zmqQueue<T,N>::pop(){
    if(++begin_pos==N){
        chunk_t* t = begin_chunk;
        begin_chunk = begin_chunk->next;
        begin_chunk->prev = nullptr;
        begin_pos = 0;

        chunk_t* sc = spare_chunk.exchange(t);
        free(sc);
    }
}
