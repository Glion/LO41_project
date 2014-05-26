/*
 * Simon Magnin-Feysot
 * Aurélie Pelligand
 *
 * Projet de l'UV L041:
 * Gérer les poubelles d'une ville
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>

#define MENAGER 0
#define VERRE 1
#define CARTON 2

#define BAC 0
#define CLE 1
#define CLE_BAC 2

int P (int SemId, int Nsem) {       /*   P(s)      */

    struct sembuf SemBuf = {0,-1,0} ;
    SemBuf.sem_num = Nsem ;
    return semop(SemId, &SemBuf,1) ;
}

int V (int SemId, int Nsem) {       /*   V(s)      */

    struct sembuf SemBuf = {0,1,0} ;
    SemBuf.sem_num = Nsem ;
    return semop(SemId, &SemBuf, 1) ;
}
// struct client Foyer, Poubelle,
typedef struct dechet{

    int type;
    // 0 : ménager
    // 1 : verre
    // 2 : carton

    int volume;
}Dechet;

typedef struct poubelle {

    int type; // same as dechet
    int volume;
    int remplissage;
}Poubelle;

typedef struct ramasseur{

    int rempli ;
    int capaCamion;
    int tournee[100];
}Ramasseur;

typedef struct usager{

    int contrat;
    int foyer;
    Dechet poubelle;
    Poubelle poubelleDuFoyer;
}Usager;

void remplirPoubelle(Usager user){
//mutex sur Poubelle.remplissage => ressource critique
    if(user.poubelle.type == MENAGER){
    P(0);

    V(0);
    }
    else if(user.poubelle.type == VERRE){
    P(0);

    V(0);
    }
    else{
    P(0);

    V(0);
    }
}


void viderPoubelle(int poids, int poubelle){
//mutex sur poubelle.remplissage => ressource critique

}

int main(int argc, char** argv){

    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    // Créer Threads usager
    // Créer Threads camion de ramassage
    //processus Centre de tri
    return EXIT_SUCCESS;
}
