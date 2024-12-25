all:clean
	$(CC) dmenu_launch.c -liniparser -o dmenu_launch

clean:
	rm -rf dmenu_launch

.PHONY: all clean
