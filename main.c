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
#include <sys/time.h>
#include <sys/resource.h>

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

#define JOURS 365

int shmid_donnees, shmid_users, shmid_poubelles, shmid_camions;
int shm_cle;
int *donnees;
pthread_mutex_t mutex_utilisateur = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_camion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_poubelle = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_poubelle_collective = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t attente;

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

Usager *allUser;
Poubelle *allBin;
Ramasseur *allTrucks;

void viderPoubelle (Poubelle *p, int id, int usager) ;

int remplirPoubelle (int id, int type, int semid, int semnum) {

    int s;
    if (allUser[id].dechets[type].type == allUser[id].poubelleDuFoyer.type && allUser[id].poubelleDuFoyer.remplissage + allUser[id].dechets[type].volume <= allUser[id].poubelleDuFoyer.volume && (allUser[id].contrat == BAC || allUser[id].contrat == CLE_BAC)) { //déchets ménager dans poubelle foyer
        pthread_mutex_lock(&mutex_poubelle);
        allUser[id].poubelleDuFoyer.remplissage += allUser[id].dechets[type].volume;
        printf("L'usager %d a déposé %d L de d'ordure dans sa propre poubelle\n", id, allUser[id].dechets[type].volume);
        allUser[id].dechets[type].volume = 0;
        if (( (float)allUser[id].poubelleDuFoyer.remplissage / (float)allUser[id].poubelleDuFoyer.volume) > 0.7 ) {
            pthread_cond_signal(&attente);
        }
        pthread_mutex_unlock(&mutex_poubelle);
        return TRUE;
    }
    else if ((allUser[id].contrat == CLE || allUser[id].contrat == CLE_BAC) && allUser[id].dechets[type].type == MENAGER) {//déchets ménager dans bac collectif
        srand (time(NULL)) ;
        int poubelle = rand() % NOMBRE_COLLECTIVE + 1;
        if (allBin[poubelle].remplissage + allUser[id].dechets[type].volume < allBin[poubelle].volume) {
            pthread_mutex_lock(&mutex_poubelle_collective);
            allBin[poubelle].remplissage += allUser[id].dechets[type].volume;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle collective.\n", id, allUser[id].dechets[type].volume);
            allUser[id].dechets[type].volume = 0;
            allUser[id].facturation_bac++;
            if (( (float)allBin[poubelle].remplissage / (float)allBin[id].volume) > 0.7 ) {
                pthread_cond_signal(&attente);
            }
            pthread_mutex_unlock(&mutex_poubelle_collective);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else if (allUser[id].dechets[type].type == VERRE) { // dechets en verre
        srand (time(NULL)) ;
        s = rand() % NOMBRE_CARTON + 1;
        int poubelle = s + NOMBRE_COLLECTIVE + NOMBRE_VERRE;
        if (allBin[poubelle].remplissage + allUser[id].dechets[type].volume < allBin[poubelle].volume) {
            pthread_mutex_lock(&mutex_poubelle_collective);
            allBin[poubelle].remplissage += allUser[id].dechets[type].volume;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle verre.\n", id, allUser[id].dechets[type].volume);
            allUser[id].dechets[type].volume = 0;
            if (( (float)allBin[poubelle].remplissage / (float)allBin[id].volume) > 0.7 ) {
                pthread_cond_signal(&attente);
            }
            pthread_mutex_unlock(&mutex_poubelle_collective);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else if (allUser[id].dechets[type].type == CARTON) {//déchets en carton
        srand (time(NULL)) ;
        s = rand() % NOMBRE_CARTON + 1;
        int poubelle = s + NOMBRE_COLLECTIVE + NOMBRE_VERRE + NOMBRE_CARTON;
        if (allBin[poubelle].remplissage + allUser[id].dechets[type].volume < allBin[poubelle].volume) {
            pthread_mutex_lock(&mutex_poubelle_collective);
            allBin[poubelle].remplissage += allUser[id].dechets[type].volume;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle carton.\n", id, allUser[id].dechets[type].volume);
            allUser[id].dechets[type].volume = 0;
            if (( (float)allBin[poubelle].remplissage / (float)allBin[id].volume) > 0.7 ) {
                pthread_cond_signal(&attente);
            }
            pthread_mutex_unlock(&mutex_poubelle_collective);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    return FALSE;
}

void viderPoubelle (Poubelle *p, int id, int usager) {//Ramasseur camion, Poubelle poubellePleine) {
    printf("Camion n°%d vide la poubelle\n", id);
    if(usager){
        pthread_mutex_lock(&mutex_poubelle);
            allTrucks[id].remplissage += p->remplissage;
            p->remplissage = 0;
        pthread_mutex_unlock(&mutex_poubelle);
    }
    else{
        pthread_mutex_lock(&mutex_poubelle_collective);
            allTrucks[id].remplissage += p->remplissage;
            p->remplissage = 0;
        pthread_mutex_unlock(&mutex_poubelle_collective);
    }
}

void compoFoyer (int id) {

    if (allUser[id].foyer == 1)
        allUser[id].poubelleDuFoyer.volume = 80;
    else if (allUser[id].foyer == 2)
        allUser[id].poubelleDuFoyer.volume = 120;
    else if (allUser[id].foyer == 3 || allUser[id].foyer == 4)
        allUser[id].poubelleDuFoyer.volume = 180;
    else if (allUser[id].foyer >= 5)
        allUser[id].poubelleDuFoyer.volume = 240;
}

void *utiliser (void *num) {

    int id = (int) *((int*) num);
    printf("Création de l'utilisateur n°%d\n", id);
    pthread_mutex_unlock(&mutex_utilisateur);
    int k, j, semid, semnum;
    //initalisation des données utilisateur
    srand ((int) time(NULL)) ;
    allUser[id].contrat = rand() % 2 + 1;
    allUser[id].foyer = rand() % 6 + 1;
    allUser[id].dechets[0].type == MENAGER;
    allUser[id].dechets[1].type == VERRE;
    allUser[id].dechets[2].type == CARTON;
    if (allUser[id].contrat == 0 || allUser[id].contrat == 2) {
    //si il a le bon contrat on initialise sa poubelle perso
        allUser[id].poubelleDuFoyer.remplissage = 0;
        compoFoyer(id);
        allUser[id].poubelleDuFoyer.type = MENAGER;
    }
    // lancement de son activité
    srand ((int) time(NULL)) ;
    for (j = 0; j < JOURS; j++) {
        for ( k = 0; k < 3; k++) {
            allUser[id].dechets[j].volume = (rand() % 20 + 1); // Génére des déchets de O à 20L
            if (remplirPoubelle(id, j, semid, semnum) == FALSE) {
                printf("L'utilisateur n°%d fait un depôt sauvage de %d L\n", id, allUser[id].dechets[j].volume);
                allUser[id].dechets[j].volume = 0;
                //dépôt sauvage, dans le cas où l'usager n'a pas pu vider sa poubelle ...
            }
        }
        sleep(1);// Dort une seconde pour simuler journée et permettre un affichage agréable en console
        printf("Jour %d \n", j);
    }
    pthread_exit(NULL);
}

void *eboueur (void *num) { //Thread Camions

    int id = (int) *((int *)num);
    printf("création du camion n°%d\n", id);
    pthread_mutex_unlock(&mutex_camion);
    int l = 0, j;
    allTrucks[id].capaCamion = 3000; //initialisation
    allTrucks[id].remplissage = 0;
    Poubelle *poubelleAVider;
    srand ((int) time(NULL)) ;
    int choix = rand() % 2 + 1;
    if (choix == 1) {// s'occupe des poubelles collectives
        while (1) {
            pthread_cond_wait(&attente, &mutex_poubelle_collective);
            if ((float)allBin[l].remplissage / (float)allBin[l].volume > 0.7) {
                printf("camion n°%d, vide une poubelle\n", id);
                viderPoubelle(&allBin[l], id,  FALSE);
                allTrucks[id].type == allBin[id].type;
            }
            l++;
        }
        while (1) {
            for (l = 0; l < (NOMBRE_VERRE + NOMBRE_CARTON + NOMBRE_COLLECTIVE); l ++) {
                if (allBin[l].type == allTrucks[id].type && ((float)allBin[l].remplissage / (float)allBin[l].volume) > 0.7) {
                    printf("camion n°%d, vide une poubelle\n", id);
                    poubelleAVider = &allBin[l];
                }
            }
            viderPoubelle(poubelleAVider, id, FALSE);
        }
    }
    else{//s'occupe des poubelles personnelles
        allTrucks[id].type = MENAGER;
        while (1) {
            pthread_cond_wait(&attente, &mutex_poubelle);
            for (l = 0; l < (NOMBRE_USAGER); l ++) {
                if (allUser[l].poubelleDuFoyer.type == allTrucks[id].type && ((float)allUser[l].poubelleDuFoyer.remplissage / (float)allUser[l].poubelleDuFoyer.volume) > 0.7) {
                    printf("camion n°%d, vide une poubelle utilisateur\n", id);
                    poubelleAVider = &allUser[l].poubelleDuFoyer;
                }
            }
            viderPoubelle(poubelleAVider, id, TRUE);
        }
    }
    pthread_exit(NULL);
}

void fin (int signal) {//création du fichier pour les facturations clients

    pthread_mutex_destroy(&mutex_poubelle);
    pthread_mutex_destroy(&mutex_poubelle_collective);
    pthread_mutex_destroy(&mutex_utilisateur);
    pthread_mutex_destroy(&mutex_camion);
    shmctl(shmid_donnees, IPC_RMID, NULL);
    shmctl(shmid_users, IPC_RMID, NULL);
    shmctl(shmid_poubelles, IPC_RMID, NULL);
    shmctl(shmid_camions, IPC_RMID, NULL);
    exit(0);
}

void initialisationPoubelleCollective () {

    int i;
    for (i = 0; i < (NOMBRE_COLLECTIVE + NOMBRE_CARTON + NOMBRE_VERRE); i++) {
        if (i < NOMBRE_COLLECTIVE) {
            allBin[i].type == MENAGER;
        }
        else if (i < NOMBRE_COLLECTIVE + NOMBRE_VERRE) {
            allBin[i].type == VERRE;
        }
        else{
            allBin[i].type == CARTON;
        }
        allBin[i].volume = 1200;
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

    int a;
    pid_t pid_ramassage;
    int i, e = 0;
    int countCamion, rc;
    pthread_cond_init(&attente, 0);
    signal(SIGINT, fin);//libération des objets IPC
    /*
     * Mise des données du programme passeée en paremètre
     * dans des segments de mémoire partagée,
     * pour un accès simplifié entre touts les
     * processus/threads du programme
     */
     if (argc != 6) { // Vérification des paramètres et explications de ceux-ci, si erreur
        printf("*********Erreur dans l'entrée des paramètres !*************\n");
        printf("argv[1] : Nombre d'utilisateurs \n");
        printf("argv[2] : Nombre de camions \n");
        printf("argv[3] : Nombre de poubelles collectives \n");
        printf("argv[4] : Nombre de poubelles à verre \n");
        printf("argv[5] : Nombre de poubelles à carton \n");
        return EXIT_FAILURE;
    }
    // Données en mémoire partagées
    shmid_donnees = shmget(IPC_PRIVATE, 5*sizeof(int), 0666);
    donnees = (int *) shmat (shmid_donnees, NULL, 0);
    donnees[0] = atoi(argv[1]);
    donnees[1] = atoi(argv[2]);
    donnees[2] = atoi(argv[3]);
    donnees[3] = atoi(argv[4]);
    donnees[4] = atoi(argv[5]);
    shmid_poubelles = shmget(IPC_PRIVATE, (NOMBRE_VERRE + NOMBRE_COLLECTIVE + NOMBRE_CARTON)*sizeof(Poubelle), 0666);
    allBin = (Poubelle *) shmat (shmid_poubelles, NULL, 0);
    shmid_camions = shmget(IPC_PRIVATE, donnees[1]*sizeof(Ramasseur), 0666);
    initialisationPoubelleCollective();
    allTrucks = (Ramasseur *) shmat (shmid_camions, NULL, 0);
    pthread_t usager_id[NOMBRE_USAGER];
    pthread_t camion_id[NOMBRE_CAMION];
    shmid_users = shmget(IPC_PRIVATE, donnees[0]*sizeof(Usager), 0666);
    allUser = (Usager *) shmat (shmid_users, NULL, 0);
    for (i = 0; i < NOMBRE_USAGER; i++) { //création threads usagers
        pthread_mutex_lock(&mutex_utilisateur);
        rc = pthread_create (&usager_id[i], NULL, utiliser, (void *) &i);
        if (rc) {
            printf("ERROR ; return code from pthread_create() is %d\n", rc);
            exit(-1);
            if(rc == 11){
            printf("Lack of memory for another thread creation :/");
            }
        }
    }

    i = 0;
    for (e = 0; e < NOMBRE_CAMION; e++) {// création threads camion
        pthread_mutex_lock(&mutex_camion);
        rc = pthread_create(&camion_id[e], NULL, eboueur, (void *) &e);
        printf("camion %d\n", e);
        if (rc) {
            printf("ERROR ; return code from pthread_create is %d\n",rc);
            fin(SIGINT);
            exit(-1);
        }
    }
    e = 0;
    //attend la fin des threads
    for (i=0; i < NOMBRE_USAGER; i++) {
        pthread_join(usager_id[i], NULL);
    }
    for (i=0; i < NOMBRE_CAMION; i++) {
        pthread_join(camion_id[i], NULL);
    }
    //libération des objets IPC
    pthread_mutex_destroy(&mutex_poubelle);
    pthread_mutex_destroy(&mutex_poubelle_collective);
    pthread_mutex_destroy(&mutex_utilisateur);
    pthread_mutex_destroy(&mutex_camion);
    shmctl(shmid_donnees, IPC_RMID, NULL);
    shmctl(shmid_users, IPC_RMID, NULL);
    shmctl(shmid_poubelles, IPC_RMID, NULL);
    shmctl(shmid_camions, IPC_RMID, NULL);
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}
