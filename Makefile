all:clean
	$(CC) dmenu_launch.c -o dmenu_launch

clean:
	rm -rf dmenu_launch

.PHONY: all clean
