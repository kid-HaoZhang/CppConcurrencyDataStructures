#include<iostream>
using namespace std;
template <class T, class R, typename... Args>
class  MyDelegate
{
public:
    MyDelegate(T* t, R  (T::*f)(Args...) ):m_t(t),m_f(f) {}

    R operator()(Args&&... args) 
    {
            return (m_t->*m_f)(std::forward<Args>(args) ...);
    }

private:
    T* m_t;
    R  (T::*m_f)(Args...);
};   

template <class T, class R, typename... Args>
MyDelegate<T, R, Args...> CreateDelegate(T* t, R (T::*f)(Args...))
{
    return MyDelegate<T, R, Args...>(t, f);
}

struct A
{
    void Fun(int i){cout<<i<<endl;}
    void Fun1(int i, double j){cout<<i+j<<endl;}
};

int main()
{
    A a;
    auto d = CreateDelegate(&a, &A::Fun); //创建委托
    d(1); //调用委托，将输出1
    auto d1 = CreateDelegate(&a, &A::Fun1); //创建委托
    d1(1, 2.5); //调用委托，将输出3.5
}