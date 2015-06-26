#pragma once

#include <vector>
#include <string>

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




