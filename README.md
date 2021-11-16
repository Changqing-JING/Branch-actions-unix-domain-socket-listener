# UNIX Domain Socket Listener
It is a server-client unix domain socket listener


### How to Compile
only available in linux
```bash
mkdir build && cd build
cmake .. -DUI=1 -DCMAKE_BUILD_TYPE=release
make
```

binary is in `build/bin`:
 - `liblistener.so` is lib to mock POSIX API and forward the unix-domian-socket message to server
 - `uds-forward` is server run in linux, collect and forward messages from liblistener and store as file or forward to GUI
 - `uds-listener` is cross-platform GUI client to display the message. (now only test in linux)

### How to Use
1. set environment before the want-to-listened program running.
```bash
export UNIX_DOMAIN_SOCKET_FORWARD_IP=127.0.0.1    # target server ip, default localhost
export UNIX_DOMAIN_SOCKET_FORWARD_PORT=12345      # target server port
export LD_PRELOAD=./liblistener.so                # path to liblistener.so
```

2. disable firewall if in rack
```bash
nft delete table ip filter
```

3. open uds-forward
   - --in <input_port>                shall be same as UNIX_DOMAIN_SOCKET_FORWARD_PORT
   - --out <output_port>              used for uds-listener connect
   - --file <local_store_file_path>   used for store local

4. open uds-listener
   - typing IP and Port, click start