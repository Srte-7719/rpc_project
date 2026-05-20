CFLAGS := -g -I../../../build/release-install-cpp11/include -I../../..
LDFLAGS := -L../../../build/release-install-cpp11/lib -ljsoncpp -lmuduo_net -lmuduo_base -pthread


all: registry_server rpc_server rpc_client

registry_server: registry_server.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)


rpc_server: rpc_server.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)


rpc_client: rpc_client.cpp
	g++ $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f registry_server rpc_server rpc_client