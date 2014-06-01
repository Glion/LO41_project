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

int shmid;
int shm_cle;

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

// Les défférentes structures qui représente les différentes entités qui réagissent ensemble
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

typedef struct ramasser {

    Ramasseur camion;
    Poubelle poubellePleine;
}Ramasser;

typedef struct use {

    Usager usager;
}Use;

void remplirPoubelle (Usager user, int semid, int semnum, Dechet dechets) {

    int i;
    //mutex sur Poubelle.remplissage => ressource critique
    if (user.dechets[i].type == user.poubelleDuFoyer.type && user.poubelleDuFoyer.remplissage + user.dechets[i].volume <= user.poubelleDuFoyer.volume) {
        P(semid, semnum);
        user.poubelleDuFoyer.remplissage += user.dechets[i].volume;
        user.dechets[i].volume = 0;
        V(semid, semnum);
        if (user.poubelleDuFoyer.type == MENAGER) user.facturation_bac++;
    }
    else if (user.poubelleDuFoyer.type == MENAGER && user.poubelleDuFoyer.remplissage + user.dechets[i].volume > user.poubelleDuFoyer.volume) {
        if (user.contrat == CLE_BAC) {
        }
    }
}

void* viderPoubelle (void *data) {//Ramasseur camion, Poubelle poubellePleine) {

    int semid, semnum;
    Ramasser *ramasser = data;
    //mutex sur poubelle.remplissage => ressource critique
    if (ramasser->camion.type == ramasser->poubellePleine.type) {
        P(semid, semnum);
        ramasser->camion.remplissage += ramasser->poubellePleine.remplissage;
        ramasser->poubellePleine.remplissage = 0;
        V(semid, semnum);
    }
    else {
        printf("error : Wrong bin type\n");
    }
    free(ramasser);
}

void *utiliser (void *data) { //Usager user){

    Use *use = data;
    int i, j, semid, semnum;
    Dechet dechets[3];
    dechets[0].type == MENAGER;
    dechets[1].type == VERRE;
    dechets[2].type == CARTON;
    for (i = 0; i < JOURS; i++) {
        for ( j = 0; j < 3; i ++) {
            srand ((unsigned) time(NULL)) ;
            dechets[j].volume = rand() % 20 + 1; // Génére des déchets de O à 30L
            remplirPoubelle(use->usager, semid, semnum, dechets[j]);
        }
        usleep(500000);// Dort une demi-seconde pour simuler le cycle Jour/nuit
    }
    pthread_exit(&use->usager.addition);
}

void compoFoyer (Usager user) {

    if(user.foyer == 1)
        user.poubelleDuFoyer.volume = 80;
    if(user.foyer == 2)
        user.poubelleDuFoyer.volume = 120;
    if(user.foyer == 3 || user.foyer == 4){
        user.poubelleDuFoyer.volume = 180;
     }else if (user.foyer >= 5){
        user.poubelleDuFoyer.volume = 240;
     }
}

void displayConsole (int signal, Usager user) {

    int i, prixbac, prixcle, taille;
    for(i = 0; i < NOMBRE_USAGER; i++){
    }
}

void displayFile (int signal, Usager user) {

    FILE *facturation;
    facturation = fopen("facturation.txt", "w");
    int i, prixbac, prixcle, taille;
    for(i = 0; i < NOMBRE_USAGER; i++){
        fprintf(facturation, "Usager numéro %d doit payer : %f\n", ++i, (user.facturation_bac*prixbac*taille + user.facturation_cle*prixcle));
    }
    fclose(facturation);
}

Usager *allUser;

int main (int argc, char** argv) {

    int i, countCamion, rc;
    Usager usager[NOMBRE_USAGER];
    Ramasseur camion[NOMBRE_CAMION];
    struct Use *use;
    pthread_t usager_id[NOMBRE_USAGER];
    pthread_t camion_id[NOMBRE_CAMION];
    shmid = shmget(IPC_PRIVATE, 100*sizeof(Usager), 0666);
    allUser = (Usager *)shmat(shmid, NULL, 0);
    for (i = 0; i < NOMBRE_USAGER; i ++) {
        use = malloc(sizeof(Usager));
        pthread_create (&usager_id[i], NULL, utiliser, use);
    }
    signal(SIGTSTP, displayConsole);
    signal(SIGKILL, displayFile);
    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    countCamion = 0;
    struct Ramasser *info;
    for (i = 0; i < NOMBRE_USAGER; ++i) {
        if(usager[i].poubelleDuFoyer.remplissage > 0.8*usager[i].poubelleDuFoyer.volume){ //si poubelle pleine à 80%, le camion va vider les poubelles
            if(countCamion < 3){
                info = malloc(sizeof(info));
                rc = pthread_create(&camion_id[i], NULL, viderPoubelle, info);
                if(rc){
                    printf("ERROR ; return code from pthread_create() is %d\n",rc);
                    exit(-1);
                }
                countCamion++;
                usager[i].facturation_bac++;
            }
        }
    }
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
 ************************************************/
