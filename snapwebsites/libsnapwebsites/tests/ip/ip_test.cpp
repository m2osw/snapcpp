#include "addr.h"
#include "not_used.h"

#include <iostream>

using namespace snap;
using namespace snap_addr;
using namespace std;

int main( int argc, char *argv[] )
{
	NOTUSED(argc);
	NOTUSED(argv);

	addr::vector_t address_list( addr::get_local_addresses() );

	for( const auto& addr : address_list )
	{
        cout << "Network type: ";
		switch( addr.get_network_type() )
		{
            case addr::network_type_t::NETWORK_TYPE_UNDEFINED  : cout << "Undefined";  break;
            case addr::network_type_t::NETWORK_TYPE_PRIVATE    : cout << "Private";    break;
            case addr::network_type_t::NETWORK_TYPE_CARRIER    : cout << "Carrier";    break;
            case addr::network_type_t::NETWORK_TYPE_LINK_LOCAL : cout << "Local Link"; break;
            case addr::network_type_t::NETWORK_TYPE_MULTICAST  : cout << "Multicast";  break;
            case addr::network_type_t::NETWORK_TYPE_LOOPBACK   : cout << "Loopback";   break;
            case addr::network_type_t::NETWORK_TYPE_ANY        : cout << "Any";        break;
            case addr::network_type_t::NETWORK_TYPE_UNKNOWN    : cout << "Unknown";    break;
		}
        cout << endl;

		std::string const ip_string( addr.get_ipv4or6_string() );
		cout << "IP address: " << ip_string;

		if( addr.is_ipv4() )
		{
			cout << " (ipv4)";
		}
        else
        {
            cout << " (ipv6)";
        }
        cout << endl;
	}
}

// vim: ts=4 sw=4 et
