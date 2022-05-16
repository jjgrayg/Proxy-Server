////////////////////////////////////////////////////////
///
///     "utilities.h"
///
///     A collection of utility functions used in various
///     places throughout the program
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////

#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

using std::string; using std::vector;

vector<string> split_string(string, char);
vector<string> split_string(string, char, char);
std::string replace_all(string, const string&, const string&);
std::string get_current_date_and_time(string sep = "~");
double time_since_epoch(std::chrono::high_resolution_clock::time_point* t = nullptr);

#endif // UTILITIES_H
