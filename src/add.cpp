#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

void add(int a, int b) {
    cout << "Starting task..." << endl;   
    cout << "++++++++++++++++++++++++++++++++++" << endl;   

    cout << "Adding " << a << " and " << b << endl;
    cout << "Sum: " << a + b << endl;
    cout << "++++++++++++++++++++++++++++++++++" << endl;
    cout << "线程休息两秒钟." << endl;
    this_thread::sleep_for(chrono::milliseconds(2000));
}
int add2(int a, int b) {
    cout << "Starting task..." << endl;   
    cout << "++++++++++++++++++++++++++++++++++" << endl;   

    cout << "Adding " << a << " and " << b << endl;
    cout << "Sum: " << a + b << endl;
    cout << "++++++++++++++++++++++++++++++++++" << endl;
    cout << "线程休息两秒钟." << endl;
    this_thread::sleep_for(chrono::milliseconds(2000));
    //线程异步类
    return a + b;
}