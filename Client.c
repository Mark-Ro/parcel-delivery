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

void redColor()
{
    printf("\033[1;31m");
}

void greenColor()
{
    printf("\033[1;32m");
}

void cyanColor()
{
   printf("\033[1;36m");
}

void blueColor()
{
    printf("\033[1;34m");
}

void magentaColor()
{
    printf("\033[1;35m");
}

void lampeggiante()
{
    printf ("\033[5m");
}

void resetColor()
{
    printf("\033[0m");
}

int bufferVuoto(char buffer[])
{
    int i=0,risultato=1;;
    while (buffer[i]!=0)
    {
        if (buffer[i]!=' ' && buffer[i]!='\n')
            risultato=0;
        i++;
    }
    return risultato;
}

int sd,porta,indicePartitaEffettivo=0;
char indirizzoIP[100];

void accessoUtente(int);
void richiestaContinuoGioco();

void uscita()
{
    char buffer[100];
    redColor();
    printf ("\n\n\nIl client si sta arrestando...\n");
    resetColor();
    memset(buffer,0,100);
    strcpy(buffer,"quit");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    if (close(sd)==-1) { perror("Errore close socket:"); exit(-1); }
    exit(0);
}

void ignora()
{
    
}

int indexMat(char matrice[], int i, int j)
{
    return (i * NUMERO_RIGHE) + j;
}

void stampa_matrice (char matrice[])
{
    int i,j;

    blueColor();

    for(j=0;j<NUMERO_RIGHE;j++)
    {
        printf("  %d ",j);
    }

    printf("\n");

    printf ("-");

    for(j=0;j<NUMERO_RIGHE;j++)
    {
        printf("----");
    }

    printf("\n");

	for(i=0; i<NUMERO_RIGHE; i++)
    {
    	for(j=0; j<NUMERO_RIGHE; j++)
            if(matrice[indexMat(matrice,i,j)]!='1'){
                printf("\033[1;34m| \033[0m");
                if (matrice[indexMat(matrice,i,j)]=='#')
                    printf("\033[1;31m%c \033[0m",matrice[indexMat(matrice,i,j)]);
                else if (matrice[indexMat(matrice,i,j)]=='$')
                    printf("\033[1;33m%c \033[0m",matrice[indexMat(matrice,i,j)]);
                else if (matrice[indexMat(matrice,i,j)]=='@')
                    printf("\033[1;35m%c \033[0m",matrice[indexMat(matrice,i,j)]);
                else if (matrice[indexMat(matrice,i,j)]=='^') {
                    lampeggiante();
                    printf("\033[1;32m%c \033[0m",matrice[indexMat(matrice,i,j)]);
                    resetColor();
                }
                else if (matrice[indexMat(matrice,i,j)]=='B')
                    printf("\033[1;37m%c \033[0m",matrice[indexMat(matrice,i,j)]);
                else
                    printf("\033[1;35m%c \033[0m",matrice[indexMat(matrice,i,j)]);
                }
            else{
                printf("\033[1;34m|   \033[0m");
            }
        blueColor();
  		printf("| %d\n",i);

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

void visualizzaUtentiConnessi()
{
    char buffer[1000];
    memset(buffer,0,1000);
    if (read(sd,buffer,1000)<=0) { perror("Errore read:"); exit(-1); }
    blueColor();
    printf ("Lista utenti: %s\n",buffer);
    resetColor();
    memset(buffer,0,1000);
    strcpy(buffer,"Ok");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    memset(buffer,0,100);
}

void login(int sd)
{
    int i=0,uscita=0;    
    char carattere,buffer[100],stringa[100];
    memset(buffer,0,100);
    memset(stringa,0,100);
    do
    {
        greenColor();
        printf ("Inserisci nickname\n");
        resetColor();
        memset(stringa,0,100);
        i=0;
        scanf ("%s",buffer);
        while ((carattere=getchar())!='\n')
            stringa[i++]=carattere;
        if (strlen(stringa)==0)
            uscita=1;
        else
            printf("\033[1;31mErrore, Nickname non valido\033[0m\n\n");
    } while (uscita==0);
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    memset(buffer,0,100);
    if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
    uscita=0;
    if (strcmp(buffer,"Inserisci password")==0)
    {
        do
        {
            do
            {
                memset(buffer,0,100);
                memset(stringa,0,100);
                i=0;
                greenColor();
                printf ("Inserisci password\n");
                resetColor();
                scanf("%s",buffer);
                while ((carattere=getchar())!='\n')
                    stringa[i++]=carattere;
                if (strlen(stringa)!=0)
                    printf("\033[1;31mErrore, Password non valida\033[0m\n\n");
                else
                    uscita=1;
            } while (uscita==0);
            if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
            memset(buffer,0,100);
            if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
            if (strcmp(buffer,"Login riuscito!")==0)
                greenColor();
            else
                redColor();
            printf ("\n%s\n",buffer);
            resetColor();
        } while (!(strcmp(buffer,"Login riuscito!")==0));
        memset(buffer,0,100);
        strcpy(buffer,"Login effettuato!");
        if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    }
    else
    {
        redColor();
        if (strcmp(buffer,"Presente in lista")==0)
            printf ("Errore, utente collegato ed in partita!\n");
        else
            printf ("Errore, username non esiste!\n");
        resetColor();
        accessoUtente(sd);
    }
}

void registrazione(int sd)
{
    int i=0,successo=0,approvato=0,uscita=0;
    char carattere,buffer[100],stringa[100];
    do
    {
        approvato=0;
        do
        {
            do
            {
                memset(buffer,0,100);
                memset(stringa,0,100);
                i=0;
                greenColor();
                printf ("Inserisci Nickname\n");
                resetColor();
                scanf("%s",buffer);
                while ((carattere=getchar())!='\n')
                    stringa[i++]=carattere;
                if (strlen(stringa)!=0)
                    printf("\033[1;31mErrore, Nickname non valido\033[0m\n\n");
                else
                    uscita=1;
            } while (uscita==0);
            if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
            memset(buffer,0,100);
            if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
            if (strcmp(buffer,"Nickname approvato!")==0)
                greenColor();
            else
                redColor();
            printf ("%s\n",buffer);
            resetColor();
            if (strcmp(buffer,"Nickname approvato!")==0)
                approvato=1;
        } while (approvato==0);
        uscita=0;
        do
        {
            memset(buffer,0,100);
            memset(stringa,0,100);
            i=0;
            greenColor();
            printf ("Inserisci password\n");
            resetColor();
            scanf("%s",buffer);
            while ((carattere=getchar())!='\n')
                stringa[i++]=carattere;
            if (strlen(stringa)!=0)
                printf("\033[1;31mErrore, Password non valida\033[0m\n\n");
            else
                uscita=1;
        } while (uscita==0);
        if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
        memset(buffer,0,100);
        if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
        if (strcmp(buffer,"Errore")==0)
        {
            redColor();
            printf ("Errore di registrazione, nickname prenotato. Riprovare\n");
            resetColor();
        }
        else
        {
            greenColor();
            printf ("Registrazione avvenuta con successo\n");
            resetColor();
            successo=1;
        }
    } while(successo==0);
    memset(buffer,0,100);
    strcpy(buffer,"Login effettuato!");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
}

void accessoUtente(int sd)
{
    int i=0,esito=0;
    char carattere,buffer[100],stringa[100];
    do
    {
        memset(buffer,0,100);
        memset(stringa,0,100);
        i=0;
        greenColor();
        printf ("1.Login\n2.Registrazione\n\nScelta: ");
        resetColor();
        scanf ("%s",buffer);
        while ((carattere=getchar())!='\n')
            stringa[i++]=carattere;
        if (strlen(stringa)!=0)
        {
            memset(buffer,0,100);
            printf("\033[1;31mErrore, Scelta non valida\033[0m\n\n");
        }
        else if (strcmp(buffer,"1")!=0 && strcmp(buffer,"2")!=0)
            printf("\033[1;31mErrore, Scelta non valida\033[0m\n\n");
    } while (!(strcmp(buffer,"1")==0) && !(strcmp(buffer,"2")==0));
    system("clear");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }

    if ((strcmp(buffer,"1")==0))
        login(sd);
    else
        registrazione(sd);
}

int controlloInputRigaDiComando(int argc, char** argv)
{
    int porta;
    redColor();
    if (argc!=3)
    {
        printf ("Errore, richiesti indirizzo IP e Porta TCP!\n");
        resetColor();
        exit(-1);
    }
    porta=atoi(argv[2]);
    if (porta==0 || porta<5000 || porta>32768)
    {
        printf ("Inserire una porta valida!\n");
        resetColor();
        exit(-1);
    }
    resetColor();
    return porta;
}

int secondiRimanenti(int sd)
{
    int seconds;
    char buffer[100];
    memset(buffer,0,100);
    if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
    seconds=atoi(buffer);
    memset(buffer,0,100);
    strcpy(buffer,"Ok");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    return seconds;
}

int connessioneServer()
{
    struct sockaddr_in server_address;
    server_address.sin_family=AF_INET;
    server_address.sin_port=htons(porta);
    if (inet_aton(indirizzoIP,&server_address.sin_addr)==0) { printf ("Errore connessione!\n"); exit(-1); }
    if ((sd=socket(PF_INET,SOCK_STREAM,0))==-1) { perror("Errore socket:"); exit(-1); }
    if (connect(sd,(struct sockaddr *)&server_address,sizeof(server_address))==-1) { perror("Errore connect"); exit(-1); }
    return sd;
}

void spostamentoGiocatore()
{
    int i=0,uscita=0;
    char carattere,buffer[100],spostamento[100],stringa[100];
    do
    {
        greenColor();
        printf("Spostati: S=Sud, N=Nord, O=Ovest, E=Est!\n");
        memset(spostamento,0,100);
        resetColor();
        scanf("%s",spostamento);
        i=0;
        memset(buffer,0,100);
        while ((carattere=getchar())!='\n')
            buffer[i++]=carattere;
        if (!bufferVuoto(buffer))
            memset(spostamento,0,100);
    }
    while(strcmp(spostamento,"s") && strcmp(spostamento,"n") && strcmp(spostamento,"o") && strcmp(spostamento,"e"));
    if (write(sd,spostamento,strlen(spostamento))<=0) { perror("Errore write:"); exit(-1); } //Invia spostamento

    memset(buffer,0,100);
    memset(spostamento,0,100);
    if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
    strcpy(spostamento,buffer);
    memset(buffer,0,100);
    strcpy(buffer,"Ok");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    if (strcmp(spostamento,"Pacco")==0)
    {
        memset(buffer,0,100);
        if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
        do
        {
            magentaColor();
            printf ("Trovato pacco! Coordinate X: %c, Y: %c\n",buffer[0],buffer[1]);
            uscita=0;            
            do
            {
                greenColor();
                printf ("Vuoi prendere il pacco? Prendi: P, Deposita: D\n");
                resetColor();
                memset(spostamento,0,100);
                memset(stringa,0,100);
                i=0;
                scanf ("%s",spostamento);
                while ((carattere=getchar())!='\n')
                    stringa[i++]=carattere;
                if (strlen(stringa)==0)
                    uscita=1;
                else
                    printf("\033[1;31mErrore, input non valido!\033[0m\n");
            } while (uscita==0);
        }
        while (strcmp(spostamento,"p") && strcmp(spostamento,"d"));
        if (write(sd,spostamento,strlen(spostamento))<=0) { perror("Errore write:"); exit(-1); }
        memset(buffer,0,100);
        if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
        memset(spostamento,0,100);
        strcpy(spostamento,"Ok");
        if (write(sd,spostamento,strlen(spostamento))<=0) { perror("Errore write:"); exit(-1); }
        if (strcmp(buffer,"Errore")==0)
        {
            printf("\033[1;31mAttenzione, ti hanno rubato il pacco! Devi essere veloce!\033[0m\n");
            printf("\033[1;32mPremi un tasto per continuare\n\033[0m");
            while(getchar()!='\n');
            getchar();
        }
    }
}

void riceviVincitore()
{
    char buffer[1000];
    memset(buffer,0,1000);
    if (read(sd,buffer,1000)<=0) { perror("Errore read:"); exit(-1); }
    greenColor();
    printf ("%s\n",buffer);
    resetColor();
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    memset(buffer,0,1000);
    if (read(sd,buffer,1000)<=0) { perror("Errore read:"); exit(-1); }
    blueColor();
    printf("Numero pacchi consegnati dal giocatore %s\n",buffer);
    resetColor();
    memset(buffer,0,1000);
    strcpy(buffer,"Ok");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
}

void gioco()
{
    int i,exitFlag=0,seconds=1,uscitaFlag=0,indicePartitaCorrente=0;
    char buffer[100],spostamento[100];
    system("clear");
    greenColor();
    sd=connessioneServer();
    printf ("Benvenuto nel gioco!\n\n");
    resetColor();
    accessoUtente(sd);
    visualizzaUtentiConnessi();
    greenColor();
    printf ("Configurazione del gioco, attendere...\n");
    resetColor();
    seconds=secondiRimanenti(sd);
    memset(buffer,0,100);
    if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
    indicePartitaEffettivo=atoi(buffer);
    memset(buffer,0,100);
    strcpy(buffer,"Ok");
    if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
    memset(buffer,0,100);
    while (seconds>0)
    {
        system("clear");
        visualizzaUtentiConnessi();
        redColor();
        printf("Secondi rimanenti : %d\n",secondiRimanenti(sd));
        resetColor();
        if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
        stampa_matrice(buffer);
        memset(buffer,0,100);
        strcpy(buffer,"Ok");
        if (write(sd,buffer,strlen(buffer))<=0) { perror("Errore write:"); exit(-1); }
        memset(buffer,0,100);
        if (read(sd,buffer,100)<=0) { perror("Errore read:"); exit(-1); }
        indicePartitaCorrente=atoi(buffer);
        if (indicePartitaCorrente!=indicePartitaEffettivo)
            seconds=0;
        else
        {
            spostamentoGiocatore();
            seconds=secondiRimanenti(sd);
        }
    }
    if (indicePartitaCorrente==indicePartitaEffettivo)
        riceviVincitore();
    if (close(sd)==-1) { perror("Errore close socket:"); exit(-1); }
    indicePartitaEffettivo++;
    richiestaContinuoGioco();
}

void richiestaContinuoGioco()
{
    char buffer[100];
    do
    {
        greenColor();
        memset(buffer,0,100);
        printf ("GiocoFinito!\nVuoi iniziare una nuova sessione di gioco? Digita Y o n\n");
        resetColor();
        scanf ("%s",buffer);
    } while (strcmp("Y",buffer)!=0 && strcmp("y",buffer)!=0 && strcmp("n",buffer));
    resetColor();
    if (strcmp("Y",buffer)==0 || strcmp("y",buffer)==0)
    {
        greenColor();
        printf ("Connessione in corso...\n");
        resetColor();
        sleep(2);
        gioco();
    }
    else
        exit(0);
}

int main(int argc, char** argv)
{
    porta=controlloInputRigaDiComando(argc,argv);
    memset(indirizzoIP,0,100);
    strcpy(indirizzoIP,argv[1]);
    signal(SIGINT,uscita);
    signal(SIGPIPE,uscita);
    signal(SIGQUIT,uscita);
    signal(SIGTERM,uscita);
    signal(SIGSTOP,uscita);
    gioco();
    exit(0);
}
