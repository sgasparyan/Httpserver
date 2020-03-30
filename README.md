This program is a single threaded HTTP server with caching. 
The makefile in this directory builds an executable fill
called httpserver. To run the makefile write "make" in the command line.
Then to run the program write
./httpserver host port -c -l logFile
this program will run a server connected to Port port and IP address host.
The program will throw errors if no host. If only a host is specified
the server will connect to port 80. The -c flag notifies to server if caching
should be used or not. The -l flag notifies the server if it needs to
implement logging or not.
