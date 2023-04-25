#include <mutex>
#include <condition_variable>
#include <queue>

class BlockQueue
{
public:
  BlockQueue()
  {
  }
  ~BlockQueue()
  {
  }
  // 插入packet，需要指明音视频类型
  // 返回0说明正常
  int Enqueue(int val)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(val);

    cond_.notify_one();
    return 0;
  }

  // 取出packet，并也取出对应的类型
  // 返回值: -1 abort; 1 获取到消息; 0 超时
  int Dequeue(int &val, const int timeout = 0)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty())
    { // 等待唤醒
      // return如果返回false，继续wait, 如果返回true退出wait
      cond_.wait_for(lock, std::chrono::milliseconds(timeout), [this]
                     { return !queue_.empty() | abort_request_; });
    }
    if (abort_request_)
    {
      printf("abort request");
      return -1;
    }
    if (queue_.empty())
    {
      return 0;
    }

    // 真正干活
    val = queue_.front(); //读取队列首部元素，这里还没有真正出队列
    queue_.pop();
    return 1;
  }

  bool Empty()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }
  // 唤醒在等待的线程
  void Abort()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    abort_request_ = true;
    cond_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::queue<int> queue_;

  bool abort_request_ = false;
};