/*
 * main.cpp
 *
 *  Created on: Dec 17, 2012
 *      Author: akavo
 */

#include <stdio.h>
#include <string>
#include <fstream>
#include "boost/program_options.hpp"

#include "FileTagger.h"
#include "common.h"

namespace po = boost::program_options;



int main(int argc, char **argv) {
	std::string c_directory;
	std::string c_pattern;
	std::vector<std::string> c_empty_v;
	bool c_trim = false, c_safe = false,  c_recursive = false;
	/////////
	std::string prog = "Tag Mp3 files from filename";
	po::options_description desc(prog);

	//Add options
	desc.add_options()	("help,h", "this message")
						("pattern,p", po::value<std::string>(), "pattern to match")
						("recursive,r", "recursive iteration")
						("trim,t", "remove leading and trailing space from fields")
						("safe,s", "safe mode, do not update files")
						("empty,e", po::value<std::vector<std::string> >(), "only update tags if the tag specified with this option is initially empty")
						("directory,d", po::value<std::string>(&c_directory)->required(), "path to folder (required)");

	po::positional_options_description positionalOptions;
	positionalOptions.add("directory", 1);

	po::variables_map vm;

	try {
		po::store(po::command_line_parser(argc, argv).options(desc)
		                  .positional(positionalOptions).run(),
		                vm); // throws on error

		po::variables_map parameters;
		if (vm.count("pattern")) {
			c_pattern = vm["pattern"].as<std::string>();
		}
		if(vm.count("recursive")) {
			c_recursive = true;
		}
		if (vm.count("trim")) {
			c_trim = true;
		}
		if (vm.count("safe")) {
			c_safe = true;
		}
		if (vm.count("empty")) {
			c_empty_v = vm["empty"].as< std::vector<std::string> >();
			for(std::vector<std::string>::iterator it = c_empty_v.begin(); it!=c_empty_v.end(); ++it) {
				if(!isField(*it)) {
					std::string error = *it + " is not a valid field";
					throw Exc(error);
				}
			}
		}
		if (vm.count("help")) {
			// depending on the help param or version param do the appropriate things.
			std::cout << "Usage: options_description [options]\n";
			std::cout << desc;
		}
		po::notify(vm);

		// Execute here
		Pattern p(c_pattern, c_trim);
		p.print();
		FileTagger tagger(p);
		tagger.SetEmptyFieldConstraint(c_empty_v);
		tagger.SetSafeMode(c_safe);
		tagger.Tag(c_directory, c_recursive);
		//
	} catch (std::exception& e) {
		if (!vm.count("help")) {
			std::cerr << "Error: " << e.what() << std::endl;
			std::cerr << "Try -h or --help for help." << std::endl;
			return -1;
		}

	}

	return 0;
}

