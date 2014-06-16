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

Usager *allUser;
Poubelle *allBin;
Ramasseur *allTrucks;
void viderPoubelle (Poubelle *p, int id, int usager) ;

int remplirPoubelle (int *id, int type, int semid, int semnum) {

    if (allUser[*id].dechets[type].type == allUser[*id].poubelleDuFoyer.type && allUser[*id].poubelleDuFoyer.remplissage + allUser[*id].dechets[type].volume <= allUser[*id].poubelleDuFoyer.volume && (allUser[*id].contrat == BAC || allUser[*id].contrat == CLE_BAC)) { //déchets ménager dans poubelle foyer
        P(semid, semnum);
        allUser[*id].poubelleDuFoyer.remplissage += allUser[*id].dechets[type].volume;
        printf("L'usager %d a déposé %d L de d'ordure dans sa propre poubelle\n", *id, allUser[*id].dechets[type].volume);
        allUser[*id].dechets[type].volume = 0;
        V(semid, semnum);
//        if (( (float)allUser[id].poubelleDuFoyer.remplissage / (float)allUser[id].poubelleDuFoyer.volume) > 0.7 ) signal(SIGUSR1, viderPoubelle);
        return TRUE;
    }
    else if ((allUser[*id].contrat == CLE || allUser[*id].contrat == CLE_BAC) && allUser[*id].dechets[type].type == MENAGER) {//déchets ménager dans bac collectif
        srand ((unsigned) time(NULL)) ;
        int poubelle = (rand() % NOMBRE_COLLECTIVE + 1);
        if (allBin[poubelle].remplissage + allUser[*id].dechets[type].volume < allBin[poubelle].volume) {
            allBin[poubelle].remplissage += allUser[*id].dechets[type].volume;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle collective.\n", *id, allUser[*id].dechets[type].volume);
            allUser[*id].dechets[type].volume = 0;
            allUser[*id].facturation_bac++;
//            if (( (float)allBin[id].remplissage / (float)allBin[id].volume) > 0.7 ) signal(SIGUSR1, viderPoubelle);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else if (allUser[*id].dechets[type].type == VERRE) { // dechets en verre
        srand ((unsigned) time(NULL)) ;
        int poubelle = (rand() % NOMBRE_VERRE + 1) + NOMBRE_COLLECTIVE;
        if (allBin[poubelle].remplissage + allUser[*id].dechets[type].volume < allBin[poubelle].volume) {
            allBin[poubelle].remplissage += allUser[*id].dechets[type].volume;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle verre.\n", *id, allUser[*id].dechets[type].volume);
            allUser[*id].dechets[type].volume = 0;
//            if (( (float)allBin[id].remplissage / (float)allBin[id].volume) > 0.7 ) signal(SIGUSR1, viderPoubelle);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else if (allUser[*id].dechets[type].type == CARTON) {//déchets en carton
        srand ((unsigned) time(NULL)) ;
        int poubelle = (rand() % NOMBRE_CARTON + 1) + NOMBRE_COLLECTIVE + NOMBRE_VERRE;
        if (allBin[poubelle].remplissage + allUser[*id].dechets[type].volume < allBin[poubelle].volume) {
            allBin[poubelle].remplissage += allUser[*id].dechets[type].volume;
            printf("L'usager %d a déposé %d L d'ordure dans une poubelle carton.\n", *id, allUser[*id].dechets[type].volume);
            allUser[*id].dechets[type].volume = 0;
//            if (( (float)allBin[id].remplissage / (float)allBin[id].volume) > 0.7 ) signal(SIGUSR1, viderPoubelle);
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    return FALSE;
}

void viderPoubelle (Poubelle *p, int id, int usager) {//Ramasseur camion, Poubelle poubellePleine) {

    allTrucks[id].remplissage += p->remplissage;
    p->remplissage = 0;
    if (usager) {
        //facturation
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

void *utiliser (void *num) { //Usager user){

    int *id = (int*) num;
    int i, j, semid, semnum;
    printf("création de l'utilisateur %d\n", *id);
    //initalisation des données utilisateur
    srand ((unsigned) time(NULL)) ;
    allUser[*id].contrat = rand() % 2 + 1;
    allUser[*id].foyer = rand() % 6 + 1;
    allUser[*id].dechets[0].type == MENAGER;
    allUser[*id].dechets[1].type == VERRE;
    allUser[*id].dechets[2].type == CARTON;
    if (allUser[*id].contrat == 0 || allUser[*id].contrat == 2) {
        allUser[*id].poubelleDuFoyer.remplissage = 0;
        compoFoyer(*id);
        allUser[*id].poubelleDuFoyer.type = MENAGER;
    }
    // lancement de son activité
    for (i = 0; i < JOURS; i++) {
        printf("Jour n°%d\n",i );
        for ( j = 0; j < 3; j++) {
            srand ((unsigned) time(NULL)) ;
            allUser[*id].dechets[j].volume = (rand() % 20 + 1); // Génére des déchets de O à 20L
            printf("volume des dechets de l'utilisateur %d à la journée %d : %d\n",*id,i,allUser[*id].dechets[j].volume);
            if (remplirPoubelle(id, allUser[*id].dechets[j].type, semid, semnum) == FALSE) {
                printf("L'utilisateur n°%d fait un depôt sauvage de %d L en plein milieu de la rue !!!\n", *id, allUser[*id].dechets[j].volume);
                allUser[*id].dechets[j].volume = 0;
                //dépôt sauvage, dans le cas où l'usager n'a pas pu vider sa poubelle ...
            }
            if (*id >= NOMBRE_USAGER) printf("ERROR\n");
        }
        sleep(1);// Dort une demi-seconde pour simuler journée et permettre un affichage agréable en console
    }
    //pthread_exit(&usager->addition);
    pthread_exit(NULL);
}

void *eboueur (void *num) { //Thread Camions

    int *id = (int *)num;
    int i = 0, j, semid, semnum;
    allTrucks[*id].capaCamion = 3000; //initialisation
    allTrucks[*id].remplissage = 0;
    Poubelle *poubelleAVider;
    pause();
    srand ((unsigned) time(NULL)) ;
    int choix = rand() % 2 + 1;
    if (choix == 1){// s'occupe des poubelles collectives
        while (allTrucks[*id].type == -1) {
            if ((float)allBin[i].remplissage / (float)allBin[i].volume > 0.7) {
                viderPoubelle(&allBin[i], *id,  FALSE);
                allTrucks[*id].type == allBin[*id].type;
            }
            i++;
        }
        while (allTrucks[*id].remplissage <= 2300) {
            for (i = 0; i < (NOMBRE_VERRE + NOMBRE_CARTON + NOMBRE_COLLECTIVE); i ++) {
                if (allBin[i].type == allTrucks[*id].type && ((float)allBin[i].remplissage / (float)allBin[i].volume) > 0.7) {
                    poubelleAVider = &allBin[i];
                }
            }
            viderPoubelle(poubelleAVider, *id, FALSE);
        }
    }
    else{//s'occupe des poubelles personnelles
        allTrucks[*id].type = MENAGER;
        while (allTrucks[*id].remplissage <= 2300) {
            for (i = 0; i < (NOMBRE_USAGER); i ++) {
                if (allUser[i].poubelleDuFoyer.type == allTrucks[*id].type && ((float)allUser[i].poubelleDuFoyer.remplissage / (float)allUser[i].poubelleDuFoyer.volume) > 0.7) {
                    poubelleAVider = &allUser[i].poubelleDuFoyer;
                }
            }
            viderPoubelle(poubelleAVider, *id, TRUE);
        }
    }
}

void displayFile (int signal) {//création du fichier pour les facturations clients

    FILE *facturation;
    facturation = fopen("facturation.txt", "w");
    int i;
    for(i = 0; i < NOMBRE_USAGER; i++){
        fprintf(facturation, "Usager numéro %d doit payer : %f\n", ++i, (float)(allUser[i].facturation_bac*3*allUser[i].poubelleDuFoyer.volume/10 + allUser[i].facturation_cle*5));
    }
    fclose(facturation);
    exit(0);
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
    for (i = 0; i < NOMBRE_USAGER; i++) { //création threads usagers
        rc = pthread_create (&usager_id[i], NULL, utiliser, (void *) &i);
        if (rc) {
            printf("ERROR ; return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
    }
    signal(SIGINT, displayFile);
    //LORSQUE POUBELLE PLEINE envoi un signal SIGUSR1 au centre de tri, pour vider la poubelle
    for (i = 0; i < NOMBRE_CAMION; ++i) {// création threads camion
        rc = pthread_create(&camion_id[i], NULL, eboueur, (void *) &i);
        if (rc) {
            printf("ERROR ; return code from pthread_create() is %d\n",rc);
            exit(-1);
        }
        countCamion++;
        allUser[i].facturation_bac++;
    }
    pthread_exit(NULL);
    shmctl(shmid_donnees, IPC_RMID, NULL);
    shmctl(shmid_users, IPC_RMID, NULL);
    shmctl(shmid_poubelles, IPC_RMID, NULL);
    shmctl(shmid_camions, IPC_RMID, NULL);
    return EXIT_SUCCESS;
}

// TODO

// Envoie du signaux sigusr1 poubelles(SIGUSR1) à Camion
// Mutex a vérifier
// Réseaux de pétrie ...
// Bug du n+1 users ..
