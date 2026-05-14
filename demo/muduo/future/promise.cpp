#include <iostream>
#include <future>
#include <thread>
int Add(int a, int b) {
    std::cout << "Add: " << a << " + " << b << std::endl;
    return a + b;
}

int main()
{
    //三步走
    //1.获取promise对象
    std::promise<int> p;
    //2.获取future对象
    std::future<int> f = p.get_future();
    //3.执行任务
    std::thread t([&p]() {
        int sum= Add(1, 2);
        p.set_value(sum);
    });
    t.join(); 
    std::cout << f.get() << std::endl;
    return 0;
}
