/**************************************************************************
    Travail pratique No 2 : Thread utilisateurs
    
    Ce fichier sert à tester votre implémentation d'une librarie de 
    threads utilisateurs. VOUS NE POUVEZ PAS LE MODIFIER!
         
	Systemes d'explotation GLO-2001
	Universite Laval, Quebec, Qc, Canada.
	(c) 2015 Philippe Giguere
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadUtilisateur.h"

#define nbThread 3	//Nombre de thread

/* Structure passée à chaque thread lors de son démarrage */
typedef struct threadArg {
  int *arg1, *arg2;
  int Numero;
} threadArg;

// Voici la fonction qui sera exécutée par les threads.
void thrFunc(void *arg){
	long i, cpt;
	// On cast le pointeur vers la structure passée lors de l'appel ThreadCreer();
	threadArg  *pArg = (threadArg*) arg;
	for (i=0; i<3; i++) {
		printf("Thread %d tourne avec une variable sur la pile à 0x%p.\n",ThreadId(),&i);
		fflush(stdout);
		for (cpt=0; cpt < 10000000; cpt++);
		ThreadCeder();
	}
	printf("Thread %d QUITTE!\n",ThreadId());
	ThreadQuitter();
	return;
}

/* ==================== m a i n (  ) ==================== */

int main(void) {
	threadArg arg[nbThread];
	tid MyThreadIds[nbThread];
	int i;

	// Initialisation de votre libraire des threads utilisateurs
	if (ThreadInit() == -1) {
		perror("Erreur d'initialisation de la librarie!\n");
	    return 1;
	}

	// Il doit maintenant y avoir un thread ID associé au main
	printf("Main: Le thread ID du main est %d.\n",ThreadId());

	// Creation des Threads
	for (i = 0; i < nbThread; i++) {
		arg[i].Numero = i;
		printf("Main: Démarrage du thread numéro %d\n",arg[i].Numero);
	    if ((MyThreadIds[i] = ThreadCreer(thrFunc, (void *)&arg[i])) < 0) {
	    	perror("Erreur lors de la création d'un nouveau thread!\n");
	    	return 1;
	    }
	    printf("Main: thread avec ID %d crée.\n",MyThreadIds[i]);
	}

	
	//Attendre la fin de tout les threads avec ThreadJoindre
	for (i = 0; i < nbThread; i++) {// Creation des Threads
		printf("Main: je joins le thread ID %d\n",MyThreadIds[i]);
		int ret = ThreadJoindre(MyThreadIds[i]);
		if (ret == 1) printf("Main: Le join sur le thread %d vient de retourner!\n",MyThreadIds[i]);
		if (ret == -2) printf("Main: Le thread %d avait déjà terminé!\n",MyThreadIds[i]);
	}
	
	printf("Main: tous les threads ont terminé!\n");

    
	printf("Main: Je détruis tous les threads.\n");
	// Détruire les threads
	for (i = 0; i < nbThread; i++) {
		ThreadDetruire(MyThreadIds[i]);
	}

	return EXIT_SUCCESS;
}
