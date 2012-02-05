Copy the directories Server and Client to a directory of your choice, say myDir.

Server

1. Open a shell window and change directory to myDir/Server.
2. Issue the command make to compile the server code.
3. Now issue the command myhttpd <http_version> -p<port_number> -t<timeout_period>. The strings in angular brackets are placeholders and the angular brackets MUST not be included in the actual command issued. This will start the server.

Client

1. Open a shell window and change directory to myDir/Client
2. Issue the command make to compile the client code.
3. Now issue the command myclient <host_ip> <host_port_number> <number_of_threads>. The strings in angular brackets are placeholders and the angular brackets MUST not be included in the actual command issued. This will start the client.
