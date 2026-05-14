#include <iostream>
#include <future>
#include <thread>
int Add(int a, int b) {
    std::cout << "Add: " << a << " + " << b << std::endl;
    return a + b;
}

int main()
{
    std::future<int> res = std::async(std::launch::async, Add, 1, 2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout<<"--------------------"<<std::endl;
    std::cout << res.get() << std::endl;
    return 0;
}
