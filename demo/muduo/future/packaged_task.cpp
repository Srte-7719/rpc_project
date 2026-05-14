#include <iostream>
#include <future>
#include <thread> 
int Add(int a, int b) {
    std::cout << "Add: " << a << " + " << b << std::endl;
    return a + b;
}

int main()
{
    //1.封装任务
    auto task = std::make_shared<std::packaged_task<int(int, int)>>(Add);
    //2.获取任务包关联的future对象
    std::future<int> res = task->get_future();
    std::cout<<"--------------------"<<std::endl;
    //3.执行任务
    std::thread t([&task]() {
        task->operator()(1, 2);
    });
    t.join();
    std::cout<<"--------------------"<<std::endl;
    //4.获取结果
    std::cout << res.get() << std::endl;
    return 0;
}
