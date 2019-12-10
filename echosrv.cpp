/******************************************************************************
Copyright (c) 2019, isaponsoft (Isao Shibuya)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the libamtrs project.
******************************************************************************/

#include <thread>


#ifdef	_MSC_VER

// ========================================================
// for Microsoft Visual C++ / Windows
//	cl.exe echosrv.cpp
// --------------------------------------------------------
#include <winsock2.h>
using	sock_type		= ::SOCKET;
using	sock_len_type	= int;
static constexpr sock_type	invalid_socket	=  INVALID_SOCKET;
void socket_close(sock_type _s) { ::closesocket(_s); }
auto get_error_code() { return WSAGetLastError(); }
void startup() { WSADATA wsaData; WSAStartup(MAKEWORD(2,0), &wsaData); }
void cleanup() { WSACleanup(); }
#pragma comment(lib, "ws2_32.lib")
// --------------------------------------------------------

#else

// ========================================================
// for UNIX like
//	clang++ echosrv.cpp -lpthread
//	gcc++ echosrv.cpp -lpthread
// --------------------------------------------------------
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
using	sock_type		= int;
using	sock_len_type	= socklen_t;
static constexpr sock_type	invalid_socket	=  -1;
void socket_close(sock_type _s) { ::close(_s); }
auto get_error_code() { return errno; }
void startup(){}
void cleanup(){}
// --------------------------------------------------------

#endif



// *********************************************************
// Custom parameters
// ---------------------------------------------------------
static constexpr uint16_t	listen_port			= 9300;  // echo server listen port number.
// ---------------------------------------------------------

void print_error();


int main(int _argc, char** _args)
{
	startup();

	struct sockaddr_in	bindAddr	= {0};
	bindAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	bindAddr.sin_family			= AF_INET;
	bindAddr.sin_port			= htons(listen_port);

	sock_type			server 		= ::socket(PF_INET, SOCK_STREAM, 0);
	if (::bind(server, (struct sockaddr *)&bindAddr, sizeof(bindAddr)))
	{
		print_error();
		return	1;
	}

	if (::listen(server, 5))	// 5 is max client
	{
		print_error();
		return	1;
	}

	while (1)
	{
		struct sockaddr_in	c_addr;
		sock_len_type		len		= sizeof(c_addr);
		sock_type			client	= ::accept(server, (struct sockaddr *)&c_addr, &len);
		if (client == invalid_socket)
		{
			print_error();
			break;
		}

		std::thread	t([client]()
		{
			int		siz;
			char	buff[1460];				// buffer size >= MSS
			while ((siz = ::recv(client, buff, sizeof(buff), 0)) > 0)
			{
				::send(client, buff, siz, 0);
			}
			socket_close(client);
		});
		t.detach();
	}

	socket_close(server);

	cleanup();
	return	0;
}



// ********************************************************
// Error message.
// --------------------------------------------------------

#include <iostream>

void print_error()
{
	auto	ec	= std::error_code(get_error_code(), std::system_category());
	auto	msg	= ec.message();
	std::cout << msg << std::endl;
}

