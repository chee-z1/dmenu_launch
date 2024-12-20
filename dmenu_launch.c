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
	char *dir_names;
	char *current_dir_name;
	char *_saveptr;
	DIR *current_dir;
	struct dirent *current_dirent;
	int is_end;
} Dirs;

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

int dirs_init(Dirs *dirs)
{
	dirs->dir_names = getenv("XDG_DATA_DIRS");
	dirs->current_dir_name = NULL;
	dirs->_saveptr = NULL;
	dirs->current_dir = NULL;
	dirs->is_end = 0;
	if (!dirs->dir_names) return -1;
}

DIR *_dirs_next_dir(Dirs *dirs) 
{
	if (dirs->current_dir) 
		closedir(dirs->current_dir);
	if (dirs->current_dir_name)
		free(dirs->current_dir_name);

	char *dir_name;
	if (dirs->_saveptr)
		dir_name = strtok_r(NULL, ":", &dirs->_saveptr);
	else
		dir_name = strtok_r(dirs->dir_names, ":", &dirs->_saveptr);

	if (!dir_name) {
		dirs->is_end = -1;
		return NULL;
	}

	dirs->current_dir_name = (char *)malloc(
		strlen(dir_name) + strlen("/applications/") + 1
		);
	strcpy(dirs->current_dir_name, dir_name);
	strcat(dirs->current_dir_name, "/applications/");
	return opendir(dirs->current_dir_name);
}

struct dirent *_dirs_next_dirent(Dirs *dirs)
{
	if (!dirs->current_dirent) {
		dirs->current_dir = _dirs_next_dir(dirs);
		if (!dirs->current_dir) 
			return NULL;
	}
	dirs->current_dirent = readdir(dirs->current_dir);
	return dirs->current_dirent;
}

char *_dirs_next_filename(Dirs *dirs)
{
	struct dirent *dirent;
	char *fullname;
	while (!(dirent = _dirs_next_dirent(dirs))) {
		return NULL;
	}
	fullname = (char *)malloc(
		strlen(dirs->current_dir_name) + strlen(dirent->d_name) + 1
		);
	strcpy(fullname, dirs->current_dir_name);
	strcat(fullname, dirent->d_name);
	return fullname;
}

char *dirs_next_file(Dirs *dirs)
{
	char filename = NULL;
	FILE *file = NULL;
	while (!file) {
		filename = _dirs_next_filename(dirs);
		if (!filename && dirs->is_end)
			return NULL;
		file = fopen(filename);
	}
	return file;
}
