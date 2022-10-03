#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define NUMERO_RIGHE 10
#define SIZE NUMERO_RIGHE*NUMERO_RIGHE
#define NUMERO_PACCHI 10

void redColor()
{
    printf("\033[1;31m");
}

void cyanColor()
{
    printf("\033[1;34m");
}

void resetColor()
{
    printf("\033[0m");
}

//LOGIN

struct elemento
{
    char utente[100];
    struct elemento* next;
};

typedef struct elemento* List;

struct attributi_client
{
    int client_sd,posizioneX,posizioneY,zaino,numeroConsegneEffettuate,destinazioneX,destinazioneY;
    char identificativo,nickname[100];
};

typedef struct attributi_client* attributi_client;

struct scacchiera
{
    char matrice[100];
    pthread_mutex_t semaforoScacchiera;
};

typedef struct scacchiera* scacchiera;

struct giocatore_migliore
{
    char nickname[100];
    int numeroPacchi;
    pthread_mutex_t sem_GiocatoreMigliore;
};

typedef struct giocatore_migliore* Giocatore_migliore;

void accessoUtente(attributi_client);
void fineTempo();

scacchiera Scacchiera=NULL;
int server_sd,logging,seconds=3,porta,contatore_clients=0,numeroPacchi=NUMERO_PACCHI,indicePartita=0;
Giocatore_migliore giocatore_migliore;
struct sockaddr_in server_address,client_address;
List utenti=NULL;
pthread_mutex_t sem_File=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sem_List=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sem_NumeroPacchi=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sem_Tempo=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sem_Logging=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sem_contatore_clients=PTHREAD_MUTEX_INITIALIZER;

int indexMat(char matrice[], int i, int j)
{
    return (i * NUMERO_RIGHE) + j;
}

List nuovoElemento(char utente[])
{
    List e=(List)malloc(sizeof(struct elemento));
    memset(e->utente,0,100);
    strcpy(e->utente,utente);
    e->next=NULL;
    return e;
}

List insInTesta(List L, char utente[])
{
    List e=nuovoElemento(utente);
    e->next=L;
    return e;
}

List eliminaElemento(List L, char utente[])
{
    List tmp=L;
    if (L!=NULL)
        L->next=eliminaElemento(L->next,utente);

    if (L!=NULL)
    {
        if (strcmp(L->utente,utente)==0)
        {
            tmp=L;
            L=L->next;
            free(tmp);
        }
    }
    return L;
}

int isNicknamePresentInList(List utenti, char nickname[])
{
    int risultato=0;
    if (utenti!=NULL)
    {
        if (strcmp(utenti->utente,nickname)==0)
            risultato=1;
        else
            risultato=isNicknamePresentInList(utenti->next,nickname);
    }
    return risultato;
}

void stampaLista(List L, char array_utenti[])
{
    if (L!=NULL)
    {
        strcat(array_utenti,L->utente);
        strcat(array_utenti," ");
        stampaLista(L->next,array_utenti);
    }
}

int isNicknamePresentInFile(char nickname_input[])
{
    int esiste=0;
    char nickname_file[100],password_file[100];
    FILE* fp;
    pthread_mutex_lock(&sem_File);
    if ((fp=fopen("Utenti.txt","a+"))==NULL) { printf ("Errore login!\n"); exit(-1); }
    rewind(fp);
    while (!feof(fp) && esiste==0)
    {
        fscanf(fp,"%s%s",nickname_file,password_file);
        if (strcmp(nickname_input,nickname_file)==0)
            esiste=1;
    }
    fclose(fp);
    pthread_mutex_unlock(&sem_File);
    return esiste;
}

void login(attributi_client attributi)
{
    int trovato=0,client_sd=attributi->client_sd;
    char nickname_input[100],nickname_file[100],password_input[100],password_file[100],buffer[100];
    FILE* fp;
    memset(nickname_input,0,100);
    if (read(client_sd,nickname_input,100)<=0) { perror("Errore read:"); pthread_exit(NULL); }
    pthread_mutex_lock(&sem_File);
    if ((fp=fopen("Utenti.txt","a+"))==NULL) { printf ("Errore login!\n"); pthread_exit(NULL); }
    rewind(fp);
    while (!feof(fp) && trovato==0)
    {
        fscanf(fp,"%s%s",nickname_file,password_file);
        if (strcmp(nickname_input,nickname_file)==0 && !isNicknamePresentInList(utenti,nickname_input))
            trovato=1;
    }
    pthread_mutex_unlock(&sem_File);
    if (!trovato)
    {
        memset(buffer,0,100);
        if (!isNicknamePresentInList(utenti,nickname_input))
            strcpy(buffer,"Utente non trovato");
        else
            strcpy(buffer,"Presente in lista");
        if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
        accessoUtente(attributi);
    }
    else
    {
        fclose(fp);
        memset(buffer,0,100);
        strcpy(buffer,"Inserisci password");
        if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
        do
        {
            memset(password_input,0,100);
            if (read(client_sd,password_input,100)<=0) { perror("Errore read:"); pthread_exit(NULL); }
            if (!(strcmp(password_input,password_file)==0))
            {
                memset(buffer,0,100);
                strcpy(buffer,"Password errata! Riprovare\n");
                if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
            }
        } while (!(strcmp(password_input,password_file)==0));
        memset(buffer,0,100);
        strcpy(buffer,"Login riuscito!");
        if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
        memset(attributi->nickname,0,100);
        strcpy(attributi->nickname,nickname_input);
        pthread_mutex_lock(&sem_List);
        utenti=insInTesta(utenti,nickname_input);
        pthread_mutex_unlock(&sem_List);
        memset(buffer,0,100);
        if (read(client_sd,buffer,100)<=0) { perror("Errore read:"); pthread_exit(NULL); } //Legge Login Riuscito
    }
}

void registrazione(attributi_client attributi)
{
    int trovato=0,successo=0,client_sd=attributi->client_sd;
    char buffer[100],nickname_input[100],password_input[100];
    FILE* fp;
    do
    {
        trovato=0;
        do
        {
            memset(nickname_input,0,100);
            if (read(client_sd,nickname_input,100)<=0) { perror("Errore write:"); pthread_exit(NULL); }
            if (isNicknamePresentInFile(nickname_input))
            {
                memset(buffer,0,100);
                strcpy(buffer,"Nickname presente! Riprovare!");
                if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
            }
            else
                trovato=1;
        } while (trovato==0);

        memset(buffer,0,100);
        strcpy(buffer,"Nickname approvato!");
        if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
        memset(password_input,0,100);
        if (read(client_sd,password_input,100)<=0) { perror("Errore read:"); pthread_exit(NULL); }
        if (!isNicknamePresentInFile(nickname_input) && !(strcmp(password_input,"quit")==0))
        {
            pthread_mutex_lock(&sem_File);
            if ((fp=fopen("Utenti.txt","a"))==NULL) { printf ("Errore login!\n"); pthread_exit(NULL); }
            fprintf(fp,"%s %s\n",nickname_input,password_input);
            fclose(fp);
            pthread_mutex_unlock(&sem_File);
            memset(buffer,0,100);
            strcpy(buffer,"Successo");
            if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
            memset(buffer,0,100);
            successo=1;
        }
        else
        {
            memset(buffer,0,100);
            strcpy(buffer,"Errore");
            if (write(client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
            memset(buffer,0,100);
        }
    } while (successo==0);
    memset(attributi->nickname,0,100);
    strcpy(attributi->nickname,nickname_input);
    pthread_mutex_lock(&sem_List);
    utenti=insInTesta(utenti,nickname_input);
    pthread_mutex_unlock(&sem_List);
    memset(buffer,0,100);
    if (read(client_sd,buffer,100)<=0) { perror("Errore read:"); pthread_exit(NULL); } //Legge Login Riuscito
}

void logout(attributi_client attributi)
{
    utenti=eliminaElemento(utenti,attributi->nickname);
    if (attributi->zaino==1)
    {
        pthread_mutex_lock(&sem_NumeroPacchi);
        --numeroPacchi;
        pthread_mutex_unlock(&sem_NumeroPacchi);
    }
    pthread_mutex_lock(&(Scacchiera->semaforoScacchiera));
    Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
    pthread_mutex_unlock(&(Scacchiera->semaforoScacchiera));
    if (close(attributi->client_sd)<=0) perror("Errore close socket:");
    pthread_exit(NULL);
}

void accessoUtente(attributi_client attributi)
{
    int scelta=0,client_sd=attributi->client_sd;
    char buffer[100];
    do
    {
        memset(buffer,0,100);
        if (read(client_sd,buffer,100)<=0) { perror("Errore read:"); pthread_exit(NULL); }
        else
            scelta=atoi(buffer);
    } while (scelta!=1 && scelta!=2);

    if (scelta==1)
        login(attributi);
    else if (scelta==2)
        registrazione(attributi);
}

void inviaListaUtentiConnessi(attributi_client attributi)
{
    char buffer[1000];
    pthread_mutex_lock(&sem_List);
    memset(buffer,0,1000);
    stampaLista(utenti,buffer);
    if (write(attributi->client_sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); pthread_exit(NULL); }
    pthread_mutex_unlock(&sem_List);
    memset(buffer,0,1000);
    if (read(attributi->client_sd,buffer,1000)<=0) { perror("Errore read:"); pthread_exit(NULL); }
}

//----------------------------------------------------------------------------------------------------------------------------

void readFromClient(attributi_client attributi, char buffer[], int lunghezza)
{
    memset(buffer,0,lunghezza);
    if (read(attributi->client_sd,buffer,lunghezza)<=0) { perror("Errore read:"); logout(attributi); }
    if (strcmp(buffer,"quit")==0)
        logout(attributi);
}

void writeToClient(attributi_client attributi, char buffer[])
{
    if(write(attributi->client_sd,buffer,strlen(buffer))<=0)
    {
        perror("Errore write:");
        logout(attributi);
    }
}

void inserisciOstacoli(char matrice[])
{
    time_t t;
    srand((unsigned) time(&t));
    int i,j,cont=0;
    for (i=1;i<NUMERO_RIGHE-1;i++)
    {
        cont=0;
        while (cont!=1)
        {
            j=rand()%8+1;
            if (matrice[indexMat(matrice,i,j)]==' ')
            {
                matrice[indexMat(matrice,i,j)]='#';
                cont++;
            }
        }
    }
}

void inserisciOggetti(char matrice[])
{
    time_t t;
    srand((unsigned) time(&t));
    int i,j,cont=0;
    for (i=0;i<NUMERO_RIGHE;i++)
    {
        cont=0;
        while (cont!=1)
        {
            j=rand()%8+1;
            if (matrice[indexMat(matrice,i,j)]==' ')
            {
                matrice[indexMat(matrice,i,j)]='$';
                cont++;
            }
        }
    }

}

void inizializzaMatrice(char matrice[])
{
    int i,j;
    memset(matrice,0,SIZE);
    for (i=0;i<NUMERO_RIGHE;i++)
    {
        for (j=0;j<NUMERO_RIGHE;j++)
            matrice[indexMat(matrice,i,j)]=' ';
    }
    inserisciOstacoli(matrice);
    inserisciOggetti(matrice);
}

void stampa_matrice (char matrice[])
{
    int i,j;

    cyanColor();

    printf ("-");

    for(j=0;j<NUMERO_RIGHE;j++)
        printf("----");

    printf("\n");

	for(i=0; i<NUMERO_RIGHE; i++)
    {
    	for(j=0; j<NUMERO_RIGHE; j++)
    	{
            printf("\033[1;34m| \033[0m");

            if (matrice[indexMat(matrice,i,j)]=='#')
                printf("\033[1;31m%c \033[0m",matrice[indexMat(matrice,i,j)]);
            else if (matrice[indexMat(matrice,i,j)]=='$')
                printf("\033[1;33m%c \033[0m",matrice[indexMat(matrice,i,j)]);
            else
                printf("\033[1;35m%c \033[0m",matrice[indexMat(matrice,i,j)]);
        }

        cyanColor();

  		printf("|\n");

  		if(i!=NUMERO_RIGHE-1)
  		{
    		for(j=0;j<NUMERO_RIGHE;j++)
                printf("|---");

  			printf("|\n");
		}
	}

    printf ("-");

	for(j=0;j<NUMERO_RIGHE;j++)
        printf("----");

    printf("\n");

    resetColor();
}

void inizializzaScacchieraGiocatore(char matrice[], int ostacoli[],attributi_client attributi)
{
    int i,j;
    memset(matrice,0,100);
    strcpy(matrice,Scacchiera->matrice);
    for (i=0;i<NUMERO_RIGHE;i++)
        for (j=0;j<NUMERO_RIGHE;j++)
            if (matrice[indexMat(matrice,i,j)]=='#' && ostacoli[i]==0)
                matrice[indexMat(matrice,i,j)]='1';
    matrice[indexMat(matrice,attributi->posizioneY,attributi->posizioneX)]='^';
    if (attributi->zaino==1)
        matrice[indexMat(matrice,attributi->destinazioneY,attributi->destinazioneX)]='B';
}

scacchiera inizializzaScacchiera()
{
    inizializzaMatrice(Scacchiera->matrice);
    stampa_matrice(Scacchiera->matrice);
    pthread_mutex_lock(&sem_NumeroPacchi);
    numeroPacchi=NUMERO_PACCHI;
    pthread_mutex_unlock(&sem_NumeroPacchi);
    pthread_mutex_lock(&(giocatore_migliore->sem_GiocatoreMigliore));
    giocatore_migliore->numeroPacchi=0;
    indicePartita++;
    pthread_mutex_unlock(&(giocatore_migliore->sem_GiocatoreMigliore));
    return Scacchiera;
}

void inizializzaGiocatore(attributi_client attributi){
    time_t t;
    srand((unsigned) time(&t));
    int i,j;
    pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
    do{
        j = rand() % 8+1;
        i = rand() % 8+1;
    }
    while(Scacchiera->matrice[indexMat(Scacchiera->matrice,  i,  j)]!=' ');
    attributi->posizioneY=i;
    attributi->posizioneX=j;
    attributi->zaino=0;
    attributi->numeroConsegneEffettuate=0;
    attributi->destinazioneX=-1;
    attributi->destinazioneY=-1;
    Scacchiera->matrice[indexMat(Scacchiera->matrice,  i,  j)]='@';
    pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
}

void generaBase(char buffer[], int* destinazioneX, int* destinazioneY)
{
    time_t t;
    srand((unsigned) time(&t));
    memset(buffer,0,100);
    pthread_mutex_lock(&(Scacchiera->semaforoScacchiera));
    do
    {
        *destinazioneX=rand()%10;
        *destinazioneY=rand()%10;
    }
    while (Scacchiera->matrice[indexMat(Scacchiera->matrice,*destinazioneY,*destinazioneX)]!=' ');
    pthread_mutex_unlock(&(Scacchiera->semaforoScacchiera));
    sprintf(buffer,"%d%d",*destinazioneX,*destinazioneY);
}

void writeLoggingConsegnaPacchi(char nickname[])
{
    char buffer[100];
    time_t ora;
    ora=time(NULL);
    memset(buffer,0,100);
    strcpy(buffer,nickname);
    strcat(buffer," ");
    strcat(buffer,asctime(localtime(&ora)));
    pthread_mutex_lock(&sem_Logging);
    if(write(logging,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    pthread_mutex_unlock(&sem_Logging);
}

void verificaGiocatoreMassimo(attributi_client attributi)
{
    pthread_mutex_lock(&(giocatore_migliore->sem_GiocatoreMigliore));
    if (giocatore_migliore->numeroPacchi<attributi->numeroConsegneEffettuate)
    {
        memset(giocatore_migliore->nickname,0,100);
        strcpy(giocatore_migliore->nickname,attributi->nickname);
        giocatore_migliore->numeroPacchi=attributi->numeroConsegneEffettuate;
    }
    pthread_mutex_unlock(&(giocatore_migliore->sem_GiocatoreMigliore));
}

void inviaVincitore(attributi_client attributi)
{
    char buffer[1000];
    memset(buffer,0,1000);
    if (giocatore_migliore->numeroPacchi==0)
        sprintf(buffer,"Nessun vincitore, nessun pacco consegnato!");
    else
        sprintf(buffer,"Giocatore vincente: %s con %d pacchi consegnati!",giocatore_migliore->nickname,giocatore_migliore->numeroPacchi);
    writeToClient(attributi,buffer);
    memset(buffer,0,1000);
    readFromClient(attributi,buffer,1000);
    memset(buffer,0,1000);
    sprintf(buffer,"%d",attributi->numeroConsegneEffettuate);
    writeToClient(attributi,buffer);
    memset(buffer,0,1000);
    readFromClient(attributi,buffer,1000);
}

void spostamentoGiocatore(char movimento,attributi_client attributi,int ostacoli []){
    int tmpX=attributi->posizioneX,tmpY=attributi->posizioneY,destinazioneX=-1,destinazioneY=-1;
    char buffer[100];
    if(movimento=='s')
    {
        tmpY++;
        if(tmpY<=9){
            if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]==' '){
                pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                attributi->posizioneY=tmpY;
                attributi->posizioneX=tmpX;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='#') {
                ostacoli[attributi->posizioneY+1]=1;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
            {
                writeToClient(attributi,"Pacco");
                readFromClient(attributi,buffer,100); //Legge OK
                generaBase(buffer,&destinazioneX,&destinazioneY);
                writeToClient(attributi,buffer); //Scrive coordinate
                readFromClient(attributi,buffer,100); //Legge P o D
                if (strcmp(buffer,"p")==0)
                {
                    if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
                    {
                        writeToClient(attributi,"Ok");
                        readFromClient(attributi,buffer,100);
                        pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                        pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                        attributi->posizioneY=tmpY;
                        attributi->posizioneX=tmpX;
                        attributi->destinazioneX=destinazioneX;
                        attributi->destinazioneY=destinazioneY;
                        attributi->zaino=1;
                    }
                    else {
                        writeToClient(attributi,"Errore");
                        readFromClient(attributi,buffer,100);
                    }
                }
                else {
                    writeToClient(attributi,"Ok");
                    readFromClient(attributi,buffer,100);
                }
            }
            else
            {
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
        }
        else
        {
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
        }
    }

    if(movimento=='n')
    {
        tmpY--;
        if(tmpY>=0){
            if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]==' '){
                pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                attributi->posizioneY=tmpY;
                attributi->posizioneX=tmpX;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='#'){
                ostacoli[attributi->posizioneY-1]=1;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
            {
                writeToClient(attributi,"Pacco");
                readFromClient(attributi,buffer,100);
                generaBase(buffer,&destinazioneX,&destinazioneY);
                writeToClient(attributi,buffer);
                readFromClient(attributi,buffer,100);
                if (strcmp(buffer,"p")==0)
                {
                    if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
                    {
                        writeToClient(attributi,"Ok");
                        readFromClient(attributi,buffer,100);
                        pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                        pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                        attributi->posizioneY=tmpY;
                        attributi->posizioneX=tmpX;
                        attributi->destinazioneX=destinazioneX;
                        attributi->destinazioneY=destinazioneY;
                        attributi->zaino=1;
                    }
                    else {
                        writeToClient(attributi,"Errore");
                        readFromClient(attributi,buffer,100);
                    }
                }
                else {
                    writeToClient(attributi,"Ok");
                    readFromClient(attributi,buffer,100);
                }
            }
            else
            {
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
        }

        else
        {
            writeToClient(attributi,"Ok");
            readFromClient(attributi,buffer,100);
        }
    }

    if(movimento=='e')
    {
        tmpX++;
        if(tmpX<=9){
            if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]==' '){
                pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                attributi->posizioneY=tmpY;
                attributi->posizioneX=tmpX;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='#'){
                ostacoli[attributi->posizioneY]=1;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
            {
                writeToClient(attributi,"Pacco");
                readFromClient(attributi,buffer,100);
                generaBase(buffer,&destinazioneX,&destinazioneY);
                writeToClient(attributi,buffer);
                readFromClient(attributi,buffer,100);
                if (strcmp(buffer,"p")==0)
                {
                    if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
                    {
                        writeToClient(attributi,"Ok");
                        readFromClient(attributi,buffer,100);
                        pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                        pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                        attributi->posizioneY=tmpY;
                        attributi->posizioneX=tmpX;
                        attributi->destinazioneX=destinazioneX;
                        attributi->destinazioneY=destinazioneY;
                        attributi->zaino=1;
                    }
                    else {
                        writeToClient(attributi,"Errore");
                        readFromClient(attributi,buffer,100);
                    }
                }
                else {
                    writeToClient(attributi,"Ok");
                    readFromClient(attributi,buffer,100);
                }
            }
            else
            {
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
        }

        else
        {
            writeToClient(attributi,"Ok");
            readFromClient(attributi,buffer,100);
        }
    }

    if(movimento=='o')
    {
        tmpX--;
        if(tmpX>=0){
            if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]==' '){
                pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                attributi->posizioneY=tmpY;
                attributi->posizioneX=tmpX;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if(Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='#'){
                ostacoli[attributi->posizioneY]=1;
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
            else if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
            {
                writeToClient(attributi,"Pacco");
                readFromClient(attributi,buffer,100);
                generaBase(buffer,&destinazioneX,&destinazioneY);
                writeToClient(attributi,buffer);
                readFromClient(attributi,buffer,100);
                if (strcmp(buffer,"p")==0)
                {
                    if (Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]=='$' && attributi->zaino==0)
                    {
                        writeToClient(attributi,"Ok");
                        readFromClient(attributi,buffer,100);
                        pthread_mutex_lock(&Scacchiera->semaforoScacchiera);
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,attributi->posizioneY,attributi->posizioneX)]=' ';
                        Scacchiera->matrice[indexMat(Scacchiera->matrice,tmpY,tmpX)]='@';
                        pthread_mutex_unlock(&Scacchiera->semaforoScacchiera);
                        attributi->posizioneY=tmpY;
                        attributi->posizioneX=tmpX;
                        attributi->destinazioneX=destinazioneX;
                        attributi->destinazioneY=destinazioneY;
                        attributi->zaino=1;
                    }
                    else {
                        writeToClient(attributi,"Errore");
                        readFromClient(attributi,buffer,100);
                    }
                }
                else {
                    writeToClient(attributi,"Ok");
                    readFromClient(attributi,buffer,100);
                }
            }
            else
            {
                writeToClient(attributi,"Ok");
                readFromClient(attributi,buffer,100);
            }
        }

        else
        {
            writeToClient(attributi,"Ok");
            readFromClient(attributi,buffer,100);
        }
    }

    if (attributi->posizioneX==attributi->destinazioneX && attributi->posizioneY==attributi->destinazioneY && attributi->zaino==1)
    {
        attributi->zaino=0;
        pthread_mutex_lock(&sem_NumeroPacchi);
        --numeroPacchi;
        pthread_mutex_unlock(&sem_NumeroPacchi);
        (attributi->numeroConsegneEffettuate)++;
        verificaGiocatoreMassimo(attributi);
        writeLoggingConsegnaPacchi(attributi->nickname);
    }
}

//----------------------------------------------------------------------------------------------------------------------------

void inviaTimer(attributi_client attributi)
{
    char buffer[100];
    memset(buffer,0,100);
    sprintf(buffer,"%d",seconds);
    writeToClient(attributi,buffer);
    readFromClient(attributi,buffer,100);
}

void timerGioco()
{
    while (seconds>0)
    {
        if (seconds%60==0)
            printf("\033[1;32mMinuti rimanenti %d\033[0m\n",seconds/60);
        sleep(1);
        pthread_mutex_lock(&sem_Tempo);
        seconds--;
        pthread_mutex_unlock(&sem_Tempo);
    }
}

void chiusuraSessioneGioco()
{
    contatore_clients=0;
    while (utenti!=NULL)
        utenti=eliminaElemento(utenti,utenti->utente);
}

void* gestisciTempo(void* arg)
{
    seconds=182;
    timerGioco();
    chiusuraSessioneGioco();
    pthread_exit(NULL);
}

void ignoraSegnale()
{
    printf ("Ricevuto Segnale!\n");
}

void uscita()
{
    redColor();
    printf ("\n\nIl server si sta arrestando...\n");
    resetColor();
    if (close(server_sd)==-1) { perror("Errore close socket:"); exit(-1); }
    if (close(logging)==-1) { perror("Errore close logging:"); exit(-1); }
    exit(0);
}

void* gestisciGioco(void* thread_sd)
{
    int* ptr_client_sd=(int *)thread_sd;
    int client_sd=*ptr_client_sd,uscita=0,errore,ostacoli[10]={0};
    char buffer[100],scacchiera_giocatore[100],movimentoClient;
    pthread_t tid_gestisciTempo;
    pthread_attr_t attr;
    attributi_client attributi=(attributi_client)malloc(sizeof(struct attributi_client));
    attributi->client_sd=client_sd;
    accessoUtente(attributi);
    inviaListaUtentiConnessi(attributi);
    if (contatore_clients==0)
    {
        if ((errore=pthread_attr_init(&attr))!=0) { printf ("Errore creazione del thread: %s\n",strerror(errore)); exit(-1); }
        if ((errore=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED))==0)
            if ((errore=pthread_create(&tid_gestisciTempo,&attr,gestisciTempo,NULL))) { printf ("Errore: %s\n",strerror(errore)); exit(-1); }
        Scacchiera=inizializzaScacchiera();
    }
    pthread_mutex_lock(&sem_contatore_clients);
    contatore_clients++;
    pthread_mutex_unlock(&sem_contatore_clients);
    sleep(2);
    inviaTimer(attributi);
    memset(buffer,0,100);
    sprintf(buffer,"%d",indicePartita);
    writeToClient(attributi,buffer);
    readFromClient(attributi,buffer,100);
    inizializzaGiocatore(attributi);
    while (seconds>=0 && uscita==0)
    {
        inviaListaUtentiConnessi(attributi);
        inviaTimer(attributi);
        stampa_matrice(Scacchiera->matrice);
        inizializzaScacchieraGiocatore(scacchiera_giocatore,ostacoli,attributi);
        printf("\033[1;32mNumero pacchi: %d\033[0m\n",numeroPacchi);
        writeToClient(attributi,scacchiera_giocatore);
        readFromClient(attributi,buffer,100);
        memset(buffer,0,100);
        sprintf(buffer,"%d",indicePartita);
        writeToClient(attributi,buffer);
        readFromClient(attributi,buffer,100); //Riceve spostamento
        movimentoClient=buffer[0];
        spostamentoGiocatore(movimentoClient,attributi,ostacoli);
        stampa_matrice(Scacchiera->matrice);
        if (numeroPacchi==0)
        {
            pthread_mutex_lock(&sem_Tempo);
            seconds=0;
            pthread_mutex_unlock(&sem_Tempo);
        }
        if (seconds<=0)
            uscita=1;
        inviaTimer(attributi);
    }
    inviaVincitore(attributi);
    pthread_exit(NULL);;
}

void writeLoggingIP()
{
    char bufferData[100],str[INET_ADDRSTRLEN];
    struct sockaddr_in* pV4Addr;
    struct in_addr ipAddr;
    time_t ora;
    ora=time(NULL);
    pV4Addr=(struct sockaddr_in*)&client_address;
    ipAddr=pV4Addr->sin_addr;
    inet_ntop(AF_INET,&ipAddr,str,INET_ADDRSTRLEN);
    memset(bufferData,0,100);
    strcpy(bufferData,str);
    strcat(bufferData," ");
    strcat(bufferData,asctime(localtime(&ora)));
    printf("%s\n",bufferData);
    pthread_mutex_lock(&sem_Logging);
    if(write(logging,bufferData,strlen(bufferData))<=0) { perror("Errore write:"); exit(0); }
    pthread_mutex_unlock(&sem_Logging);
}

void* gestisciConnessioni()
{
    int connect_sd,errore,*thread_sd;
    pthread_t tid_gestisci;
    pthread_attr_t attr;
    socklen_t client_len;
    while (1)
    {
        client_len=sizeof(client_address);
        if ((connect_sd=accept(server_sd,(struct sockaddr *)&client_address,&client_len))==-1) { perror("Errore accept: "); exit(-1); }
        thread_sd=(int *)malloc(sizeof(int));
        *thread_sd=connect_sd;
        printf("\033[1;35mCollegato nuovo giocatore!\033[0m\n");
        writeLoggingIP();
        if ((errore=pthread_attr_init(&attr))!=0) { printf ("Errore creazione del thread: %s\n",strerror(errore)); exit(-1); }
        if ((errore=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED))==0)
            if ((errore=pthread_create(&tid_gestisci,&attr,gestisciGioco,(void *)thread_sd))) { printf ("Errore: %s\n",strerror(errore)); exit(-1); }
    }
    return NULL;
}

void creaServer()
{
    int errore,opt=1;
    pthread_t tid_connessione;
    pthread_attr_t attr;
    server_address.sin_family=AF_INET;
    server_address.sin_port=htons(porta);
    server_address.sin_addr.s_addr=htonl(INADDR_ANY);
    if ((server_sd=socket(PF_INET,SOCK_STREAM,0))==-1) { perror("Errore socket: "); exit (-1); }
    if (setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT,&opt,sizeof(opt))) { perror("Errore setsockopt:"); exit(-1); }
    if (bind(server_sd,(struct sockaddr *)&server_address,sizeof(server_address))==-1) { perror("Errore bind: "); exit(-1); }
    if (listen(server_sd,1)==-1) { perror("Errore listen: "); exit(-1); }
    if((logging=open("logging.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR))<=0){ perror("Errore logging:"); }
    seconds=1;
    Scacchiera=(scacchiera)malloc(sizeof(struct scacchiera));
    pthread_mutex_init(&(Scacchiera->semaforoScacchiera),NULL);
    system("clear");
    printf("\033[1;32mServer avviato!\033[0m\n\n");
    if ((errore=pthread_attr_init(&attr))!=0) { printf ("Errore creazione del thread: %s\n",strerror(errore)); exit(-1); }
    if ((errore=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED))==0)
        if ((errore=pthread_create(&tid_connessione,&attr,gestisciConnessioni,NULL))) { printf ("Errore: %s\n",strerror(errore)); exit(-1); }

    giocatore_migliore=(Giocatore_migliore)malloc(sizeof(struct giocatore_migliore));
    giocatore_migliore->numeroPacchi=0;
    pthread_mutex_init(&(giocatore_migliore->sem_GiocatoreMigliore),NULL);
    while(1);
}

int controlloInputRigaDiComando(int argc, char** argv)
{
    int porta;
    if (argc!=2)
    {
        printf("\033[1;31mErrore, richiesta la porta TCP su cui mettersi in ascolto\033[0m\n\n");
        exit(-1);
    }
    porta=atoi(argv[1]);
    if (porta==0 || porta<5000 || porta>32768)
    {
        printf("\033[1;31mInserire una porta valida!\033[0m\n\n");
        exit(-1);
    }
    return porta;
}

int main(int argc, char** argv)
{

    porta=controlloInputRigaDiComando(argc,argv);
    signal(SIGINT,uscita);
    signal(SIGQUIT,uscita);
    signal(SIGTERM,uscita);
    signal(SIGPIPE,SIG_IGN);
    signal(SIGSTOP,uscita);
    creaServer();
}
