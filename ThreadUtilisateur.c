/**************************************************************************
    Travail pratique No 2 : Thread utilisateurs
    
    Ce fichier est l'implémentation de la librarie des threads utilisateurs.
         
	Systemes d'explotation GLO-2001
	Universite Laval, Quebec, Qc, Canada.
	(c) 2015 Philippe Giguere
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ucontext.h>
#include <time.h>
#include "ThreadUtilisateur.h"

/* Définitions privées, donc pas dans le .h, car l'utilisateur n'a pas besoin de
   savoir ces détails d'implémentation. OBLIGATOIRE. */
typedef enum { 
	THREAD_EXECUTE=0,
	THREAD_PRET,
	THREAD_BLOQUE,
	THREAD_TERMINE
} EtatThread;

#define TAILLE_PILE 4096   // Taille de la pile utilisée pour les threads

/* Structure de données pour créer une liste chaînée simple sur les threads qui ont fait un join.
   Facultatif */
typedef struct WaitList {
	struct TCB *pThreadWaiting;
	struct WaitList *pNext;
} WaitList;

/* TCB : Thread Control Block. Cette structure de données est utilisée pour stocker l'information
   pour un thread. Elle permet aussi d'implémenter une liste doublement chaînée de TCB, ce qui
   facilite la gestion et permet de faire un ordonnanceur tourniquet sans grand effort. 
   Facultatif. */
typedef struct TCB {  // Important d'avoir le nom TCB ici, sinon le compilateur se plaint.
	tid                 id;        // Numero du thread
	EtatThread			etat;      // Etat du thread
	ucontext_t          ctx;       // Endroit où stocker le contexte du thread
	struct TCB         *pSuivant;   // Liste doublement chaînée
	struct TCB         *pPrecedant; // Liste doublement chaînée
	struct WaitList	   *pWaitListJoinedThreads; // Liste chaînée simple des threads en attente.
} TCB;

// Pour que les variables soient cachées à l'utilisateur, on va les déclarer static. Facultatif.
static TCB *gpThreadCourant;	    //Thread en cours d'execution
static int gNTCB = 0;
static int gNextTID = 100;


/* Cette fonction insère le TCB pToInsert dans la liste doublement chaînée, juste après
   pActual. */
int InsertTCBAfter(TCB *pActual, TCB *pToInsert) {
    // Mettre à jour le TCB précédent
	TCB *pNext = pActual->pSuivant;
 	pActual->pSuivant = pToInsert;
	
	// Mettre à jour le TCB inséré
	pToInsert->pSuivant   = pNext;
	pToInsert->pPrecedant = pActual;
	
	// Et aussi celui après
	pNext->pPrecedant = pToInsert;
	
	// On incrémente le nombre de TCB
	return gNTCB++;
}

/* Cette fonction retire le TCB pActual de la liste doublement chaînée. */
int RemoveTCB(TCB *pActual) {
    // Mettre à jour le TCB inséré
 	pActual->pSuivant->pPrecedant = pActual->pPrecedant;
	pActual->pPrecedant->pSuivant = pActual->pSuivant;
	// Mettre à jour le TCB inséré
	pActual->pSuivant   = NULL;
	pActual->pPrecedant = NULL;
	
	// On décrémente le nombre de TCB
	return gNTCB--;
}

/* Cette fonction retourne le TCB correspondant au ThreadID fourni. */
TCB *GetPointerFromTid(tid ThreadID) {
	/* On inspecte la liste circulaire pour la traduction. Pas super rapide. */
	int i;
	TCB *pTCB = gpThreadCourant;
	for (i=0; i< gNTCB; i++) {
		if (pTCB->id == ThreadID) { return pTCB;}
		pTCB = pTCB->pSuivant;
	}
	return NULL;
}

// Et le reste, c'est à vous...

static TCB	*newThread() {
  TCB	*thread = malloc(sizeof(TCB));
  char	*pile = malloc(TAILLE_PILE);

  if (thread == NULL || pile == NULL)
    return NULL;
  getcontext(&(thread->ctx));
  thread->ctx.uc_stack.ss_sp = pile;
  thread->ctx.uc_stack.ss_size = TAILLE_PILE;
  thread->id = gNextTID;
  gNextTID++;
  thread->etat = THREAD_PRET;
  thread->pSuivant = NULL;
  thread->pPrecedant = NULL;
  thread->pWaitListJoinedThreads = NULL;
  return thread;
}

int	ThreadInit()  {
  gpThreadCourant = newThread();
  gpThreadCourant->pSuivant = gpThreadCourant;
  gpThreadCourant->pPrecedant = gpThreadCourant;
  gNTCB++;
}

tid	ThreadCreer(void (*fn)(void *), void *arg) {
  TCB	*thread = newThread();

  if (thread == NULL)
    return -1;
  InsertTCBAfter(gpThreadCourant, thread);
  makecontext(&(thread->ctx), (void *)fn, 1, arg);
  return thread->id;
}

static char	getEtat(TCB *thread) {
  switch (thread->etat)  {
  case THREAD_EXECUTE:
    return 'E';
  case THREAD_PRET:
    return 'P';
  case THREAD_BLOQUE:
    return 'B';
  case THREAD_TERMINE:
    return 'T';
  default:
    return '?';
  }
}

void	ThreadCeder()  {
  TCB	*tmp = gpThreadCourant->pSuivant;
  TCB	*actuel = gpThreadCourant;
  int	hasBeenModified = 0;

  printf("ThreadCeder():\n");
  printf("ThreadID:%d\tÉtat:%c\n", gpThreadCourant->id, getEtat(gpThreadCourant));
  if (gpThreadCourant->etat == THREAD_EXECUTE)
    gpThreadCourant->etat = THREAD_PRET;
  while (tmp != actuel) {
    printf("ThreadID:%d\tÉtat:%c\n", tmp->id, getEtat(tmp));
    if (hasBeenModified == 0 && tmp->etat == THREAD_PRET) {
      gpThreadCourant = tmp;
      hasBeenModified = 1;
      gpThreadCourant->etat = THREAD_EXECUTE;
    }
    tmp = tmp->pSuivant;
  }
  swapcontext(&(actuel->ctx), &(gpThreadCourant->ctx));
}

static int	addWaitingThread(tid ThreadAJoindre) {
  int	returnValue = 1;
  TCB	*tmpThread;

  tmpThread = GetPointerFromTid(ThreadAJoindre);
  if (tmpThread == gpThreadCourant) {
    returnValue = -1;
  } else if (tmpThread->etat == THREAD_TERMINE)  {
    returnValue = -2;
  } else  {
    WaitList *tmpWait = gpThreadCourant->pWaitListJoinedThreads;

    while (tmpWait != NULL) {
      if (tmpWait->pThreadWaiting == tmpThread)
	return returnValue;
      else
	tmpWait = tmpWait->pNext;
    }
    WaitList *newItem = malloc(sizeof(WaitList));

    newItem->pNext = gpThreadCourant->pWaitListJoinedThreads;
    newItem->pThreadWaiting = tmpThread;
    gpThreadCourant->pWaitListJoinedThreads = newItem;
  }
  
  return returnValue;
}

int	ThreadJoindre(tid ThreadAJoindre) {
  gpThreadCourant->etat = THREAD_BLOQUE;

  int returnValue = addWaitingThread(ThreadAJoindre);
  if (returnValue == 1)
    ThreadCeder();
  return returnValue;
}

void	ThreadQuitter() {
  TCB	*tmp = gpThreadCourant->pSuivant;;

  gpThreadCourant->etat = THREAD_TERMINE;

  while (tmp != gpThreadCourant) {
    if (tmp->pWaitListJoinedThreads) {
      WaitList *wait = tmp->pWaitListJoinedThreads;
      WaitList *prevWait = NULL;
      while (wait != NULL)  {
	if (wait->pThreadWaiting == gpThreadCourant)  {
	  WaitList *aSupprimer = wait;

	  wait = wait->pNext;
	  if (prevWait == NULL) {
	    tmp->pWaitListJoinedThreads = wait;
	  } else {
	    prevWait->pNext = wait;
	  }
	  free(aSupprimer);
	} else {
	  prevWait = wait;
	  wait = wait->pNext;
	}
      }
      if (tmp->pWaitListJoinedThreads == NULL && tmp->etat == THREAD_BLOQUE)
	tmp->etat = THREAD_PRET;
    }
    tmp = tmp->pSuivant;
  }
  ThreadCeder();
}

int ThreadDetruire(tid ThreadID) {
  TCB	*thread = GetPointerFromTid(ThreadID);

  if (thread->pWaitListJoinedThreads)  {
    WaitList *wait = thread->pWaitListJoinedThreads;
    while (wait)  {
      WaitList *toDelete = wait;
      wait = wait->pNext;
      free(toDelete);
    }
  }
  RemoveTCB(thread);
  free(thread);
  return 0;
}

tid ThreadId() {
  if (gpThreadCourant != NULL)
    return gpThreadCourant->id;
  return 0;
}
