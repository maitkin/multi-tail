#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <netdb.h>
#include <libgen.h>
#include <inttypes.h>
#include "Util.hpp"
#include "Log.hpp"


#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define EVENTS		(IN_MODIFY|IN_DELETE_SELF|IN_MOVE_SELF|IN_UNMOUNT)
#define NEVENTS		4


using namespace std;

class Tail {


private:
	string FilePath;
	string pattern;
	string baseFileName;
	string baseDirectory;
	bool suppressFilename;

public:
	Tail(string path, string _pattern, bool suppress_filename) {

		TRACE("Tail(%s,%s)\n",path.c_str(),_pattern.c_str());
		FilePath = path;
		pattern = _pattern;
		suppressFilename = suppress_filename;

		// extract base name,dir
		char _path[strlen(path.c_str())],_dir[strlen(path.c_str())];
		strcpy(_path,path.c_str());
		strcpy(_dir,path.c_str());
		baseFileName = string(basename(_path));
		baseDirectory = string(dirname(_dir));

		TRACE("%s\n",FilePath.c_str());
	}

	void start() {
		while (1) {
			waitForFileToAppear();
		}
	}

		
protected:


	void waitForFileToAppear() {


		// file exists
		if( access( FilePath.c_str(), F_OK ) == 0 ) {
			TRACE("file exists\n");
			tailFile();
		}
		else {
				
			TRACE("waiting for file to appear\n");
			int i=0;
			char buffer[1024 * (sizeof (struct inotify_event) + 16 ) ];
			int id = inotify_init();

			if ( id < 0 ) { perror( "inotify_init" ); }

			int wd = inotify_add_watch( id, baseDirectory.c_str(), IN_CREATE | IN_ONESHOT);
			
			int length = read( id, buffer, EVENT_BUF_LEN ); 

			if ( length < 0 ) { perror( "read" );	}  		
		
			while ( i < length) {     
					
				struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];     			
				if ( event->len && (( event->mask & IN_CREATE && (!(event->mask & IN_ISDIR))))) {							
					TRACE( "noticed %s \n", event->name );
					if (baseFileName.compare(event->name)) {
						break;
					}
				}
				i += EVENT_SIZE + event->len;
			}
		}
	}
	



	// Seeks <size> of file since being watched and prints rest of file to stdout
	void rollFile(const char *filepath, off_t *size) {

		char buf[BUFSIZ];
		int fd;
		struct stat st;
		off_t pos;
				
				
		fd = open(filepath, O_RDONLY);
				
		if (fd < 0)
			perror("cannot open %s");
				
		if (fstat(fd, &st) == -1)
			perror("file doesn't exist %s");
				
		if (st.st_size == *size) {
			close(fd);
			return;
		}
				
		if (lseek(fd, *size, SEEK_SET) != (off_t)-1) {

			ssize_t rc, wc;
						
			while ((rc = read(fd, buf, sizeof(buf))) > 0) {
								
				if (pattern.empty()) {

					if (!suppressFilename) {
						printf("\033[0;31m%s:\033[0m",baseFileName.c_str());
					}
					fflush(stdout);
					wc = write(STDOUT_FILENO, buf, rc);
				}
				else {
										
					vector<string> lines = split(buf, '\n');
										
					for (vector<string>::iterator it = lines.begin(); it != lines.end(); ++it) {
						if ( strstr((*it).c_str(), pattern.c_str()) != NULL) {
							if (!suppressFilename) {
								printf("\033[0;31m%s\033[0m:",baseFileName.c_str());
							}
							printf("%s\n",(*it).c_str());
						}
					}
				}								
			}
			fflush(stdout);
		}

		pos = lseek(fd, 0, SEEK_CUR);

		/* If we've successfully read something, use the file position, this
		 * avoids data duplication. If we read nothing or hit an error, reset
		 * to the reported size, this handles truncated files.
		 */
		*size = (pos != -1 && pos != *size) ? pos : st.st_size;

		close(fd);
	}



	// Waits for file to be changed, then calls rollFile
	int tailFile() {
	
		TRACE("tailFile()\n");

		struct stat file_stat;

		if (stat(FilePath.c_str(), &file_stat) != 0) {
			perror("fstat failed");
			return -1;
		}


		char buf[ NEVENTS * sizeof(struct inotify_event) ];
		int fd, ffd, e;
		ssize_t len;

		fd = inotify_init();
		if (fd == -1)
			return 0;

		ffd = inotify_add_watch(fd, FilePath.c_str(), EVENTS);
		if (ffd == -1) {
			if (errno == ENOSPC)
				perror("%s: cannot add inotify watch (limit of inotify watches was reached)");
		}

		while (ffd >= 0) {

			TRACE("watching %s file\n",FilePath.c_str());

			len = read(fd, buf, sizeof(buf));

			TRACE("%s changed\n",FilePath.c_str());

			if (len < 0 && (errno == EINTR || errno == EAGAIN)) {
				continue;
			}

			if (len < 0)
				perror("%s: cannot read inotify events");

			for (e = 0; e < len; ) {

				struct inotify_event *ev = (struct inotify_event *) &buf[e];
			
				if (ev->mask & IN_MODIFY) {
					rollFile(FilePath.c_str(), &file_stat.st_size);
				}
				else if (ev->mask & IN_MOVE_SELF || ev->mask & IN_DELETE_SELF) {
					TRACE("%s has disappeared\n",FilePath.c_str());
					inotify_rm_watch(fd,ffd);
					close(fd);
					return 1;
				}
				else {
					inotify_rm_watch(fd,ffd);
					break;
				}
				e += sizeof(struct inotify_event) + ev->len;
			}
		}
		close(fd);
		return 1;
	}



};

