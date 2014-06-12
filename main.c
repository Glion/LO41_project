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
#include <signal.h>

#define TRUE 1
#define FALSE 0

#define MENAGER 0
#define VERRE 1
#define CARTON 2

#define BAC 0
#define CLE 1
#define CLE_BAC 2

#define NOMBRE_USAGER donnees[0]
#define NOMBRE_CAMION donnees[1]
#define NOMBRE_COLLECTIVE donnees[3]
#define NOMBRE_VERRE donnees[4]
#define NOMBRE_CARTON donnees[5]

#define JOURS 30

int shmid_donnees, shmid_user;
int shm_cle;
int *donnees;

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

// Les différentes structures qui représente les différentes entités qui réagissent ensemble
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

Usager *allUser;

int remplirPoubelle (Usager user, int semid, int semnum, Dechet dechets) {

    int i;
    //mutex sur Poubelle.remplissage => ressource critique
    if (user.dechets[i].type == user.poubelleDuFoyer.type && user.poubelleDuFoyer.remplissage + user.dechets[i].volume <= user.poubelleDuFoyer.volume) {
        P(semid, semnum);
        user.poubelleDuFoyer.remplissage += user.dechets[i].volume;
        user.dechets[i].volume = 0;
        V(semid, semnum);
        if (user.poubelleDuFoyer.type == MENAGER) user.facturation_bac++;
        return TRUE;
    }
    else if (user.poubelleDuFoyer.type == MENAGER && user.poubelleDuFoyer.remplissage + user.dechets[i].volume > user.poubelleDuFoyer.volume) {
        if (user.contrat == CLE_BAC) {

        return TRUE;
        }
    }
    return FALSE;
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

    Usager *usager = data;
    int i, j, semid, semnum;
    Dechet dechets[3];
    dechets[0].type == MENAGER;
    dechets[1].type == VERRE;
    dechets[2].type == CARTON;
    for (i = 0; i < JOURS; i++) {
        for ( j = 0; j < 3; i ++) {
            srand ((unsigned) time(NULL)) ;
            dechets[j].volume = rand() % 20 + 1; // Génére des déchets de O à 20L
            if (remplirPoubelle(*usager, semid, semnum, dechets[j]) == FALSE) dechets[j].volume = 0;
            //dépôt sauvage, dans le cas où l'usager n'a pas pu vider sa poubelle ...
        }
        usleep(500000);// Dort une demi-seconde pour simuler journée
    }
    pthread_exit(usager.addition);
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
        printf("Usager numéro %d doit payer : %f\n", ++i, (user.facturation_bac*prixbac*taille + user.facturation_cle*prixcle));
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

/*
 * argv[1] : utilisateurs
 * argv[2] : camions
 * argv[3] : poubelle collective
 * argv[4] : poubelle verre
 * argv[5] : poubelle carton
 */
int main (int argc, char** argv) {

    int i, countCamion, rc, sem_id;
    struct Use *use;
    /*
     * Mise des données du programme passeée en paremètre
     * dans des segments de mémoire partagée,
     * pour un accès simplifié entre touts les
     * processus/threads du programme
     */
    shmid_donnees = shmget(IPC_PRIVATE, 5*sizeof(int), 0666);
    donnees = (int *) shmat (shmid_donnees, NULL, 0);
    donnees[0] = atoi(argv[1]);
    donnees[1] = atoi(argv[2]);
    donnees[2] = atoi(argv[3]);
    donnees[3] = atoi(argv[4]);
    donnees[4] = atoi(argv[5]);
    Usager usager[NOMBRE_USAGER];
    Ramasseur camion[NOMBRE_CAMION];
    pthread_t usager_id[NOMBRE_USAGER];
    pthread_t camion_id[NOMBRE_CAMION];
    sem_id = semget(ftok("Poubelles", IPC_PRIVATE), 1, IPC_CREAT);
    shmid_user = shmget(IPC_PRIVATE, 100*sizeof(Usager), 0666);
    allUser = (Usager *) shmat (shmid_user, NULL, 0);
    for (i = 0; i < NOMBRE_USAGER; i ++) {
        use = malloc(sizeof(Usager));
        rc = pthread_create (&usager_id[i], NULL, utiliser, use);
        if (rc) {
            printf("ERROR ; return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
    }

    signal(SIGTSTP, displayConsole);
    signal(SIGKILL, displayFile);
    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    
    countCamion = 0;
    struct Ramasser *info;
    for (i = 0; i < NOMBRE_USAGER; ++i) {
        if (usager[i].poubelleDuFoyer.remplissage > 0.8*usager[i].poubelleDuFoyer.volume) { //si poubelle pleine à 80%, le camion va vider les poubelles
            if (countCamion < 3) {
                info = malloc(sizeof(info));
                rc = pthread_create(&camion_id[i], NULL, viderPoubelle, info);
                if (rc) {
                    printf("ERROR ; return code from pthread_create() is %d\n",rc);
                    exit(-1);
                }
                countCamion++;
                usager[i].facturation_bac++;
            }
        }
    }
    // Créer Threads usager
    // Créer Threads camion de ramassage
    //processus Centre de tri qui gère les threads camion poubelle
    //processus Ville qui gère les threads usagers
    pthread_exit(NULL);
    shmctl(shmid_donnees, IPC_RMID, NULL);
    shmctl(shmid_user, IPC_RMID, NULL);
    return EXIT_SUCCESS;
}

// TODO
// créer et terminer les threads proprement, y a une erreur ligne 160 ! (Glion je te laisse voir ça)
// main.c:160:24: error: request for member ‘addition’ in something not a structure or union

// utiliser la mémoire partager ( je m'en occupe, je suis en plein dessus, quasiment fini)
// vérifier les signaux ( je m'en occupe, je suis en plein dessus )
// réseaux de pétrie ... ( si t'es motivé x) )
// fonctionnement de l'ensemble du programme, rien oublier ?
