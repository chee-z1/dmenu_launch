#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	FILE *in; //Dmenu's stdin
	FILE *out; //Dmenu's stdout
} Dmenu;

typedef struct {
	char *strtok_save;
	char *dirs;
	char *dirname;
	DIR *curdir;
	struct dirent *curent;
} DeskEnt;

int dmenu_init(Dmenu *dmenu, char *argv[])
{
	int in_fd[2];
	int out_fd[2];
	if (pipe(in_fd) == -1)
		return -1;
	if (pipe(out_fd) == -1)
		return -1;
	pid_t pid = fork();
	if (pid == -1)
		return -1;
	if (pid == 0) {
		close(in_fd[0]);
		close(out_fd[1]);
		dmenu->in = fdopen(in_fd[1], "w");
		if (dmenu->in == NULL)
			return -1;
		dmenu->out = fdopen(out_fd[0], "r");
		if (dmenu->out == NULL)
			return -1;
		return 0;
	} else {
		close(in_fd[1]);
		close(out_fd[0]);
		dup2(in_fd[0], STDIN_FILENO);
		dup2(out_fd[1], STDOUT_FILENO);
		if (execvp(argv[0], argv) == -1)
			return errno;
	}
}

int deskent_init(DeskEnt *de) 
{
	de->dirs = getenv("XDG_DATA_DIRS");
	de->strtok_save = NULL;
	de->curdir = NULL;
	de->curent = NULL;
	if (de->dirs == NULL) return -1;
	return 0;
}

char *deskent_next(DeskEnt *de)
{
	while (!(de->curdir && de->curent)) {
		char *dname;
		if (de->strtok_save) 
			dname = strtok_r(NULL, ":", &de->strtok_save);
		else
			dname = strtok_r(de->dirs, ":", &de->strtok_save);
		if (!dname) {
			closedir(de->curdir);
			return NULL; //return NULL on end
		}
		
		if (de->dirname)
			free(de->dirname);
		de->dirname = (char *) malloc(
			strlen(dname) + strlen("/applications/") + 1
			);
		strcpy(de->dirname, dname);
		strcat(de->dirname, "/applications/");
		
		if (de->curdir) 
			closedir(de->curdir);
		de->curdir = opendir(de->dirname);
		if (!de->curdir) 
			continue;
		
		de->curent = readdir(de->curdir);
	}
	struct dirent *curent = de->curent;
	de->curent = readdir(de->curdir);
	char *name = malloc(
		strlen(de->dirname) + strlen(curent->d_name) + 1
		);
	strcpy(name, de->dirname);
	strcat(name, curent->d_name);
	return name;
	//TODO:Make code clean
	//TODO:Filter directories and cache files
}
