/* He inclòs totes les llibreries presents en el fronton3.c */
#include <stdio.h>	/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "winsuport2.h"	/* incloure definicions de funcions propies */
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"


#define MIDA_PALETA 4	/* definicions constants del programa */
#define MAX_PILOTES 9
#define MAX_REBOTS 5

char missatge_enviar[100];

/* funcio per moure la pilota: retorna un 1 si la pilota surt per la porteria,*/
/* altrament retorna un 0 */

/* Necessita rebre per valor:
vel_f, vel_c,f_pil, c_pil, pos_f, pos_c
Necessita rebre per parámetre:
fi2, num_rebots
*/

int num_pil;
float vel_f, vel_c;
int id_mis[MAX_PILOTES];
int jocEnMarxa = 1;
pthread_t tid; //Thread per la bustia
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;	/* crea un sem. Global*/

void * escolta_bustia(void *n_v){
	char entrada[100], sortida[100];	
	while(jocEnMarxa==1){
		receiveM(id_mis[num_pil], entrada);
		fprintf(stderr,"entrada: %s\n", entrada);
		if(entrada[0]=='e'){
			jocEnMarxa = 0;
		}else{		
			int pilota_t;
			pthread_mutex_lock(&mutex);
			float vel_f_t = 0.0;
			float vel_c_t = 0.0;
			pthread_mutex_unlock(&mutex);
			char direccio;
			char resposta;
			sscanf(entrada, "%i;%f;%f;%c;%c", &pilota_t, &vel_f_t, &vel_c_t, &direccio, &resposta);
			fprintf(stderr,"entrada des : %i;%f;%f;%c;%c\n", pilota_t, vel_f_t, vel_c_t, direccio, resposta);
			fprintf(stderr,"pilota llegida : %i; pilota actual: %i;resposta: %c\n", pilota_t, 1+num_pil, direccio);
			pthread_mutex_lock(&mutex);		
			if(resposta == 's'){
				fprintf(stderr,"responc missatge\n");
				fprintf(stderr,"sortida: %i;%f;%f;%c;n\n", 1+num_pil, vel_f, vel_c, direccio);
				sprintf(sortida, "%i;%f;%f;%c;n", 1+num_pil, vel_f, vel_c, direccio);			
				sendM(id_mis[pilota_t-1], sortida, 100);
			}
			if(direccio == 'd'){
				vel_f = vel_f_t;
				vel_c = vel_c_t;
				fprintf(stderr,"xoc d\n");
			}
			if(direccio == 'v'){
				vel_c = vel_c_t;
				fprintf(stderr,"xoc v\n");
			}
			if(direccio == 'h'){
				vel_f = vel_f_t;
				fprintf(stderr,"xoc h\n");
			}
			pthread_mutex_unlock(&mutex);
		}
	}
	return n_v;
}


int main(int n_args, char *ll_args[]){

	/* Variables locals */
	int f_h, c_h, i;
	char rh,rv,rd;

	/* Variables passades per valor */
	num_pil = atoi(ll_args[1])-1;
	
	vel_f = atof (ll_args[2]);
	vel_c = atof (ll_args[3]);
	int f_pil = atoi (ll_args[4]);
	int c_pil = atoi (ll_args[5]);
	float pos_f = atof (ll_args[6]);
	float pos_c = atof (ll_args[7]);
	int n_col = atoi (ll_args[8]);
	int n_fil = atoi (ll_args[9]);
	int retard = atoi (ll_args[10]);	/* Variables passades per parámetre */

	int rebotes = atoi (ll_args[11]);
	int *num_rebots = map_mem(rebotes);		/* obtenir adres. de mem. compartida */

	int fin2 = atoi (ll_args[12]);
	int *fi2 = map_mem(fin2);

	int id_win = atoi (ll_args[13]);
	void *p_win;
	p_win = map_mem (id_win);

	win_set(p_win,n_fil,n_col);	/* crea acces a finestra oberta pel proces pare */
	int sem_rebots = atoi(ll_args[14]);	    /* recupera id de semafor */
	int sem_win = atoi(ll_args[15]);	    /* recupera id de semafor */

	int num_pilotes = atoi (ll_args[16]);
	pthread_mutex_init(&mutex, NULL);		/* inicialitza el semafor */

	for (i = 0; i < num_pilotes; i++)
	{
		id_mis[i] = atoi(ll_args[i + 17]);	// Carreguem els ids de les busties en un array de caracters
	}
	
	//Llancem thread bustia
	pthread_create(&tid,NULL,escolta_bustia,(void *) num_pil);
	

/*	 INTENT PER A PASSAR UNA MATRIU...
	char Aid_mis[MAX_PILOTES][10]; // recipient del punter
	memcpy(Aid_mis, ll_args[17], sizeof(Aid_mis));

	int i, id_mis[num_pilotes];
	for ( i = 0; i<num_pilotes ; i++ )
	{
		fprintf(stderr, "\n%i second", id_mis[i]);
		id_mis[i] = atoi (ll_args[17]);
	}

	for (i = 0; i < num_pilotes; i++)
	{
		id_mis[i] = atoi (ll_args[17]);
		fprintf(stderr, "%i second", id_mis[i]);
	}

	
	 INSTRUCCIONS PER AL SEMAFOR 
	waitS(id_sem);		 tanca semafor 
	signalS(id_sem); 		obre semafor */

	do
	{
		pthread_mutex_lock(&mutex);		/* tanca semafor */
		f_h = pos_f+vel_f;		/* posicio hipotetica de la pilota (entera) */
		c_h = pos_c+vel_c;
		pthread_mutex_unlock(&mutex); 		/* obre semafor */  
		rh = rv = rd = ' ';
		if ((f_h != f_pil) || (c_h != c_pil))
		{
		if (f_h != f_pil) 		/* provar rebot vertical */
		{
			waitS(sem_win);
		    rv = win_quincar(f_h,c_pil);	/* veure si hi ha algun obstacle */
			signalS(sem_win); 		
			if (rv != ' ')			/* si hi ha alguna cosa */
			{   
				pthread_mutex_lock(&mutex);
				vel_f = -vel_f;		/* canvia sentit velocitat vertical */
				f_h = pos_f+vel_f;		/* actualitza posicio hipotetica */
				pthread_mutex_unlock(&mutex);
				if (rv == '0')
				{
					waitS(sem_rebots);
					(*num_rebots)--;
					signalS(sem_rebots); 
		            	}else if (rv != '+'){
					int pilota_rebot = (int)'1'+rv;
					fprintf(stderr,"rebot v : %i; pilota actual: %i;\n", pilota_rebot, num_pil);
					if( pilota_rebot != -1 && pilota_rebot != num_pil){
						pthread_mutex_lock(&mutex);					
						sprintf(missatge_enviar, "%i;%f;%f;v;s", num_pil, vel_f, vel_c);
						pthread_mutex_unlock(&mutex); 
						sendM(id_mis[pilota_rebot-1],missatge_enviar,100);
					}
				}else{	
					//si és la vora
				}
			}
		}
		    if (c_h != c_pil) 		/* provar rebot horitzontal */
		    {
			waitS(sem_win);
		    rh = win_quincar(f_pil,c_h);	/* veure si hi ha algun obstacle */
			signalS(sem_win); 
		    if (rh != ' ')			/* si hi ha algun obstacle */
		    {
				pthread_mutex_lock(&mutex);
		        vel_c = -vel_c;		/* canvia sentit vel. horitzontal */
		        c_h = pos_c+vel_c;		/* actualitza posicio hipotetica */
				pthread_mutex_unlock(&mutex);
		        if (rh == '0')
		        {
				waitS(sem_rebots);
				(*num_rebots)--;
				signalS(sem_rebots);
		        }else if (rh != '+'){
					int pilota_rebot = (int)'1'+rh;
					fprintf(stderr,"rebot h : %i; pilota actual: %i;\n", pilota_rebot, num_pil);
					if( pilota_rebot != -1 && pilota_rebot != num_pil){
						pthread_mutex_lock(&mutex);
						sprintf(missatge_enviar, "%i;%f;%f;h;s", num_pil, vel_f, vel_c);
						pthread_mutex_unlock(&mutex); 
						sendM(id_mis[pilota_rebot-1],missatge_enviar,100);						
					}
				}else{	
					//si és la vora
				}
		    }
		}
		if ((f_h != f_pil) && (c_h != c_pil))	/* provar rebot diagonal */
		{
			waitS(sem_win);
		    rd = win_quincar(f_h,c_h);
			signalS(sem_win);
		    if (rd != ' ')				/* si hi ha obstacle */
		    {
				pthread_mutex_lock(&mutex);
		        vel_f = -vel_f; vel_c = -vel_c;	/* canvia sentit velocitats */
		        f_h = pos_f+vel_f;
		        c_h = pos_c+vel_c;		/* actualitza posicio entera */
				pthread_mutex_unlock(&mutex);
		        if (rd == '0')
		        {
				waitS(sem_rebots);
				(*num_rebots)--;
				signalS(sem_rebots);
		        }else if (rd != '+'){
					int pilota_rebot = (int)'1'+rd;
					fprintf(stderr,"rebot d : %i; pilota actual: %i;\n", pilota_rebot, num_pil);
					if( pilota_rebot != -1 && pilota_rebot != num_pil){
						pthread_mutex_lock(&mutex);		
						sprintf(missatge_enviar, "%i;%f;%f;d;s", num_pil, vel_f, vel_c);
						pthread_mutex_unlock(&mutex);
						sendM(id_mis[pilota_rebot-1],missatge_enviar,100);
					}
				}else{	
					//si és la vora
				}
		    }
		}
		waitS(sem_win);
		if (win_quincar(f_h,c_h) == ' ')	/* verificar posicio definitiva */
		{					/* si no hi ha obstacle */
		    win_escricar(f_pil,c_pil,' ',NO_INV);  	/* esborra pilota */
			pthread_mutex_lock(&mutex);		    
			pos_f += vel_f; pos_c += vel_c;
			pthread_mutex_unlock(&mutex);
		    f_pil = f_h; c_pil = c_h;		/* actualitza posicio actual */
		    if (c_pil != 0){	 		/* si ho surt del taulell, */
		        win_escricar(f_pil,c_pil, '1'+num_pil ,INVERS); /* imprimeix pilota */
		    }
		    else
		    {
		        (*fi2)=1;		/* final per sortir  */
		    }
		}
		signalS(sem_win);
	
	    }
    	else { 
			pthread_mutex_lock(&mutex);
			pos_f += vel_f; pos_c += vel_c; 
			pthread_mutex_unlock(&mutex);
		}
    	win_retard(retard);
	}while (!(*fi2));
			/* obre semafor */
	char sortida[1];
	sortida[0] = 'e';
	jocEnMarxa = 0;
	sendM(id_mis[num_pil],sortida, 1 );
	
	pthread_mutex_destroy(&mutex);		/* destrueix el semafor */
	return(0);			/* retorna sense errors d'execucio */
}


