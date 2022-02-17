/* librairie standard ... */
#include <stdlib.h>
/* pour getopt */
#include <unistd.h>
/* déclaration des types de base */
#include <sys/types.h>
/* constantes relatives aux domaines, types et protocoles */
#include <sys/socket.h>
/* constantes et structures propres au domaine UNIX */
#include <sys/un.h>
/* constantes et structures propres au domaine INTERNET */
#include <netinet/in.h>
/* structures retournées par les fonctions de gestion de la base de
données du réseau */
#include <netdb.h>
/* pour les entrées/sorties */
#include <stdio.h>
/* pour la gestion des erreurs */
#include <errno.h>

#define PROTOCOLE 0

void construire_message(char *message, char motif, int lg);
void afficher_message(char *message, int lg);
int creer_socket_local();
struct sockaddr_in* creer_adr_distante_puits();
void creer_adr_local_puits();
int envoi_msg(int taille_message);

int nb_message = -1; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
int source = -1 ; /* 0=puits, 1=source */
int taille_message = 30;
int typeProtocole;
int port;
char nom_station[25];

void main (int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;
	
    port = atoi(argv[argc-1]);
    strcpy(nom_station, argv[argc-2]);

	while ((c = getopt(argc, argv, "pl:n:su")) != -1) {
		switch (c) {
		case 'p':
			if (source == 1) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1);
			}
			source = 0;
			break;

		case 's':
			if (source == 0) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1) ;
			}
			source = 1;
			break;
		case 'n':
			nb_message = atoi(optarg);
			break;
        case 'u':
            typeProtocole = 0;
            break;
        case 'l':
            taille_message = atoi(optarg);
            break;
		default:
			printf("usage: cmd [-p|-s][-n ##]\n");
            typeProtocole = 1;
			break;
		}
	}

	if (source == -1) {
		printf("usage: cmd [-p|-s][-n ##]\n");
		exit(1) ;
	}

	if (nb_message != -1) {
		if (source == 1)
			printf("nb de tampons à envoyer : %d\n", nb_message);
		else
			printf("nb de tampons à recevoir : %d\n", nb_message);
	} else {
		if (source == 1) {
			nb_message = 10 ;
			printf("nb de tampons à envoyer = 10 par défaut\n");
		} else {
	    	printf("nb de tampons à envoyer = infini\n");
        }
	}
	if (source == 1){
		printf("on est dans le source\n");
        int err;
        err = envoi_msg(taille_message);
    }
	else{
		printf("on est dans le puits\n");

    }

} 

int envoi_msg(int taille_message){
    char *message = (char *)malloc(taille_message*sizeof(char));

    //1. Création socket
    int sock = creer_socket_local();
    if(sock<0){
        perror("Erreur socktet");
        exit(1);
    }
    // Adress distante
    struct hostent *hp;
    struct sockaddr_in adr_distant;
    memset((char*)&adr_distant, 0, sizeof(adr_distant));
    adr_distant.sin_family = AF_INET;
    adr_distant.sin_port = htons(port);        // port distant
    hp = gethostbyname(nom_station);             // nom station
    if(hp == NULL){
        perror("erreur gethostbyname\n");
        exit(1);
    }
    memcpy((char*)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
   
    // sendto
    construire_message(message,'a', taille_message);
    int err = sendto(sock, message, taille_message, 0, (struct sockaddr*)&adr_distant,sizeof(adr_distant));
    if(err < 0){
        perror("Erreur sendto");
        exit(1);
    }
    printf("Message de %d octets envoyé sur le port %d\n", err, port);
    printf("Options : %d msg, %d octets par msg\n", nb_message, taille_message);
    printf("ProtocoSOCK_DGRAMle : %d\n", source);  // spécifier protocole
    printf("Machine puits : %s\n", nom_station);
}

int creer_socket_local(){
    int sock;
    if (typeProtocole == 0){
        sock = socket(AF_INET, SOCK_DGRAM, PROTOCOLE);
    } else {
        sock = socket(AF_INET, SOCK_STREAM, PROTOCOLE);
    }
    if(sock == -1){
        printf("Echec de creation du socket\n");
        exit(1);
    }
    return sock;
}

// PUITS
void creer_adr_local_puits(){
    struct sockaddr_in adr_local;
    memset((char*)&adr_local, 0, sizeof(adr_local));
    adr_local.sin_family = AF_INET;
    adr_local.sin_port = 9999;
    adr_local.sin_addr.s_addr = INADDR_ANY;
}


void construire_message(char *message, char motif, int lg){
    int i;
    for (i=0;i<lg;i++){
        message[i] = motif; 
    }
}

void afficher_message(char *message, int lg) {
    int i;
    printf("message construit : ");
    for (i=0;i<lg;i++){
        printf("%c", message[i]);
    }       
    printf("\n");
}
