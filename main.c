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

int P (int SemId, int Nsem) {

    struct sembuf SemBuf = {0,-1,0} ;
    SemBuf.sem_num = Nsem ;
    return semop(SemId, &SemBuf,1) ;
}

int V (int SemId, int Nsem) {

    struct sembuf SemBuf = {0,1,0} ;
    SemBuf.sem_num = Nsem ;
    return semop(SemId, &SemBuf, 1) ;
}

// Les défférentes structures qui repré&sente les différentes entités qui réagissent ensemble
typedef struct dechet{

    int type;
    int volume;
}Dechet;

typedef struct poubelle {

    int type; // same as dechet
    int volume;
    int remplissage;
}Poubelle;

typedef struct ramasseur{

    int remplissage;
    int capaCamion;
    int tournee[100];
    int type; //type de dechets dont il s'occupe
}Ramasseur;

typedef struct usager{

    int contrat;
    int foyer;
    Dechet dechets;
    Poubelle poubelleDuFoyer;
}Usager;

void remplirPoubelle(Usager user, int semid, int semnum){

    //mutex sur Poubelle.remplissage => ressource critique
    if (user.dechets.type == MENAGER) {
        P(semid, semnum);
        user.poubelleDuFoyer += user.dechets.volume;
        V(semid, semnum);
    } else if (user.dechets.type == VERRE) {
        P(semid, semnum);
        poubelleVerre += user.dechets.volume;
        V(semid, semnum);
    } else {
        P(semid, semnum);
        poubelleCarton += user.dechets.volume;
        V(semid, semnum);
    }
}


void viderPoubelle(Ramasseur camion, Poubelle poubelle){

    //mutex sur poubelle.remplissage => ressource critique
    if (camion.type == MENAGER) {
        P(semid, semnum);
        camion.remplissage += user.poubelleDuFoyer;
        user.poubelleDuFoyer = 0;
    } else if (camion.type == VERRE) {
        camion.remplissage += poubelleVerre;
        poubelleVerre = 0;
    } else {


    }

}

int main(int argc, char** argv){
    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    // A tout moment envoyé un SIGSTOP, stop la simulation et affiche les contenus des poubelles
    // Créer Threads usager
    // Créer Threads camion de ramassage
    //processus Centre de tri qui gère les threads camion poubelle
    //processus Ville qui gère les threads usagers
    return EXIT_SUCCESS;
}
