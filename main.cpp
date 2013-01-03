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


int main(int argc, char **argv) {
	tstring c_directory;
	tstring c_pattern;
	tstring c_trim_chars;
	std::vector<tstring> c_empty_v;
	bool c_trim = false, c_safe = false,  c_recursive = false;
	/////////
	std::string prog = "Tag Mp3 files from filename";
	po::options_description desc(prog);

	//Add options
	desc.add_options()	("help,h", "this message")
						("pattern,p", po::tvalue<tstring>(), "pattern to match")
						("recursive,r", "recursive iteration")
						("trim,t", po::tvalue<tstring>()->implicit_value(_T(" "), " "), "remove leading and trailing space from fields")
						("safe,s", "safe mode, do not update files")
						("empty,e", po::tvalue<std::vector<tstring> >(), "only update tags if the tag specified with this option is initially empty")
						("directory,d", po::tvalue<tstring>(&c_directory)->required(), "path to folder (required)");

	po::positional_options_description positionalOptions;
	positionalOptions.add("directory", 1);

	po::variables_map vm;

	try {
		po::store(po::command_line_parser(argc, argv).options(desc)
		                  .positional(positionalOptions).run(),
		                vm); // throws on error

		po::variables_map parameters;
		if (vm.count("pattern")) {
			c_pattern = vm["pattern"].as<tstring>();
		}
		if(vm.count("recursive")) {
			c_recursive = true;
		}
		if (vm.count("trim")) {
			c_trim = true;
			c_trim_chars = vm["trim"].as<tstring>();
		}
		if (vm.count("safe")) {
			c_safe = true;
			tcout << "Safe mode is on" << std::endl;
		}
		if (vm.count("empty")) {
			c_empty_v = vm["empty"].as< std::vector<tstring> >();
			for(std::vector<tstring>::iterator it = c_empty_v.begin(); it!=c_empty_v.end(); ++it) {
				if(!isField(*it)) {
					tstring error = *it + _T(" is not a valid field");
					std::string err(error.begin(), error.end());
					throw Exc(err);
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
		p.SetTrimChars(c_trim_chars);
		p.print();
		FileTagger tagger(p);
		tagger.SetEmptyFieldConstraint(c_empty_v);
		tagger.SetSafeMode(c_safe);
		tagger.Tag(c_directory, c_recursive);
		//
	} catch (std::exception& e) {
		if (!vm.count("help")) {
			tcerr << "Error: " << e.what() << std::endl;
			tcerr << "Try -h or --help for help." << std::endl;
			return -1;
		}

	}

	return 0;
}

