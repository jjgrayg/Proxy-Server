Project 2 - Proxy Server

===========================================================================================
This proxy server is built with the C++11 standard using the QT, sqlite, and Boost libraries, 
so in order to build the project you must have:
	- qmake installed for converting the .pro file into a makefile
	- QT libraries installed on the system
	- Boost libraries installed on the system
	
Before building you must change the library includes marked in the .pro file to point to their
respective directories. The build can then be completed simply by running:
	- qmake ProxyServer.pro
	- make
	
I understand that installing the required libraries may be tedious so there is an included
executable in the "Executable" folder that is compiled to run on a 64-bit Windows machine and
is packaged with the correct .dll files to run.

This server properly responds to the requests at <proxy_ip>/proxy_usage?, 
<proxy_ip>/proxy_usage_reset?, and <proxy_ip>/proxy_log?. Example usage:
	- Assuming we are accessing from the same machine that the proxy is running, and
		the proxy is running on port 8080
	- localhost:8080/proxy_log?

This server also properly handles HTTP CONNECT requests allowing free browsing to even sites that use
HTTPS. There can still be more done with this server (possible implementing SSL, improving the GUI), 
and I plan to improve it, but this is the product up to spec for the project.

==================================================================================
Running

Running the program is as simple a running the executable. The GUI has several options:
	- Debugging: Enables a verbose output on the GUI console
	- Logging: Enables a log file to be stored (log.txt and log_file.csv)
	- Cache Size: The cache size to keep in megabytes
	- Port: The port number to start the proxy on
	
Simply click "Start server" to start the server on the specified port, adjust your browser/OS
settings to use the server, and enjoy!
	