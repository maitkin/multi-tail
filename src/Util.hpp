#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

using namespace std;

#define MAX_HOSTNAME_SIZE 255


vector<string> split(const char *str, char c = ' ') {
		vector<string> result;

		do {
				const char *begin = str;
        while(*str != c && *str)
            str++;
        result.push_back(string(begin, str));
    } while (0 != *str++);

    return result;
}


std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

