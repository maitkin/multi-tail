#include <iostream>
#include <pthread.h>
#include <vector>
#include <limits.h>

#include "Tail.hpp"
#include "Util.hpp"
#include "Log.hpp"
using namespace std;


class thread_args {
public:
	thread_args(string one,string two, bool three) {
		filename = one;
		pattern = two;
		suppress = three;
	}
	string pattern, filename;
	bool suppress;
};

void usage() {
	printf("usage: ");
	printf("mtail [OPTION]\n");
	printf("\n");
	printf("Options:\n");
	printf("-f\tPath to file to watch, append an optional :PATTERN to only return matches\n");
	printf("-s\tSuppress filename prepending\n");
	printf("-r\tReads and uses a list of path:patterns from given file\n");
	printf("-h\tHelp - this message\n");
	printf("\n\n");
	printf("Examples:\n");
	printf("Tail two files:\n");
  printf("mtail -f /var/log/messages -f /var/log/secure\n\n");
	printf("Tail two files returning only matches for the given pattern\n");
  printf("mtail -f /var/log/messages:audit -f /var/log/secure:disconnect\n\n");
	exit(1);
}


// Entry point for new threads
void *tailFile(void *_args) {
	thread_args *args = (thread_args*)_args;
	Tail tail( args->filename, args->pattern, args->suppress);
	tail.start();
}

int main (int argc, char **argv) {

	int opt;
	vector<string> patterns;
	string configFile;
	vector<thread_args> threadArguments;
	bool help=false;
	bool suppress_filename=false;


	while ((opt = getopt(argc, argv, "f:r:hs")) != -1) {
		switch(opt) {

		case 'h':
			usage();
			return 0;
			break;

		case 's':
			suppress_filename=true;
			break;

		case 'r':
			configFile = string(optarg);
			break;
			
		case 'f':
			patterns.push_back(optarg);
			break;
			
		default:
			printf("Unrecognized option!\n");
			usage();
			break;
		}
	}
	

	if (!configFile.empty()) {

		struct stat file_stat;
		if (stat(configFile.c_str(), &file_stat) != 0) {
			perror("couldn't open configuration file");
			return -1;
		}

		if (file_stat.st_size > SSIZE_MAX) {
			perror("configuration file is too large to process");
			return -1;
		}

		int fd = open(configFile.c_str(),O_RDONLY);

		if (fd == -1) {
			perror("couldn't open configuration file");
			return -1;
		}

		char buf[file_stat.st_size];
		int b = read(fd,&buf[0],file_stat.st_size);
		
		if (b == 0) {
			fprintf(stderr,"configuration file is empty!\n");
			return -1;
		}

		string config_content(buf);
		vector<string> config_lines = split(config_content.c_str(),'\n');
		for (vector<string>::iterator it = config_lines.begin(); it != config_lines.end(); ++it) {
			patterns.push_back(*it);
		}

	}


	for (vector<string>::iterator it = patterns.begin(); it != patterns.end(); ++it) {

		vector<string> file_and_pattern = split(it->c_str(),':');

				if (file_and_pattern.size() == 0) {
					perror("error parsing file and pattern");
				}
				else if (file_and_pattern.size() == 1) {
					threadArguments.push_back(thread_args(rtrim(file_and_pattern[0]),string(""),suppress_filename));
				}
				else {
					threadArguments.push_back(thread_args(rtrim(file_and_pattern[0]),
																								file_and_pattern[1],suppress_filename));
				}
	}
 
	if (threadArguments.size() == 0) usage();


	vector<pthread_t*> mthreads;

	int index=0;
	for (vector<thread_args>::iterator it = threadArguments.begin(); it != threadArguments.end(); ++it) {		
		pthread_t *t = (pthread_t*)malloc(sizeof(pthread_t));
		pthread_create(t,NULL,tailFile,&(*it));
		mthreads.push_back(t);
		index++;
	}						
	
	for (vector<pthread_t*>::iterator it = mthreads.begin() ; it != mthreads.end(); ++it) {
		if(pthread_join(*(*it),NULL)) {
			fprintf(stderr, "Error joining thread\n");
			return 2;			
		}
	}

	for (vector<pthread_t*>::iterator it = mthreads.begin() ; it != mthreads.end(); ++it) {
		free (*it);
	}
	
	return 0;	
}

