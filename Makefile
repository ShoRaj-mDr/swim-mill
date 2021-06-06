test: swim_mill.c
	gcc swim_mill.c -o test -pthread
	gcc Fish.c -o fish
	gcc Food.c -o food
	
clean:
	rm fish
	rm food
	rm test
