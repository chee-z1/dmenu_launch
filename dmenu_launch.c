#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <iniparser/iniparser.h>
#include <iniparser/dictionary.h>

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

char *dirs_next_filename(Dirs *dirs)
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

FILE *dirs_next_file(Dirs *dirs, char **name_store)
{
	char *filename = NULL;
	FILE *file = NULL;
	while (!file) {
		filename = dirs_next_filename(dirs);
		if (!filename && dirs->is_end)
			return NULL;
		file = fopen(filename, "r");
		if (name_store)
			*name_store = filename;
		else
			free(filename);
	}
	return file;
}


dictionary *add_desktop_entries_from(dictionary *dic, FILE *file, char *name)
{
	const dictionary *appfile = iniparser_load_file(file, name);
	int nsec = iniparser_getnsec(appfile);
	for (int n=0;n<nsec;++n) {
		const char *secname = iniparser_getsecname(appfile, n);
		if (strncmp(secname, "desktop action", sizeof("desktop action") - 1)
			&& strcmp(secname, "desktop entry")) {
			continue;
		}
		char *full_name_key = malloc(strlen(secname) + strlen("name") + 2);
		char *full_exec_key = malloc(strlen(secname) + strlen("exec") + 2);
		strcpy(full_name_key, secname);
		strcat(full_name_key, ":");
		strcat(full_name_key, "name");
		strcpy(full_exec_key, secname);
		strcat(full_exec_key, ":");
		strcat(full_exec_key, "exec");
		const char *name = iniparser_getstring(appfile, full_name_key, NULL);
		const char *exec = iniparser_getstring(appfile, full_exec_key, NULL);
		if (!name) continue;
		if (!exec) continue;
		dictionary_set(dic, name, exec);
		free(full_name_key);
		return dic;
	}
}


