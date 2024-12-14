#include <stdio.h>
#include <errno.h>
#include <unistd.h>

typedef struct {
	FILE *in; //Dmenu's stdin
	FILE *out; //Dmenu's stdout
} Dmenu;

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
