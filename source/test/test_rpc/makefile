
CFLAGS := -g -I../../../build/release-install-cpp11/include -I../../..

LDFLAGS := -L../../../build/release-install-cpp11/lib \
           -ljsoncpp -lmuduo_net -lmuduo_base -pthread


TARGET := client server


all: $(TARGET)

client: test_client.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)

server: test_server.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)
clean:
	rm -f $(TARGET)