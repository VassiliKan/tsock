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
/* pour la manipulation des strings */
#include <string.h>

#define PROTOCOLE 0






/*

Avec UDP + option -n : while(1) ou pas ?

Message émis formaté, message reçus juste print

*/





void construire_message(char *message, char motif, int lg);
void afficher_message(char *message, int lg);
char* formatMessage(int a);
int countChiffres(int a);
int creer_socket_local();
int envoi_msg_UDP();
int recevoir_msg_UDP();
int envoi_msg_TCP();
int recevoir_msg_TCP();

int nb_message = 10; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
int source =-1 ; /* 0=puits, 1=source */
int taille_message = 30;
int typeProtocole = 1; // par défaut, protocole = 1 = TCP
int port;
char nom_station[25];
char nomProtocole[5] = "TCP";

void main (int argc, char **argv)
{
    int err;
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
            strcpy(nomProtocole, "UDP");
            break;
        case 'l':
            taille_message = atoi(optarg);
            break;
		default:
			printf("usage: cmd [-p|-s][-n ##]\n");
			break;
		}
	}

	if (source == -1) {
		printf("usage: cmd [-p|-s][-n ##]\n");
		exit(1) ;
	}

	if (nb_message != -1) {
		if (source == 1)
			printf("Nombre de tampons à envoyer : %d\n", nb_message);
		else
			printf("Nombre de tampons à recevoir : %d\n", nb_message);
	} else {
		if (source == 1) {
			printf("Nombre de tampons à envoyer = 10 par défaut\n");
		} else {
	    	printf("Nombre de tampons à envoyer = infini\n");
        }
	}

	if (source == 1){
		printf("On est dans la source\n");
        if (typeProtocole == 0){
            err = envoi_msg_UDP();
        } else {
            err = envoi_msg_TCP();
        }
    }
	else {
		printf("On est dans le puits\n");
        if (typeProtocole == 0){
            err = recevoir_msg_UDP();
        } else {
            err = recevoir_msg_TCP();
        }
    }

} 

                    ///////////////
///////////////////////   UDP   ///////////////////////
                    ///////////////


int envoi_msg_UDP(){
    char *chaine = (char *)malloc(taille_message*sizeof(char)); // contient la chaine composé d'un caractere fixe
    char *message = (char *)malloc((5+taille_message)*sizeof(char)); // contient la chaine ci-dessus prefixee du format '---nb'

    //1. Création socket
    int sock = creer_socket_local();

    // Création de l'adresse distante
    struct hostent *hp;
    struct sockaddr_in adr_distant;
    memset((char*)&adr_distant, 0, sizeof(adr_distant));
    adr_distant.sin_family = AF_INET;
    adr_distant.sin_port = htons(port);        // port distant
    hp = gethostbyname(nom_station);             // nom station
    if(hp == NULL){
        perror("SOURCE : Erreur gethostbyname\n");
        exit(1);
    }
    memcpy((char*)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
   
    // Envoi des messages au puits
    int i;    
    printf("SOURCE : taille_msg_emis=%d, port=%d, nb_msg=%d, protocol=%s, dest=%s\n", taille_message, port, nb_message, nomProtocole, nom_station);
    for(i=0; i<nb_message; i++){
        construire_message(chaine, (char)(i%26)+97, taille_message);
        strcpy(message, formatMessage(i+1));
        strcat(message, chaine);
        int err = sendto(sock, message, taille_message, 0, (struct sockaddr*)&adr_distant,sizeof(adr_distant));
        if(err < 0){
            perror("SOURCE : Erreur sendto");
            exit(1);
        }
        printf("SOURCE : Envoi n°%d (%d) [%s]\n", i+1, taille_message, message);
    }
    close(sock);
    return 0;
}


int recevoir_msg_UDP(){
    //1. Création socket
    int sock = creer_socket_local();

    //2. Construction adresse du socket
    struct sockaddr_in adr_local;
    int lg_adr_local = sizeof(adr_local);
    memset((char*)&adr_local, 0, sizeof(adr_local)) ; 
    adr_local.sin_family = AF_INET ;
    adr_local.sin_port = htons(port);
    adr_local.sin_addr.s_addr = INADDR_ANY ;

    //3. Bind de l'adresse
    int err = bind (sock, (struct sockaddr *)&adr_local, lg_adr_local); 
    if (err == -1){ 
        printf("PUITS : Echec du bind\n") ;
        exit(1); 
    }

    //4. Reception des messages
    printf("PUITS : taille_msg_lu=%d, port=%d, nb_msg=%d, protocol=%s\n", taille_message, port, nb_message, nomProtocole);
    int nb_msg_recus = 0;    
    int plg_adr_em;
    char buffer[taille_message];
    struct sockaddr padr_em;
    while(1){
        err = recvfrom(sock, &buffer, taille_message, 0, &padr_em, &plg_adr_em);
        nb_msg_recus++;
        printf("PUITS : Reception n°%d (%d) [%s]\n",nb_msg_recus, taille_message, buffer);
    }
    close(sock);
    return 0;
}


                    ///////////////
///////////////////////   TCP   ///////////////////////
                    ///////////////


int envoi_msg_TCP(){
    char *chaine = (char *)malloc(taille_message*sizeof(char)); // contient la chaine composé d'un caractere fixe
    char *message = (char *)malloc((5+taille_message)*sizeof(char)); // contient la chaine ci-dessus prefixee du format '---nb'

    //1. Création socket
    int sock = creer_socket_local();

    // Création de l'adresse distante
    struct hostent *hp;
    struct sockaddr_in adr_distant;
    memset((char*)&adr_distant, 0, sizeof(adr_distant));
    adr_distant.sin_family = AF_INET;
    adr_distant.sin_port = htons(port);        // port distant
    hp = gethostbyname(nom_station);           // nom station
    if(hp == NULL){
        perror("SOURCE : Erreur gethostbyname()\n");
        exit(1);
    }
    memcpy((char*)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
   
    // Connexion à l'adresse distante
    int err = connect(sock, (struct sockaddr*)&adr_distant, sizeof(adr_distant));
    if(err == -1){
        printf("SOURCE : Erreur connexion\n");
        exit(1);
    }

    // Envoi des messages au puits
    int i;
    printf("SOURCE : taille_msg_emis=%d, port=%d, nb_msg=%d, protocol=%s, dest=%s\n", taille_message, port, nb_message, nomProtocole, nom_station);
    for(i=0; i<nb_message; i++){
        construire_message(chaine, (char)(i%26)+97, taille_message);
        strcpy(message, formatMessage(i+1));
        strcat(message, chaine);
        err = sendto(sock, message, taille_message, 0, (struct sockaddr*)&adr_distant,sizeof(adr_distant));
        if(err < 0){
            perror("SOURCE : Erreur sendto");
            exit(1);
        }
        printf("SOURCE : Envoi n°%d (%d) [%s]\n", i+1, taille_message, message);
    }
    close(sock);
    return 0;
}


int recevoir_msg_TCP(){
    //1. Création socket
    int sock = creer_socket_local();

    //2. Construction adresse du socket
    struct sockaddr_in adr_local;
    int lg_adr_local = sizeof(adr_local);
    memset((char*)&adr_local, 0, sizeof(adr_local)) ; 
    adr_local.sin_family = AF_INET ;
    adr_local.sin_port = htons(port);
    adr_local.sin_addr.s_addr = INADDR_ANY ;

    //3. Bind de l'adresse
    int err = bind (sock, (struct sockaddr *)&adr_local, lg_adr_local); 
    if (err == -1){ 
        printf("PUITS : Echec du bind\n") ;
        exit(1); 
    }

    // Connexion
    err = listen(sock, 5);
    if(err == -1){
        printf("PUITS : Erreur listen\n");
        exit(1);
    }

    struct sockaddr padr_client;
    int plg_adr_client;
    int sock_bis = accept(sock, &padr_client, &plg_adr_client);
    if(sock_bis == -1){
        printf("PUITS : Erreur accept\n");
        exit(1);
    }

    //4. Reception des messages
    printf("PUITS : taille_msg_lu=%d, port=%d, nb_msg=%d, protocol=%s\n", taille_message, port, nb_message, nomProtocole);
    int i;
    int lg_max=30;
    int taille_msg_recu;
    char buffer[taille_message];
    for (i=0 ; i < nb_message; i ++) {
        if ((taille_msg_recu = read(sock_bis, buffer, lg_max)) < 0){
            printf("PUITS : Echec read\n");
            exit(1) ;
        }
        printf("PUITS : Reception n°%d (%d) [%s]\n", i+1, taille_message, buffer);
    }
    close(sock);
    return 0;
}


                    //////////////////
///////////////////////   SOCKET   ///////////////////////
                    //////////////////


int creer_socket_local(){
    int sock;
    if (typeProtocole == 0){
        // UDP
        sock = socket(AF_INET, SOCK_DGRAM, PROTOCOLE);
    } else {
        // TCP
        sock = socket(AF_INET, SOCK_STREAM, PROTOCOLE);
    }
    if(sock == -1){
        printf("Echec de creation du socket\n");
        exit(1);
    }
    return sock;
}


                    ///////////////////
///////////////////////   MESSAGE   ///////////////////////
                    ///////////////////


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

// Construit le préfixe du message envoyé, composé de '-' et du numéro de l'envoi
char* formatMessage(int a){
    char nbStr[10];
	sprintf(nbStr, "%d", a);
	int n = strlen(nbStr);     // compte le nombre de chiffre composant le numéro du message envoyé
	char *prefixe;
	prefixe = malloc((5)*sizeof(char));
	if (5-n != 0){
		memset(prefixe, '-', (5-n)*sizeof(char));  // assigne à la chaine appelee prefixe le nombre de caractere '-' necessaire 
        memset(prefixe+(5-n), '\0', 1);
	}
    strcat(prefixe, nbStr); // concatene les '-' et le numero d'envoi (convertit sous forme de chaine)
	return prefixe;
}

