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

int shmid_donnees, shmid_users, shmid_poubelles, shmid_camions;
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
Poubelle *allBin;
Ramasseur *allTrucks;

int remplirPoubelle (int id, int type, int semid, int semnum) {

    //mutex sur Poubelle.remplissage => ressource critique
    if (allUser[id].dechets[type].type == allUser[id].poubelleDuFoyer.type && allUser[id].poubelleDuFoyer.remplissage + allUser[id].dechets[type].volume <= allUser[id].poubelleDuFoyer.volume && (allUser[id].contrat == BAC || allUser[id].contrat == CLE_BAC)) {
        P(semid, semnum);
        allUser[id].poubelleDuFoyer.remplissage += allUser[id].dechets[type].volume;
        allUser[id].dechets[type].volume = 0;
        V(semid, semnum);
        printf("L'usager %d a déposé %d L de d'ordure dans sa propre poubelle\n", id, allUser[id].dechets[type].volume);
        if (allUser[id].poubelleDuFoyer.type == MENAGER) allUser[id].facturation_bac++;
        return TRUE;
    }
    else if (allUser[id].contrat == CLE || allUser[id].contrat == CLE_BAC) {
        srand ((unsigned) time(NULL)) ;
        int poubelle = (rand() % NOMBRE_COLLECTIVE + 1);
        if (allBin[poubelle].remplissage + allUser[id].dechets[type].volume < allBin[poubelle].volume) {
            allBin[poubelle].remplissage += allUser[id].dechets[type].volume;
            allUser[id].dechets[type].volume = 0;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle collective.\n", id, allUser[id].dechets[type].volume);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else if (allUser[id].dechets[type].type == VERRE) {
        srand ((unsigned) time(NULL)) ;
        int poubelle = (rand() % NOMBRE_VERRE + 1) + NOMBRE_COLLECTIVE;
        if (allBin[poubelle].remplissage + allUser[id].dechets[type].volume < allBin[poubelle].volume) {
            allBin[poubelle].remplissage += allUser[id].dechets[type].volume;
            allUser[id].dechets[type].volume = 0;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle verre.\n", id, allUser[id].dechets[type].volume);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else if (allUser[id].dechets[type].type == CARTON) {
        srand ((unsigned) time(NULL)) ;
        int poubelle = (rand() % NOMBRE_CARTON + 1) + NOMBRE_COLLECTIVE + NOMBRE_VERRE;
        if (allBin[poubelle].remplissage + allUser[id].dechets[type].volume < allBin[poubelle].volume) {
            allBin[poubelle].remplissage += allUser[id].dechets[type].volume;
            allUser[id].dechets[type].volume = 0;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle carton.\n", id, allUser[id].dechets[type].volume);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    return FALSE;
}

void* viderPoubelle (int id, Ramasser *data) {//Ramasseur camion, Poubelle poubellePleine) {

    int semid, semnum;
    Ramasser *ramasser = data;
    //mutex sur poubelle.remplissage => ressource critique
    if (ramasser->camion.type == ramasser->poubellePleine.type) {
        P(semid, semnum);
        ramasser->camion.remplissage += ramasser->poubellePleine.remplissage;
        ramasser->poubellePleine.remplissage = 0;
        V(semid, semnum);
        printf("Le camion n°%d vient de vider la poubelle !", id);
    }
    else {
        printf("error : Wrong bin type\n");
    }
    free(ramasser);
    pthread_exit(NULL);
}

void compoFoyer (int id) {

    if(allUser[id].foyer == 1)
        allUser[id].poubelleDuFoyer.volume = 80;
    else if (allUser[id].foyer == 2)
        allUser[id].poubelleDuFoyer.volume = 120;
    else if(allUser[id].foyer == 3 || allUser[id].foyer == 4)
        allUser[id].poubelleDuFoyer.volume = 180;
    else if (allUser[id].foyer >= 5)
        allUser[id].poubelleDuFoyer.volume = 240;
}

void *utiliser (void *num) { //Usager user){

    int *id = (int*) num;
    int i, j, semid, semnum;
    //initalisation des données utilisateur
    srand ((unsigned) time(NULL)) ;
    allUser[*id].contrat = rand() % 2 + 1;
    allUser[*id].foyer = rand() % 6 + 1;
    allUser[*id].dechets[0].type == MENAGER;
    allUser[*id].dechets[1].type == VERRE;
    allUser[*id].dechets[2].type == CARTON;
    if(allUser[*id].contrat == 0 || allUser[*id].contrat == 2){
        allUser[*id].poubelleDuFoyer.remplissage = 0;
        compoFoyer(*id);
        allUser[*id].poubelleDuFoyer.type = MENAGER;
    }
    for (i = 0; i < JOURS; i++) {
        for ( j = 0; j < 3; i ++) {
            srand ((unsigned) time(NULL)) ;
            allUser[*id].dechets[j].volume = rand() % 20 + 1; // Génére des déchets de O à 20L
            if (remplirPoubelle(*id, allUser[*id].dechets[j].type, semid, semnum) == FALSE) {
                allUser[*id].dechets[j].volume = 0;
                printf("L'utilisateur n°%d fait un depôt sauvage de %d L en plein milieu de la rue !!!\n", *id, allUser[*id].dechets[j].volume);
                //dépôt sauvage, dans le cas où l'usager n'a pas pu vider sa poubelle ...
            }
        }
        usleep(500000);// Dort une demi-seconde pour simuler journée
    }
    //pthread_exit(&usager->addition);
    pthread_exit(NULL);
}

void *eboueur (void *num) {

    int *id = (int *)num;
    int i, j, semid, semnum;
    allTrucks[*id].capaCamion = 3000;
    allTrucks[*id].remplissage = 0;
    if ( (float)(NOMBRE_CAMION/(*id)) <= 0.2 ){
        allTrucks[*id].type == CARTON;
    }
    else if ( (float)(NOMBRE_CAMION/(*id)) <= 0.4 ) {
        allTrucks[*id].type == VERRE;
    }
    else{
        allTrucks[*id].type == MENAGER;
    }
    //ViderPoubelle(*id, data);
}

void displayFile (int signal, Usager user) {

    FILE *facturation;
    facturation = fopen("facturation.txt", "w");
    int i, prixbac, prixcle, taille;
    for(i = 0; i < NOMBRE_USAGER; i++){
        fprintf(facturation, "Usager numéro %d doit payer : %f\n", ++i, (float)(user.facturation_bac*prixbac*taille + user.facturation_cle*prixcle));
    }
    fclose(facturation);
}

void initialisationPoubelleCollective () {

    int i;
    for (i = 0; i < (NOMBRE_COLLECTIVE + NOMBRE_CARTON + NOMBRE_VERRE); i++) {
        if (i < NOMBRE_COLLECTIVE){
            allBin[i].type == MENAGER;
        }
        else if (i < NOMBRE_COLLECTIVE + NOMBRE_VERRE) {
            allBin[i].type == VERRE;
        }
        else{
            allBin[i].type == CARTON;
        }
    }
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
    //struct Use *use;
    //struct sigaction action;
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
    shmid_poubelles = shmget(IPC_PRIVATE, (NOMBRE_VERRE + NOMBRE_COLLECTIVE + NOMBRE_CARTON)*sizeof(int), 0666);
    allBin = (Poubelle *) shmat (shmid_donnees, NULL, 0);
    shmid_camions = shmget(IPC_PRIVATE, 5*sizeof(int), 0666);
    initialisationPoubelleCollective();
    allTrucks = (Ramasseur *) shmat (shmid_donnees, NULL, 0);
    pthread_t usager_id[NOMBRE_USAGER];
    pthread_t camion_id[NOMBRE_CAMION];
    sem_id = semget(ftok("Poubelles", IPC_PRIVATE), 1, IPC_CREAT);
    shmid_users = shmget(IPC_PRIVATE, 100*sizeof(Usager), 0666);
    allUser = (Usager *) shmat (shmid_users, NULL, 0);
    for (i = 0; i < NOMBRE_USAGER; i++) {
        //use = malloc(sizeof(Usager));
        rc = pthread_create (&usager_id[i], NULL, utiliser, (void *) &i);
        if (rc) {
            printf("ERROR ; return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
        //free(use);
    }
    //signal(SIGKILL, displayFile);XXX
    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    countCamion = 0;
    for (i = 0; i < NOMBRE_USAGER; ++i) {
        if (allUser[i].poubelleDuFoyer.remplissage > 0.8*allUser[i].poubelleDuFoyer.volume) { //si poubelle pleine à 80%, le camion va vider les poubelles
            if (countCamion < 3) {
                //info = malloc(sizeof(info));
                rc = pthread_create(&camion_id[i], NULL, eboueur, NULL/*TODO*/);
                if (rc) {
                    printf("ERROR ; return code from pthread_create() is %d\n",rc);
                    exit(-1);
                }
                countCamion++;
                allUser[i].facturation_bac++;
                //free(info);
            }
        }
    }
    // Créer Threads usager
    // Créer Threads camion de ramassage
    //processus Centre de tri qui gère les threads camion poubelle
    //processus Ville qui gère les threads usagers
    pthread_exit(NULL);
    shmctl(shmid_donnees, IPC_RMID, NULL);
    shmctl(shmid_users, IPC_RMID, NULL);
    shmctl(shmid_poubelles, IPC_RMID, NULL);
    shmctl(shmid_camions, IPC_RMID, NULL);
    return EXIT_SUCCESS;
}

// TODO

//initialiser les poubelles XXX
//refaire la gestion des camions XXX
//envoie du signaux sigusr1 poubelles(SIGUSR1) à Camion
// créer et terminer les threads proprement
// vérifier les signaux ( je m'en occupe, je suis en plein dessus )
// réseaux de pétrie ... ( si t'es motivé x) )
//Le warning pour les 2 signals c'est parce que en paramètre il doit y avoir normalement
// qu'un seul paramètre int
