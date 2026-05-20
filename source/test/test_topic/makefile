CFLAGS := -g -I../../../build/release-install-cpp11/include -I../../..
LDFLAGS := -L../../../build/release-install-cpp11/lib \
           -ljsoncpp -lmuduo_net -lmuduo_base -pthread


TARGET := server publish_client subscribe_client


all: $(TARGET)
server: server.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)


publish_client: publish_client.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)


subscribe_client: subscribe_client.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)


clean:
	rm -f $(TARGET)