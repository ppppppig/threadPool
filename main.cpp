#include <iostream>
#include "add.cpp"
#include "ThreadPool.cpp"
using namespace std;    

int main() {
    cout << "Hello, World!" << endl;
    // add(5, 10);
    std::cout << "-------------------------------------------------" << std::endl;

    // 创建线程池对象，最小线程数2，最大线程数4
    cout<<"硬件支持的并发线程数:"<<thread::hardware_concurrency()<<endl;
    
    cout<<"请输入你想要求的最小线程数量：";
    int mixThreadNum=0;
    cin >> mixThreadNum;
    cout<<"请输入你想要求的最大线程数量：";
    int maxThreadNum=0;
    cin >> maxThreadNum;

    ThreadPool pool(mixThreadNum,maxThreadNum);

    // future是c++中的模板类 ，用于获取异步操作的结果
    // future<int> 代表一个将来会得到int类型结果的"凭证"
    // vector是c++中的动态数组，可以存放任意类型的对象
    // 这里使用vector来存放多个future对象，以便后续获取每个任务的执行结果,用于保存所有提交给线程池的任务的future对象
    vector<future<int>> results;

    // 需要注意的是：这里是创建20个add任务，而不是创建20个线程，
    // 会使用线程复用机制避免创建过多的线程导致系统资源的浪费
    // int count = 20;
    int count = 50;
    for (int i = 0; i < count; i++)
    {
        cout<<"--------------------创建第"<< i << "任务--------------------"<<endl;
        // auto obj = bind(add2, i, i *2); // 绑定函数对象
        // pool.addTask(obj);
       results.emplace_back(pool.addTask(add2, i, i *2)); // addTask模板函数返回future<int>，添加到results的vector容器中
    }
    for (auto& result : results)
    {
        cout <<"线程执行结果：" << result.get() << endl;
    }
    cout << "-------------------------------------------------" << endl;
    // // 阻塞主线程
    // getchar();
    return 0;
}