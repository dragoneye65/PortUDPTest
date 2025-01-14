# PortUDPTest
/*
* This program will test if a UDP port is open and accessable from the internet
* -----------------------------------------------------------------------------
* PortUDPTest is using the third party library ASIO (https://think-async.com/Asio/)
* 
* This is what the program does:
* - Gets your public IP address from checkip.amazonaws.com
* - Creats a local server that listens on the specified udp port and wait for the message.
* - Sends a message to the public IP of the router and wait for a response from the server
* - If a response is received, the port is open and accessable from the internet
* - If no response is received, the port is closed and not accessable from the internet
* - The message sent is "Hello, server!"
* 
* Be sure to not run your game/application before running this program,
* if failure to do so, this test will not be conclusive as the port will be occupied
* 
*   Disclaimer: use at own risk!    (c) CMDR Tyroshious (@DragoneEye) January, 2025
*
* Usage: PortUDPTest <port> [-v|--verbose]
* Example: PortUDPTest 12345
* Example: PortUDPTest 12345 -v

    Known issues: Assertion: "cannot dereference string iterator because the iterator was invalidated..." 
    Been looking for it, can't find it. :-/
        Works fine in Release mode, but not in Debug mode.

*/
