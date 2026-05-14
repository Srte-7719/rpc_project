#include <iostream>
#include <json/value.h>
#include <json/writer.h>
#include <string>
#include <json/json.h>
void serialize(){
    const char *name = "小明";
    int age = 18;
    const char *sex = "男";
    float score[3] = {88,77.5,66};
    Json::Value student;
    student["姓名"] = name;
    student["age"] = age;
    student["性别"] = sex;
    student["成绩"].append(score[0]);
    student["成绩"].append(score[1]);
    student["成绩"].append(score[2]);


    Json::Value fav;
    fav["书籍"] = "西游记";
    fav["运动"] = "西游记";
    student["爱好"] = fav;

    //序列化
    //实例化工厂类对象
    Json::StreamWriterBuilder swb;
    swb.settings_["emitUTF8"] = true;
    //通过工厂类生产派生类
    Json::StreamWriter * sw = swb.newStreamWriter();
    sw->write(student, &std::cout);
    delete sw;
}

int main()
{
    serialize();
    return 0;
}
