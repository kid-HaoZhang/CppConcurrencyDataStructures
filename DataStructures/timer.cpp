#include <sys/epoll.h>
#include <queue>
#include <chrono>
#include <functional>
#include <atomic>
#include <set>
#include <memory>
#include <iostream>

using Task = std::function<void()>;

class TimerNodeBase{
public:
    time_t expireTime;
    int64_t id;
    TimerNodeBase(time_t t, int64_t id): expireTime(t), id(id){};
};

class TimerNode: public TimerNodeBase{
public:
    TimerNode(time_t delay, Task callback, int64_t id): 
    TimerNodeBase(delay, id), callback(std::move(callback)){};
    Task callback;
};

bool operator < (const TimerNodeBase& lhd, const TimerNodeBase& rhd){
    if(lhd.expireTime < rhd.expireTime)
        return true;
    else if(lhd.expireTime > rhd.expireTime)
        return false;
    return lhd.id < rhd.id;
}

class Timer{
public:
    static time_t getTick(){
        auto sc = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
        auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(sc.time_since_epoch());
        return tmp.count();
    }

    template<typename FunctionType, typename ...Args>
    TimerNodeBase addTimer(time_t msec, FunctionType&& f, Args... args){
        Task callback = std::bind(std::forward<FunctionType>(f), std::forward<Args>(args)...);
        TimerNode tnode(getTick() + msec, callback, get_id());
        map.insert(tnode);
        return static_cast<TimerNodeBase>(tnode);
    }

    bool delTimer(TimerNodeBase &node){
        auto it = map.find(node);
        if(it != map.end()){
            map.erase(it);
            return true;
        }
        return false;
    }

    bool checkTimer(){
        auto it = map.begin();
        if(it != map.end() && it->expireTime <= getTick()){
            it->callback();
            map.erase(it);
            return true;
        }
        return false;
    }

    time_t timeToSleep(){
        auto it = map.begin();
        if(it == map.end())
            return -1;
        auto diff = it->expireTime - getTick();
        return diff > 0 ? diff : 0;
    }
private:
    static int64_t gid;

    int64_t get_id(){
        return ++gid;
    }
    std::set<TimerNode, std::less<>> map;
};
int64_t Timer::gid = 0;

int main(){
    int epfd = epoll_create(1);
    std::unique_ptr<Timer> timer = std::make_unique<Timer>();
    auto startTime = std::chrono::system_clock::now();
    timer->addTimer(1000, [&](){
        auto endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        std::cout << elapsed.count() << std::endl;
    });
    epoll_event ev[64] = {0};
    while(true){
        int n = epoll_wait(epfd, ev, 64, timer->timeToSleep());
        while(timer->checkTimer());
    }

}