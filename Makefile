TestThread: ThreadUtilisateur.o TestThread.c
	gcc TestThread.c ThreadUtilisateur.o -g -o TestThread 

ThreadUtilisateur.o: ThreadUtilisateur.c ThreadUtilisateur.h
	gcc -c ThreadUtilisateur.c -g -o ThreadUtilisateur.o 
	
clean: 
	rm -rf *.o *.*~ TestThread
