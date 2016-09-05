
#include <iostream>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char * argv[])
{
	if(argc == 1)
	{
		std::cerr << "usage: gethostips <name>" << std::endl;
		exit(1);
	}

	struct hostent * h (gethostbyname(argv[1]));

	std::cerr << "got name \"" << h->h_name << "\", type " << h->h_addrtype << ", length " << h->h_length << std::endl;

	for(char **a(h->h_addr_list); *a != nullptr; ++a)
	{
		char buf[256];
		std::cerr << "  IP: " << inet_ntop(h->h_addrtype, *a, buf, sizeof(buf)) << "\n";
	}

	return 0;
}

