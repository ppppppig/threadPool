#include "ThreadPool.h"

// 线程池构造函数;
//并使用初始化列表进行初始化，这样可以避免不必要的拷贝
//设置最大线程数和最小线程数量
//初始当前线程数，空闲线程数，线程池停止标志
ThreadPool::ThreadPool(int min, int max) : m_minThread(min), m_maxThread(max), m_curThread(0), m_idleThread(0), m_stop(false)
{
    cout << "该线程池最大的线程数量为："<< max << endl;
    cout << "该线程池最大的线程数量为："<< max << endl;
    cout << "该线程池最小的线程数量为："<< min << endl;

    // 创建线程池中的管理者线程
    // 创建一个新的线程，该线程会执行当前对象（this）的manager成员函数
    m_manager = new thread(&ThreadPool::manager, this);
    // 根据最小线程数创建工作线程
    for (int i = 0; i < min; i++)
    {
        // thread t(&ThreadPool::worker, this);
        // m_works.push_back(move(t));
        // m_works.insert({thread(&ThreadPool::worker, this).get_id(), thread(&ThreadPool::worker, this)});
        // 不会发生拷贝，效率更高

        // 创建一个新的线程，该线程会执行当前对象（this）的worker成员函数
        thread t(&ThreadPool::worker, this);
        // 1.t.get_id() - 获取线程t的唯一标识符（ID）
        // 2.使用move方法实现资源的转移，线程不能进行拷贝
        // 3.{t.get_id(), move(t)} - 创建一个键值对，键是线程ID，值是线程对象本身
        // 4.将键值对插入map容器中
        m_works.insert({t.get_id(), move(t)});
        //当前线程数和空闲线程数量加一
        m_curThread++;
        m_idleThread++;
    }
    
}

/**
* @brief ThreadPool类的析构函数
*
* 销毁ThreadPool对象时调用，负责停止线程池并销毁所有工作线程和管理线程。
*/
/**
* @brief 析构函数，用于销毁线程池并清理资源
*
* 该函数首先将m_stop设置为true，通知所有线程停止工作。
* 然后遍历m_works容器，对于每个线程，先输出其ID，然后检查该线程是否可以被join。
* 如果可以，输出线程ID，调用join函数阻塞等待线程完成，并从m_works中删除该线程。
* 如果存在管理者线程m_manager，并且该线程可以被join，则调用join函数等待其完成，然后删除m_manager并输出相应信息。
*/
ThreadPool::~ThreadPool()
{
    m_stop=true;
    // 是条件变量的一个方法，用于通知所有正在等待该条件变量的线程。当条件变量收到通知时，等待的线程会被唤醒并继续执行。
    m_condition.notify_all();

    // 先销毁工作线程
    for (auto it = m_works.begin(); it != m_works.end(); )// 容器通过迭代器的遍历方法
    {
        // 对于map容器it->first是指的其键，it->second 是指的其值
        cout<<"线程,ID:"<<it->first<<"正在销毁"<<endl;
        if (it->second.joinable())//判断线程是否可以被join
        {
            cout<<"线程,ID:"<<it->second.get_id()<<"被销毁了并且退出"<<endl;
            it->second.join();//阻塞等待线程完成，join是阻塞线程（thread库提供的）的一种方法
        }
        //从容器中移除已经处理过的线程，并将迭代器指向下一个元素。
        it = m_works.erase(it);
        // m_curThread--;
        // m_idleThread--;
    }
    if (m_manager && m_manager->joinable())
    {
    cout<<"管理者线程，ID为"<<m_manager<<"被销毁了并且退出"<<endl;
        m_manager->join();
        delete m_manager;

    }

}


// 创建管理者线程
void ThreadPool::manager(void)
{
    // 循环执行管理任务，直到线程池停止
    // 管理者线程（manager thread）的主要作用是动态管理线程池中的工作线程数量，实现线程池的自动扩容和收缩功能。
    while (!m_stop.load())
    {
        //每三秒检测一次线程池中空闲线程和总线程数量
        this_thread::sleep_for(chrono::seconds(3));

        // 在声明中将m_idleThread和m_curThread声明称了automic（原子变量），可以防止多个线程同时修改
        // load()和store()是原子变量（atomic variable）的两个重要操作方法：
        // load()读取读取原子变量的当前值  store()向原子变量写入新值
        int idle = m_idleThread.load();
        int cur = m_curThread.load();

        // 创建新线程
        // 创建线程的时机是：当前空闲的进程为0，并且当前进程数量还没到限制的最大线程数
        if (idle ==0 && cur < m_maxThread)
        {
            // m_works.emplace_back(thread(&ThreadPool::worker, this));
            thread t(&ThreadPool::worker, this);
            // 使用move方法实现资源的转移
            m_works.insert({t.get_id(), move(t)});

            // m_curThread++;
            // m_idleThread++;
            //在多线程环境下直接++可能导致计数不准确。
            m_curThread.store(m_curThread.load() + 1);
            m_idleThread.store(m_idleThread.load() + 1);
        }

        // 销毁线程
        // 销毁线程的时机：空闲的线程过多，并且要保证最小线程数
        if (idle * 2 > cur && cur > m_minThread)
        {
            // 让工作线程自杀
            // 每次销毁两个线程
            m_exitThread.store(2);

            // 通知所有等待的工作线程，让它们检查是否需要退出
            m_condition.notify_all();

            // 加锁保护线程ID列表的访问
            lock_guard<mutex> lck(m_idsMutex);

            // 遍历需要销毁的线程ID列表
            for (auto id : m_ids)
            {
                /* code */
                // 在工作线程map中查找对应线程（这里也体现出为什么要使用map容器）可以通过id快速寻找到对象
                auto it = m_works.find(id);
                if (it != m_works.end()){
                    cout<<"=======线程,ID:"<<(*it).first<<"被销毁了"<<endl;
                    (*it).second.join();
                    m_works.erase(it);
                }
            }

            // 清空 m_ids 容器中的所有元素。
            m_ids.clear();
            
        }
    }
    
}

// 线程池中工作线程的主函数，负责执行任务
void ThreadPool::worker(void)
{
     // 工作线程的主循环，只要线程池没有停止就继续运行
    while (!m_stop.load())
    {
        // 初始化任务为空
        function<void(void)> task=nullptr;
        // 进入临界区，保护任务队列的访问
        {
            // 给任务队列加锁
            unique_lock<mutex> locker(m_queueMutex);

            // 1.当任务队列为空且线程池未停止时，等待任务
            while (m_tasks.empty() && !m_stop)
            {
                // 等待条件变量通知（有新任务或需要退出）
                m_condition.wait(locker);

                // 检查是否需要退出线程（由管理者线程设置）
                if (m_exitThread.load()>0)
                {
                    // 正确地减少原子变量计数
                    m_curThread.store(m_curThread.load() - 1);
                    m_idleThread.store(m_idleThread.load() - 1);
                    m_exitThread.store(m_exitThread.load() - 1);

                    cout<<"线程退出了,ID:"<<this_thread::get_id()<<endl;

                    // 将自己的ID添加到待销毁列表中
                    lock_guard<mutex> lck(m_idsMutex);
                    m_ids.emplace_back(this_thread::get_id());
                    
                    return;// 线程退出
                }

            }
            // 2.取出一个任务 取任务之前要判断线程池是否为空
            if (!m_tasks.empty())
            {
                cout<<"线程"<<this_thread::get_id()<<"取出了一个任务"<<endl;
                // 获取队列头部任务并移除
                task = move(m_tasks.front());
                m_tasks.pop();
            }
        }

        // 如果获取到了任务，则执行任务
        if (task) //任务是否为空任务
        {   
            // 执行任务，但要先更新空闲线程数
            // 执行任务前，将自己标记为忙碌（空闲线程数减1）
             m_idleThread.store(m_idleThread.load() - 1);
            // m_idleThread--;

            task();// 可调用函数直接加个（）就能执行

            // 执行任务后，恢复空闲状态（空闲线程数加1）
            m_idleThread.store(m_idleThread.load() + 1);
            // m_idleThread++;
            
        }
        

    }
}
void ThreadPool::addTask(function<void(void)> task)
{
    // 1.添加任务
    {
        // 通过｛｝添加作用域，作用域结束后自动释放锁
        // 用管理互斥锁保护
        lock_guard<mutex> lock(m_queueMutex );
        m_tasks.emplace(task);
    }
    // 2.唤醒一个线程
    // m_condition.notify_one();

    // 改进：只有当有线程在等待时才通知，避免不必要的系统调用
    if (!m_tasks.empty()) {
        m_condition.notify_one();
    }
}