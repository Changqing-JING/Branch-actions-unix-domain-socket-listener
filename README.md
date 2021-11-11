# UNIX Domain Socket Listener
It is


### How to Compile
only available in linux
```bash
mkdir build && cd build
cmake ..
make
```

binary is in `build/bin`, `uds-listener-ui`(server with UI) and `liblistener.so`(client)

### How to Use
1. set environment: if both env is undefined, will print the result to `stderr`
```bash
export UNIX_DOMAIN_SOCKET_FORWARD_IP="127.0.0.1"  # target server ip, default localhost
export UNIX_DOMAIN_SOCKET_FORWARD_PORT=1234       # target server port
export LD_PRELOAD=./liblistener.so                # path to liblistener.so
```

2. open uds-listener-ui and click start