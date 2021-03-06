#include "lwc_builtins.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <string>
#include <chrono>
#include <sstream>
#include "lwc_Parser.h"

int main(int argc, char* argv[])
{
	auto start = std::chrono::high_resolution_clock::now();
	std::string file_name = (argc > 1 ? argv[1] : "first_test.lwc");
	std::fstream fs;
	std::string s;
	std::vector<std::string> words;

	std::cout << "Reading " << file_name << "...." << std::endl;

	fs.open(file_name.c_str());
	if (fs.good()) {
		std::string line;
		std::stringstream buffer;
		buffer << fs.rdbuf();
		auto glob = lwc::build_linktree(buffer.str());
		auto blocks = lwc::block_map;
		
		while (std::getline(buffer, line)) {
			words.push_back(line);
		}
		fs.flush();
		fs.close();

		lwc::Scope global;
		std::unordered_map<std::string, lwc::variable> my_varmap;
		lwc::CodeBlock root_scope(lwc::parse_from_slines(words, global));
		std::cout << "build complete" << std::endl;

		auto start_eval = std::chrono::high_resolution_clock::now();
		lwc::evaluate_lines(root_scope);
		auto end = std::chrono::high_resolution_clock::now();
		auto evaltime = (end - start_eval);
		auto fulltime = (end - start);
		std::cout << "Done Evaluating\n" << std::endl;
		std::cout << "FULL RUNNING TIME: " << std::chrono::duration<double, std::milli>(fulltime).count() << std::endl;
		std::cout << "BUILDING TIME: " << std::chrono::duration<double, std::milli>(fulltime - evaltime).count() << std::endl;
		std::cout << "BUILT EVALUATION TIME: " << std::chrono::duration<double, std::milli>(evaltime).count() << std::endl;

		std::cout << std::endl;
	}
	else {
		std::cout << "File: " << file_name << " could not be opened." << std::endl;
	}
	return 0;
}