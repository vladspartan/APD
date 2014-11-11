build:
	mpicc -o tema4 main.c

test1:
	mpiexec -n 13 ./tema4 ./tests/top1 ./tests/messages1

test2:
	mpiexec -n 5 ./tema4 ./tests/top2 ./tests/messages2

test3:
	mpiexec -n 9 ./tema4 ./tests/top3 ./tests/messages3

clean:
	rm tema4