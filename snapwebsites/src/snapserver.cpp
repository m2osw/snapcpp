// Snap Websites Server -- snap websites server
// Copyright (C) 2011-2013  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "snapwebsites.h"


int main(int argc, char *argv[])
{
	// create a server object
	snap::server::pointer_t s( snap::server::instance() );

	// parse the command line arguments
	s->config(argc, argv);

	// if possible, detach the server
	s->detach();
	// Only the child (server) process returns here

	// prepare the database
	s->prepare_cassandra();

	// listen to connections
	s->listen();

#if 0
	// load all the plugins
	if(!snap::plugins::load(s->get_parameter("plugins"))) {
std::cerr << "quitting early...\n";
		return 254;
	}
std::cerr << "loaded anything?!\n";

	// now boot the system
	snap::server::instance()->bootstrap();
	snap::server::instance()->init();
	snap::server::instance()->execute("/robots.txt");

	QString html = "<p STYLE='font-style: bold;'>tags <div><a href=\"filter\" target='_top' style='color: purple;'>filtering</a><!-- upper test --><A>UPPER</A><script type='text/javascript'>a = 25;</script></div> with <ul class='menu\"test\"'><li>Item 1</li><li>Item 2</li></ul></p>";
	QDomDocument doc;
	QString err;
	int line;
	int column;
	if(doc.setContent(html, true, &err, &line, &column)) {
		std::cerr << "PARSING ACCEPTED! [" << doc.toString().toUtf8().data() << "]\n";
		snap::server::instance()->xss_filter(doc, "a p ul li", "!style");
		std::cerr << "FINAL OUTPUT! [" << doc.toString().toUtf8().data() << "]\n";
	}
	else {
		std::cerr << "PARSING ERROR!" << err.toUtf8().data() << " at line " << line << " column " << column << "\n";
	}
#endif
#if 0
// text the XSS filter
	htmlcxx::HTML::ParserDom parser;
	std::string html = "<p STYLE='font-style: bold;'>tags <a href=\"filter\" blank target='_top' style='color: purple;'>filtering</a><A>UPPER</A><script type='text/javascript'>a = 25;</script> with <ul class='menu\"test\"'><li>Item 1</li><li>Item 2</li></ul></p>";
	parser.parse(html);
	htmlcxx::tree<htmlcxx::HTML::Node> node = parser.getTree();
std::cerr << "call XSS filter...\n";
	snap::server::instance()->xss_filter(node, "a p ul li", "!style");
snap::tree2html::to_html n(node);//*node.child(node.begin(), 0));
std::cerr << "text returned = [" << n << "]\n";
#endif

	return 0;
}

// vim: ts=4 sw=4
