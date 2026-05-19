# 头文件路径
CFLAGS := -g -I../../../build/release-install-cpp11/include -I../../..
# 库路径与库
LDFLAGS := -L../../../build/release-install-cpp11/lib \
           -ljsoncpp -lmuduo_net -lmuduo_base -pthread

# 目标文件
TARGET := server publish_client subscribe_client

# 默认编译所有
all: $(TARGET)

# 生成 server
server: server.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 生成发布客户端
publish_client: publish_client.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 生成订阅客户端
subscribe_client: subscribe_client.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 清理规则（你现在最需要的！）
clean:
	rm -f $(TARGET)