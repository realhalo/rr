/* [rr] rr.c :: retain / recall file and directory paths.
** Copyright (C) 2007 fakehalo [v9@fakehalo.us]
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**/

#include "rr.h"

static const char id[] = "$Id: rr.c,v " RR_VERSION " " RR_CMPTIME " fakehalo Exp $";


/* start process. */
int main(signed int argc, char **argv) {
	signed int fd;
	unsigned int len;
	char buf[RR_MAXPATH + 1], *ptr, *eptr;
	FILE *fp;
	struct rr_select_struct rss;

	/* referenced for rr_error(). */
	prog = argv[0];

	/* get absolute path of "~/.rr". */
	ptr = rr_rr_filename();

	/* open / create "~/.rr". */
	if((fd = open(ptr, O_RDWR | O_CREAT, 0600)) < 0 || !(fp = fdopen(fd, "r+")))
		rr_error("failed to open: %s", ptr);

	/* done with the absoulte "~/.rr" filename, ptr will be reused. */
	free(ptr);
	ptr = 0;

	/* read from stdin, then treat it as a retain/recall entry. (non-exec) */
	if(argc == 1) {
		memset(buf, 0, RR_MAXPATH + 1);
		if(read(STDIN_FILENO, buf, RR_MAXPATH) > 0) {
			ptr = buf;
			rr_chomp(ptr);
		}

		/* nothing was read or error. */
		else exit(0);
	}

	/* execute command. using rr entries. (//entry) */
	else if(argc > 2 || (argc == 2 && strlen(argv[1]) > 2 && argv[1][0] == '/' && argv[1][1] == '/'))

		/* exits from inside the function. */
		rr_exec(argc, argv, fd, fp);

	/* retain/recall rr entry, and print it to stdout. */
	if(argc <= 2) {

		/* if stdin set this, don't take argv[1]. */
		if(!ptr) ptr = argv[1]; 

		/* nothing? abort. */
		if(!*ptr) exit(0);

		/* print: retain (and delete old) a new rr entry. */
		if(ptr[0] == '/') {

			/* wouldn't "break" rr, but it would hurt functionality if unchecked. */
			if(strlen(ptr) > RR_MAXPATH)
				rr_error("failed to retain: %.*s (path too long)", RR_MAXPATH, ptr);

			/* don't use non-existent files. */
			else if(access(ptr, F_OK))
				rr_error("failed to retain: %s (non-existent or non-accessible file)", ptr);

			/* clean-up the possible sloppy path. */
			eptr = rr_fix(ptr);

			/* check and see if there is an entry */
			rss = rr_select(fd, fp, rr_basename(eptr, 0), 0, RR_STRICT);

			/* there's an old entry, remove it from the file. */
			if(rss.off >= 0)
				rr_delete(fd, rss.off, rss.sz);

			/* insert the (new) value. */
			rr_insert(fd, rss.off, eptr);

			/* just to be tidy. */
			free(eptr);
			eptr = 0;

			/* verbose output for misc. uses. */
			puts(ptr);
		}

		/* print: recall a rr entry. */
		else {

			/* see if there is any extra trailing stuff to append to directories. */
			if((eptr = strchr(ptr, '/')) && strlen(eptr) > 1) {
				eptr++;
				len = eptr - ptr;
			}
			else len = 0;

			/* check and see if there is an entry */
			rss = rr_select(fd, fp, ptr, len, RR_STRICT);

			/* no entry? try again, this time not strict. */
			if(rss.off < 0) rss = rr_select(fd, fp, ptr, len, RR_PARTIAL);

			/* there's an entry, print it. (and extra parts, if they exist) */
			if(rss.off >= 0) {
				if(len) printf("%s%s\n", rss.path, eptr);
				else puts(rss.path);
			}

			/* even if no match, verbose output for misc. uses. */
			else puts(ptr);
		}
	}

	/* clean-up a bit. */
	fclose(fp);
	close(fd);

	fflush(stdout);

	exit(0);
}

/* print an error and exit. */
void rr_error(char *fmt, ...) {
	char buf[RR_MSGSIZE + 1];
	va_list ap;
	memset(buf, 0, RR_MSGSIZE + 1);
	va_start(ap, fmt);
	vsnprintf(buf, RR_MSGSIZE, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s: %s\n", prog, buf);
	exit(1);

	/* doesn't make it here. */
	return;
}

/* like basename(), except allows trailing slashes for directories. (no free()) */
char *rr_basename(char *file, unsigned int len) {
	char *ptr;
	for(ptr = file + (len ? len : strlen(file) - 2); file <= ptr; ptr--) {
		if(*ptr == '/') {
			ptr++;
			break;
		}
	}

	/* possibly never did a single instance of the for() loop, and *ptr could be before *file. */
	return((file > ptr ? file : ptr));
}

/* fix inappropriate (lack of) trailing slash for retaining files and directories. */
char *rr_fix(char *path) {
	unsigned int len;
	char *fix, *ptr, *sptr;
	struct stat s;

	/* allocate a max of +1 for an extra slash if needed. (other +1 for null-byte) */
	if(!(fix = (char *)malloc(strlen(path) + 2)))
		rr_error("failed to allocate memory.");
	memset(fix, 0, strlen(path) + 2);
	strcpy(fix, path);

	/* some sanity. */
	if(!strlen(fix) || !strcmp(fix, "/")) return(fix);

	/* remove consecutive slashes. ("./", "../", and symlinks are allowed) */
	for(ptr = fix, sptr = 0; *ptr; ptr++) {

		/* slash start. */
		if(*ptr == '/' && !sptr) sptr = ptr;

		/* (consecutive) slash end. */
		else if(*ptr != '/' && sptr) {

			/* more than 1 trailing slash, move some memory around. */
			if((ptr - sptr) > 1) {
				sptr++;

				/* move it back. */
				memmove(sptr, ptr, strlen(ptr));				

				len = ptr - sptr;
				ptr += strlen(ptr) - len;

				/* null out the old (unused) space. */
				memset(ptr, 0, len);

				/* memory has been moved, go back a bit. */
				ptr = sptr;
			}
			sptr = 0;
		}
	}

	/* take off any tailing slashes, even if it's just one--will be re-added below. */
	if(sptr) *sptr = 0;

	/* if it was "//" for example, it would be null now, make sure it gets at least a '/'. */
	/* check existence and add '/' if it's a directory. */
	if(!strlen(fix) || (stat(fix, &s) >= 0 && (s.st_mode & S_IFMT) == S_IFDIR)) strcat(fix, "/");

	/* good to go. */
	return(fix);
}

/* modify a given string to take out the trailing '\r' and '\n' */
void rr_chomp(char *str) {
	char *ptr;
	if((ptr = strchr(str, '\r'))) *ptr = 0;
	else if((ptr = strchr(str, '\n'))) *ptr = 0;
	return;
}

/* add an entry to "~/.rr". */
void rr_insert(signed int fd, off_t off, char *path) {

	/* make sure we're at the end of the file. */
	if(lseek(fd, 0, SEEK_END) < 0)
		rr_error("lseek() failed processing ~/" RR_FILENAME ".");

	/* insert the new path at the end of the file. */
	if(write(fd, path, strlen(path)) < 0 || write(fd, "\n", 1) < 0)
		rr_error("write() failed processing ~/" RR_FILENAME ".");

	/* go back where we were, if we were anywheres. */
	if(off >= 0 && lseek(fd, off, SEEK_SET) < 0)
		rr_error("lseek() failed processing ~/" RR_FILENAME ".");

	return;
}

/* delete an entry from "~/.rr". */
void rr_delete(signed int fd, off_t off_start, size_t l_sz) {
	off_t off;
	ssize_t sz;
	char mvbuf[RR_MVBLOCK + 1];

	/* we want to start directly after the entry. */
	off = off_start + l_sz;
	if(lseek(fd, off, SEEK_SET) < 0)
		rr_error("lseek() failed processing ~/" RR_FILENAME ".");

	/* start reading from the point after the entry we're removing. */
	while((sz = read(fd, mvbuf, RR_MVBLOCK)) > 0) {

		/* write backwards over the old entry. */
		if(lseek(fd, -(l_sz + sz), SEEK_CUR) < 0)
			rr_error("lseek() failed processing ~/" RR_FILENAME ".");
		if(write(fd, mvbuf, sz) < 0)
			rr_error("write() failed processing ~/" RR_FILENAME ".");

		/* go forward a bit for the next read. */
		if(lseek(fd, l_sz, SEEK_CUR) < 0)
			rr_error("lseek() failed processing ~/" RR_FILENAME ".");

		/* used to truncate the file with. */
		off += sz;
	}

	/* cut the extra space out the file now that everything has been moved up. */
	/* they say this isn't POSIX, but i haven't seen a (modern) system without it. */
	if(ftruncate(fd, (off_t)(off - l_sz)) < 0)
		rr_error("ftruncate() failed processing ~/" RR_FILENAME ".");

	return;
}

/* find the offset of the file in  "~/.rr". */
struct rr_select_struct rr_select(signed int fd, FILE *fp, char *file, unsigned int len, unsigned char type) {
	unsigned int l;
	size_t sz;
	char buf[RR_MAXPATH + 1], *ptr;
	struct rr_select_struct rss;

	rss.off = -1;
	rss.sz = -1;

	memset(rss.path, 0, RR_MAXPATH + 1);
	memset(buf, 0, RR_MAXPATH);

	/* just incase we're not at the start. */
	if(fseek(fp, 0, SEEK_SET) < 0)
		rr_error("fseek() failed processing ~/" RR_FILENAME ".");

	while(fgets(buf, RR_MAXPATH, fp) != NULL) {
		sz = strlen(buf);

		/* theoretically at least "/x" would be needed to be anything. */
		if (sz < 2 || buf[0] == '#' || buf[0] == ';') continue;

		rr_chomp(buf);
		ptr = rr_basename(buf, 0);

		/* match the file/path up until ?? bytes. (0 = length of *file) */
		if(len < 1) {

			/* full length of string. (strict, and more safe) */
			if(type == RR_STRICT) l = (strlen(file) > strlen(ptr) ? strlen(file) : strlen(ptr));

			/* length of the provided argument to recall with. (short-hand/partial) */
			else l = strlen(file);
		}
		else l = len;

		/* matched requested file, delete from "~/.rr". */
		if(!strncmp(ptr, file, l)) {
			rss.sz = sz;
			rss.off = ftell(fp) - rss.sz;

			/* buffers are the same size, no worries. */
			strcpy(rss.path, buf);

			/* return. */
			break;
		}
	}
	return(rss);
}

/* execute a program with processed //paths. */
void rr_exec(signed int argc, char **argv, signed int fd, FILE *fp) {
	unsigned int i, len, pargc;
	char **pargv, *ebuf, *ptr, *eptr;
	struct rr_select_struct rss;

#ifdef HAVE_GLOB
	signed int gr;
	unsigned int j, eargc;
	glob_t g;
#endif

	pargc = 0;

	/* losing the first argument, but we want to cap it off. */
	if(!(pargv = (char **)malloc(sizeof(char *) * (argc + 1))))
		rr_error("failed to allocate memory.");

	for(i = 1, ptr = 0, ebuf = 0; i < argc; i++, ptr = 0) {

		/* check and see if there is an entry for this argument. (rr_basename will chop down the rest) */
		if(strlen(argv[i]) > 2 && argv[i][0] == '/' && argv[i][1] == '/') {

			/* trailing directory stuff to glob? mark it in a pointer/len. */
			if((eptr = strchr(argv[i] + 2, '/')) && strlen(eptr) > 1) {
				eptr++;
				len = eptr - argv[i] - 2;
			}
			else len = 0;

			/* see if we have an antry to recall. */
			rss = rr_select(fd, fp, rr_basename(argv[i], len), len, RR_STRICT);

			/* nothing was found strictly, try again without being strict. */
			if(rss.off < 0) rss = rr_select(fd, fp, rr_basename(argv[i], len), len, RR_PARTIAL);

			/* we have a match. */
			if(rss.off >= 0) {

				/* this may carry over to past this block of code. */
				ptr = rss.path;

				/* trailing directory stuff to glob? do it now. */
				if(len) {

					/* make a buffer for the extra/extended path parts. (to glob or append) */
					if(!(ebuf = (char *)malloc(strlen(ptr) + strlen(eptr) + 1)))
						rr_error("failed to allocate memory.");
					memset(ebuf, 0, strlen(ptr) + strlen(eptr) + 1);
					strcpy(ebuf, ptr);
					strcat(ebuf, eptr);
					ptr = ebuf;
#ifdef HAVE_GLOB
					gr = glob(ebuf, RR_GLOB_FLAGS, NULL, &g);

					if(!gr && g.gl_pathc) {

						/* space has already been allotted for 1 element, only realloc() for >1. */
						if(g.gl_pathc > 1) {
							eargc += g.gl_pathc - 1;
							if(!(pargv = (char **)realloc(pargv, sizeof(char *) * (argc + eargc + 1))))
								rr_error("failed to re-allocate memory.");
						}

						for(j = 0; j < g.gl_pathc; j++) {
							if(!(pargv[pargc] = (char *)malloc(strlen(g.gl_pathv[j]) + 1)))
								rr_error("failed to allocate memory.");
							memset(pargv[pargc], 0, strlen(g.gl_pathv[j]) + 1);
							strcpy(pargv[pargc++], g.gl_pathv[j]);
						}

						/* free it up. */
						free(ebuf);
						globfree(&g);

						/* break out of this instance or it will add extra args. */
						continue;
					}
#endif /* HAVE_GLOB */
				}
				/* ... go below to use the ptr set initially in this code block. */				
			}
		}

		/* we don't have a match for anything, use the normal argument. */
		if(!ptr) ptr = argv[i];

		/* allocate/copy our argument. */
		if(!(pargv[pargc] = (char *)malloc(strlen(ptr) + 1)))
			rr_error("failed to allocate memory.");
		memset(pargv[pargc], 0, strlen(ptr) + 1);
		strcpy(pargv[pargc++], ptr);

		/* free the extra path parts. */
		if(ebuf) free(ebuf);
	}

	/* cap it off. */
	pargv[pargc] = 0;

	/* won't be needing these where you're going. */
	fclose(fp);
	close(fd);

	/* off she goes. */
	execvp(pargv[0], pargv);

	/* shouldn't make it here. */
	rr_error("failed to execute: %s", pargv[0]);

	/* won't make it here. */
	return;
}

/* create absolute "~/.rr" filename. */
char *rr_rr_filename() {
	char *buf, *ptr;
	struct passwd *pwd;

	/* use $RR_HOME/$HOME first. */
	if(!(ptr = getenv("RR_HOME")) && !(ptr = getenv("HOME"))) {

		/* no $RR_HOME/$HOME, use getpwuid() then. */
		if(!(pwd = getpwuid(getuid()))) rr_error("no $RR_HOME, $HOME, or passwd information found.");
		else ptr = pwd->pw_dir;
	}

	/* make the absolute "~/.rr". */
	if(!(buf = (char *)malloc(strlen(ptr) + strlen(RR_FILENAME) + 2)))
		rr_error("failed to allocate memory.");
	memset(buf, 0, strlen(ptr) + strlen(RR_FILENAME) + 2);
	sprintf(buf, "%s/%s", ptr, RR_FILENAME);

	return(buf);
}
