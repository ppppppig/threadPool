#pragma once
#include <thread>
#include <iostream>
#include <vector>
#include <map>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
using namespace std;

class ThreadPool
{
private:
    //管理者线程 
    thread *m_manager;
    //工作线程

    //使用map容器是为了更高效地管理和操作线程池中的各个线程，特别是在需要根据线程ID进行查找、删除等操作时
    map<thread::id, thread> m_works; //用于存储和管理所有的工作线程

    // 用来存放需要被销毁的线程ID列表。
    // 当管理者线程决定销毁某些工作线程时，工作线程会将自己的线程ID添加到这个容器中
    // 实现一种"线程自杀"机制，而不是由管理者线程直接强制终止工作线程
    vector<thread::id> m_ids;

    // 使用原子变量，减少加锁操作
    atomic<int> m_minThread; //最小线程数目
    atomic<int> m_maxThread; //最大线程数目
    atomic<int> m_curThread; //当前线程数目

    //退出线程数目 是指的需要退出的，而不是已经退出的线程总数
    atomic<int> m_exitThread;//退出线程数目   
    atomic<int> m_idleThread;//空闲线程数目

    // 线程池开关
    atomic<bool> m_stop;

    // 任务队列
    // function<void(void)>表示一个可调用的对象（函数、lambda、函数对象等）
    // 在任务队列中，要将所有的任务转换为统一的可调用形式 这种设计使得线程池非常灵活，可以处理各种不同类型的可执行任务。
    // 在C++中，有多种类型的可调用对象：
        // 1. 函数指针              ：函数指针是指向函数的指针变量，它存储了函数的内存地址，可以通过函数指针来调用函数。
        //                          函数指针的声明语法：返回类型 (*指针名)(参数列表);
        // 2. 函数对象（仿函数）
        // 3. Lambda表达式
        // 4. std::function
        // 5. 成员函数指针
        // 6. std::bind绑定的对象
    queue<function<void(void)>> m_tasks;

    mutex m_queueMutex; // 保护任务队列的互斥锁

    // 由于管理者线程和工作线程都会对m_ids进行读取和修改，所以要对其加锁操作
    mutex m_idsMutex;  
    // 条件变量
    // 作用：1.阻塞线程 2.唤醒线程
    condition_variable m_condition;
private:
    void manager();
    void worker();

public:

    ThreadPool(int min=2, int max=thread::hardware_concurrency());
    ~ThreadPool();
    // 添加任务
    void addTask(function<void(void)> task);

    // 泛型编程
    // 整体上是一个模板函数类型
    template<typename F, typename... Args>
    // typename...：声明一个模板参数包
    // Args：参数包的名称（可以是任意名称，如T...、Types...等）    这表示可以接受任意数量、任意类型的模板参数
    auto addTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> {
        // 将返回值的类型取别名
        // result_of<F(Args...)>::type获取函数F接受参数Args...时的返回类型
        using return_type = typename result_of<F(Args...)>::type;

        // 1.打包任务
        // 创建一个packaged_task：将普通函数包装成可异步执行的任务
        // 使用bind和forward完美转发参数
        // 用shared_ptr管理task的生命周期
        auto task = make_shared<packaged_task<return_type()>>(
            bind(forward<F>(f), forward<Args>(args)...)
        );
        //得到future对象
        future<return_type> res = task->get_future();
        {
            lock_guard<mutex> lock(m_queueMutex);
            m_tasks.emplace([task]() { (*task)(); });
        }
        // 唤醒一个线程
        // m_condition.notify_one();
        // 改进：只有当有线程在等待时才通知，避免不必要的系统调用
        if (!m_tasks.empty()) {
            m_condition.notify_one();
        }

        return res;
    }
};


