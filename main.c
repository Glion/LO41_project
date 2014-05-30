/*
 * Simon Magnin-Feysot
 * Aurélie Pelligand
 *
 * Projet de l'UV L041:
 * Gérer les poubelles d'une ville
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define MENAGER 0
#define VERRE 1
#define CARTON 2

#define BAC 0
#define CLE 1
#define CLE_BAC 2

#define NOMBRE_USAGER 100
#define NOMBRE_CAMION 3

#define JOURS 30

int P (int SemId, int Nsem) {

    struct sembuf SemBuf = {0,-1,0};
    SemBuf.sem_num = Nsem ;
    return semop(SemId, &SemBuf, 1);
}

int V (int SemId, int Nsem) {

    struct sembuf SemBuf = {0,1,0} ;
    SemBuf.sem_num = Nsem ;
    return semop(SemId, &SemBuf, 1) ;
}

// Les défférentes structures qui repré&sente les différentes entités qui réagissent ensemble
typedef struct dechet {

    int type;
    int volume;
}Dechet;

typedef struct poubelle {

    int type; // same as dechet
    int volume;
    int remplissage;
}Poubelle;

typedef struct ramasseur {

    int remplissage;
    int capaCamion;
    int tournee[100];
    int type; //type de dechets dont il s'occupe
}Ramasseur;

typedef struct usager {

    int contrat;
    int foyer;
    int facturation_bac; // compteur du nombre de ramassage
    int facturation_cle;
    int addition;
    Dechet dechets[3];
    Poubelle poubelleDuFoyer;
}Usager;

void remplirPoubelle (Usager user, int semid, int semnum, Dechet dechets) {

    int i;
    //mutex sur Poubelle.remplissage => ressource critique
    if (user.dechets[i].type == poubelle.type && poubelle.remplissage + user.dechets[i].volume <= poubelle.volume) {
        P(semid, semnum);
        poubelle.remplissage += user.dechets[i].volume;
        user.dechets[i].volume = 0;
        V(semid, semnum);
        if (poubelle.type == MENAGER) user.facturation_bac++;
    }
    else if (poubelle.type == MENAGER && poubelle.remplissage + user.dechets[i].volume > poubelle.volume) {
        if (user.contrat == CLE_BAC) {

        }
    }
}

void viderPoubelle (Ramasseur camion, Poubelle poubellePleine) {

    int semid, semnum;
    //mutex sur poubelle.remplissage => ressource critique
    if (camion.type == poubellePleine.type) {
        P(semid, semnum);
        camion.remplissage += poubellePleine.remplissage;
        poubellePleine.remplissage = 0;
        V(semid, semnum);
    }
    else {
        printf("error : Wrong bin type\n");
    }
}

void *utiliser(Usager user){

    int i, j;
    Dechet dechets[3];
    dechets[0].type == MENAGER;
    dechets[1].type == VERRE;
    dechets[2].type == CARTON;
    for (i = 0; i < JOURS; i++) {
        for ( j = 0; j < 3; i ++) {
            srand ((unsigned) time(NULL)) ;
            dechets[j].volume = rand() % 20 + 1; // Génére des déchets de O à 30L
            remplirPoubelle(user, semid, semnum, dechets[j]);
        }
        usleep(500000);// Dort une demi-seconde pour simuler le cycle Jour/nuit
    }
    pthread_exit(&user.addition);
}

int main (int argc, char** argv) {

    int i;
    //Usager usager[100];
    //Camion camion[100];
    pthread_t *usager_id[NOMBRE_USAGER];
    pthread_t camion_id[NOMBRE_CAMION];
    for (i = 0; i < NOMBRE_USAGER; i ++) {
        pthread_create (usager_id[i], NULL, utiliser, usager_id[i]);
    }
    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    // A tout moment envoyé un SIGSTOP, stop la simulation et affiche les contenus des poubelles
    // Créer Threads usager
    // Créer Threads camion de ramassage
    //processus Centre de tri qui gère les threads camion poubelle
    //processus Ville qui gère les threads usagers
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

/************************************************
 * TODO GLION :                                 *
 *                                              *
 * S'occupes de la partie centre de tri :       *
 * - envoyer les camions ramasser les poubelles *
 *   Lors du signal                             *
 * - incrémenter le compteur                    *
 *                                              *
 *   Ma partie est pas encore dans le fichier   *
 *   car elle bug, je m'en occupe rapidement    *
 ************************************************
