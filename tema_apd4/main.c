#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
NICULA BOGDAN 334CB
Tema 4 apd
*/


#define MAX_SIZE        100
#define PAYLOAD_SIZE    1500
#define BUFFER_SIZE     1520

/* tipuri de mesaj */
#define SONDAJ      1
#define ECOU        2
#define CONV        3
#define CONV_END    4
#define LEAD_WP     5
#define LEAD_ID     6
#define LEAD_OK     7     


#define BCAST   -1


/* Structura candidat pentru determinarea lider-ului */
typedef struct {

    int id;
    int num_neighbours;
} candidate;


/* Structura mesaj pentru comunicare intre procese */
typedef struct {

    char payload[PAYLOAD_SIZE];
    int type;
    int source_id;
    int dest_id;

} msg;


/* Functie care compara 2 candidati diferiti */
int compare_candidate(candidate a, candidate b)
{
    if(a.num_neighbours > b.num_neighbours)
        return 1;
    if(a.num_neighbours < b.num_neighbours)
        return -1;

    return (a.id < b.id) ? 1 : -1;
}

/* Functie care calculeaza nr de cifre al unui integer. */
int num_digits(int n)
{
    if(n < 10)
        return 1;

    return 1 + num_digits(n / 10);
}


int main(argc,argv) 
int argc;
char *argv[];  {
int root_link, numtasks, i, j, k, p, rank, dest, source, rc, count, tag=1;  
char a[MAX_SIZE][MAX_SIZE] = {0};
int routing_table[MAX_SIZE], num_neighbours = 0, num_messages = 0, msg_list_size = 0;
FILE *in_top, *in_mes;
char buffer[BUFFER_SIZE], *aux_buffer, *aux;

msg out_message, in_message;
msg *msg_list = NULL;

/* Candidati pentru leader(1) si pentru adjunct(2) interni si externi. */
candidate in_cand_1, in_cand_2, out_cand_1, out_cand_2;

MPI_Status Stat;

MPI_Init(&argc,&argv);
MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);


/* Verificare nr argumente */
if(argc != 3)
{
  if(rank == 0)
  printf("Numar gresit de parametri.\nFormatul corect este: %s fisier_topologie fisier_mesaje\n", argv[0]);

  MPI_Finalize();
  return -1;
}

/* Citirea listei de vecini */
in_top = fopen(argv[1], "r");

/* Eroare deschidere fisier */
if(in_top == NULL)
{
    printf("Fisierul de topologie nu poate fi deschis!\n");
    MPI_Finalize();
    return -1;
}

/* Salt la linia dorita din fisierul cu topologia. */
while(1)
{
    fgets(buffer, 200, in_top);

    aux = strtok(buffer, " -");
    sscanf(aux,"%d",&k);/* k = id nod */
    
    /* Ne oprim la linia dorita */
    if(k == rank)
        break;
}

/* Citire id vecini */
aux = strtok(NULL, " -");
while(aux != NULL)
{
    if(sscanf(aux,"%d",&j) != 1)
        break;
    a[k][j] = 1;
    num_neighbours++;
    aux = strtok(NULL, " -");
}

fclose(in_top);

/*---------------------------------------------------*/
/* TASK 1
     Stabilirea topologiei */
/*---------------------------------------------------*/

MPI_Barrier(MPI_COMM_WORLD);
fflush(stdout);
if(rank == 0)
    printf("\n\n--------------------\nBeginning of Stage 1\n--------------------\n");
MPI_Barrier(MPI_COMM_WORLD);/* Astept ca toate procesele sa fi incheiat etapa 1 */


if (rank == 0) {  
    out_message.type = SONDAJ;
    memcpy(buffer, &out_message, sizeof(msg));
    
    /* Trimit mesaj 'SONDAJ' tuturor vecinilor */
    for(i = 1; i < numtasks; i++)
        if(a[rank][i])
        {
            dest = i;
            rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD);       
        }  
    
    /* Initializez tabela de rutare */
    for(i = 0; i < numtasks; i++)
        routing_table[i] = 0;

    /* Astept 'ECOU' de la toti vecinii */
    k = 0;
    p = 0;
    while(k < num_neighbours)
    {       
        rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat); 
        
        memcpy(&in_message, buffer, sizeof(msg));
        
        /* Pentru mesaj 'SONDAJ' */
        if(in_message.type == SONDAJ)
        {
            /* Trimit raspuns cu o topologie goala. */
            memset(out_message.payload, 0, PAYLOAD_SIZE);
            out_message.type = ECOU;
            memcpy(buffer, &out_message, sizeof(msg));

            dest = Stat.MPI_SOURCE;
            rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
            p--;
            continue;
        }


        /* Pentru mesaj 'ECOU' */
        /* Actualizez topologia */
        for(i = 0; i < numtasks; i++)
            for(j = 0; j < numtasks; j++)
            {
                if(j != i && in_message.payload[i * numtasks + j])
                    a[i][j] = 1;
            }

        /* Actualizez tabela de rutare (stocata pe diagonala matricei de topologie)
            vezi README
        */
        for(j = 0; j < numtasks; j++)
            if(in_message.payload[j * numtasks + j] != rank)
                routing_table[j] = Stat.MPI_SOURCE;
        k++;
    }
    num_neighbours += p;// Elimin vecinii care mi-au trimis sondaj si nu sunt parinti
} 


if(rank != 0)
{
    /* Astept un prim mesaj 'SONDAJ' de la un nod parinte */
    rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat);
    root_link = Stat.MPI_SOURCE;
    memcpy(&in_message, buffer, sizeof(msg));

    out_message.type = SONDAJ;    
    memcpy(buffer, &out_message, sizeof(msg));

    /* Trimit mai apoi 'SONDAJ' celorlalti vecini */
    for(i = 0; i < numtasks; i++)
        if(i != root_link &&(a[i][rank] || a[rank][i]))
        {
            dest = i;
            rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
        }
    
    /* Pregatesc tabela de rutare */
    for(i = 0; i < numtasks; i++)
            routing_table[i] = root_link;
    routing_table[rank] = rank;

    
    /* Astept sa primesc cate un mesaj 'ECOU' de la fiecare proces 'copil' */
    k = 0;
    p = 0;
    while(k < num_neighbours - 1)   
    {
        rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat); 
        memcpy(&in_message, buffer, sizeof(msg));

        /* Pe baza mesajelor 'ECOU' actualizez topologia si tabelul de rutare */
        if(in_message.type == ECOU)
        {
            for(i = 0; i < numtasks; i++)
                for(j = 0; j < numtasks; j++)
                {
                    if(i != j && in_message.payload[i * numtasks + j])
                        a[i][j] = 1;     
                }

            for(j = 0; j < numtasks; j++)
                if(in_message.payload[j * numtasks + j] != rank && in_message.payload[j * numtasks + j])
                    routing_table[j] = Stat.MPI_SOURCE;

            k++;
        }

        /* Daca mai primesc mesaje 'SONDAJ' intorc o topologie nula */
        else if(in_message.type == SONDAJ)
        {
            memset(out_message.payload, 0, PAYLOAD_SIZE);
            out_message.type = ECOU;
            memcpy(buffer, &out_message, sizeof(msg));

            dest = Stat.MPI_SOURCE;
            rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
            p--;
        }
    }
    num_neighbours += p;//Scot din calcul vecinii care mi-au trimis sondaj si nu sunt parinti


    /* Copiez topologia si tabela de rutare in campul payload al mesajului */
    for(i = 0; i < numtasks; i++)
        for(j = 0; j < numtasks; j++)
            out_message.payload[i * numtasks + j] = a[i][j];

    for(i = 0; i < numtasks; i++)
        out_message.payload[i * numtasks + i] = routing_table[i];

    /* Raspund cu mesaj 'ECOU' nodului parinte */
    out_message.type = ECOU;
    memcpy(buffer, &out_message, sizeof(msg));
    rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, root_link, tag, MPI_COMM_WORLD);     
}


/* Afisez topologia rezultata */
if(rank == 0)
{
    printf("{%i}Topologie: \n", rank);
    for(j = 0; j < numtasks * numtasks; j++)
        if(j % numtasks == 0)
            printf("\n%i ",a[j/numtasks][j%numtasks]);
        else
            printf("%i ",a[j/numtasks][j%numtasks]);
    printf("\n\n");
}

/* Astept ca toate procesele sa isi incheie activitatea */
MPI_Barrier(MPI_COMM_WORLD);

/* Afisez pe rand topologia pentru fiecare proces */
for(i = 0; i < numtasks; i++)
{
    fflush(stdout);
    if(rank == i)
    {
        printf("\n%i\n", rank);
        for(j = 0; j < numtasks; j++)
            printf("%i %i\n", j, routing_table[j]);   
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

/* Final prima parte */
MPI_Barrier(MPI_COMM_WORLD);
fflush(stdout);
if(rank == 0)
    printf("\n\n--------------------\nBeginning of Stage 2\n--------------------\n");
MPI_Barrier(MPI_COMM_WORLD);/* Astept ca toate procesele sa fi incheiat etapa 1 */




/*---------------------------------------------------*/
/* TASK 2
     Sistem de transmitere a mesajelor */
/*---------------------------------------------------*/


/* Citesc mesajele din fisier si salvez mesajele proprii intr-un vector */
in_mes = fopen(argv[2], "r");

if(in_mes == NULL)
{
    printf("Fisierul de topologie nu poate fi deschis!\n");
    MPI_Finalize();
    return -1;
}

memset(buffer, 0, BUFFER_SIZE);
fgets(buffer, BUFFER_SIZE, in_mes);
sscanf(buffer, " %i", &num_messages);

/* Citesc pe rand toate liniile fisierului */
i = 0;
while(i < num_messages)
{
    int source, dest, offset = 0;
    fgets(buffer, BUFFER_SIZE, in_mes);
    aux_buffer = strdup(buffer);
    aux = strtok(aux_buffer, " ");
    sscanf(aux, "%i", &source);

    offset += num_digits(source) + 1;

    /* Daca mesajul apartine procesului curent(va fi trimis de nodul curent).*/
    if(source == rank)
    {
        aux = strtok(NULL, " ");
        if(aux[0] == 'B')
        {
            dest = BCAST;
            offset += 2;
        }
        else
        {
            sscanf(aux, "%i", &dest);
            offset += num_digits(dest);
        }

        /* Il adaug la coada de mesaje a procesului */
        if(msg_list == NULL)
        {
            msg_list_size++;
            msg_list = (msg *)malloc(sizeof(msg));
        }
        else
        {
            msg_list_size++;
            msg_list = (msg *)realloc(msg_list, msg_list_size * sizeof(msg));
        }

        msg_list[msg_list_size - 1].dest_id = dest;
        msg_list[msg_list_size - 1].source_id = source;
        msg_list[msg_list_size - 1].type = CONV;
        strcpy(msg_list[msg_list_size - 1].payload, buffer + offset);       
    }
    i++;
}
fclose(in_mes);

/* Sfarsit citire mesaje */


/* Incep transmisia */
out_message.dest_id = BCAST;
out_message.source_id = rank;
out_message.type = CONV;

/* Broadcast initial */
memcpy(buffer, &out_message, sizeof(msg));
printf("{%i}BROADCAST initial!\n", rank);
for(j = 0; j < numtasks; j++)
    if(routing_table[j] == j && j != rank)
        rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 

i = 0;
k = numtasks - 1;
while(k)
{
    rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat); 
    memcpy(&in_message, buffer, sizeof(msg));
    
    
    if(in_message.type == CONV)
    {
        printf("{%i}Primit BROADCAST initial de la %i\n", rank, in_message.source_id);
        for(j = 0; j < numtasks; j++)
            if(routing_table[j] == j && j != rank && j != Stat.MPI_SOURCE)
            {
                rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
            }
        k--;
    }
}

MPI_Barrier(MPI_COMM_WORLD);
/* Sfarsit broadcast initial */

/* Incep transmiterea mesajelor proprii */
for(i = 0; i < msg_list_size; i++)
{
    memcpy(buffer, &msg_list[i], sizeof(msg));

    /* Transmit mesaj cu destinatar*/
    if(msg_list[i].dest_id != BCAST)
    {
        rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, routing_table[msg_list[i].dest_id], tag, MPI_COMM_WORLD);
        printf("{%i}[%i->%i](nh %i): %s", rank, rank, msg_list[i].dest_id, 
                routing_table[msg_list[i].dest_id], msg_list[i].payload);
    }
    /* Trasmit mesaj broadcast */
    else
    {
        for(j = 0; j < numtasks; j++)
            if(routing_table[j] == j && j != rank)
            {
                rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
                printf("{%i}[B]: %s", rank, msg_list[i].payload);
            }
    }

}
/* Mesajele au fost trimise de surse */

free(msg_list);


/* Trimit mesaj de 'CONV_END'(Sfarsit etapa conversatie) */
out_message.dest_id = BCAST;
out_message.source_id = rank;
out_message.type = CONV_END;

memcpy(buffer, &out_message, sizeof(msg));
for(j = 0; j < numtasks; j++)
    if(routing_table[j] == j && j != rank)
    {
        rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
    }


/* Primire si rutare mesaje */
k = numtasks - 1;/*Nr mesaje confirmare sfarsit conversatie neprimite(asteptate)*/
while(k)
{
    rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat); 
    memcpy(&in_message, buffer, sizeof(msg));

    if(in_message.type == CONV)
    {
        /* Mesaj sosit la destinatie */
        if(in_message.dest_id == rank)
        {
            printf("{%i}[%i->%i]: %s", rank, in_message.source_id, rank, in_message.payload);
        }

        /* Rutare mesaj broadcast */
        else if(in_message.dest_id == BCAST)
        {
            printf("{%i}[B](from %i): %s", rank, in_message.source_id, in_message.payload);
            for(j = 0; j < numtasks; j++)
                if(routing_table[j] == j && j != rank && j != Stat.MPI_SOURCE)
                {
                    rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
                }
        }
        /* Rutare mesaj simplu */
        else 
        {
            printf("{%i}[%i->%i)(nh %i): %s", rank, in_message.source_id, in_message.dest_id, routing_table[in_message.dest_id], in_message.payload);
            rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, routing_table[in_message.dest_id], tag, MPI_COMM_WORLD); 
        }

    }
    /* La mesajele 'CONV_END' se face implicit broadcast */
    else if(in_message.type == CONV_END)
    {
        k--;
        for(j = 0; j < numtasks; j++)
            if(routing_table[j] == j && j != rank && j != Stat.MPI_SOURCE)
            {
                rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
            }
    }
}

/* Astept ca toate procesele sa incheie aceasta etapa. */
MPI_Barrier(MPI_COMM_WORLD);
fflush(stdout);
if(rank == 0)
    printf("\n--------------------\nBeginning of Stage 3\n--------------------\n");
MPI_Barrier(MPI_COMM_WORLD);
/* Sfarsitul etapei 2 */



/* Transmit mesaje de wake-up */
out_message.dest_id = BCAST;
out_message.source_id = rank;
out_message.type = LEAD_WP;

memcpy(buffer, &out_message, sizeof(msg));


for(j = 0; j < numtasks; j++)
    if(routing_table[j] == j && j != rank)
    {
        rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
    }

/* Astept mesaje de wake-up de la toate procesele */
k = numtasks - 1;
while(k)
{
    rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat); 
    memcpy(&in_message, buffer, sizeof(msg));

    if(in_message.type == LEAD_WP && in_message.dest_id == BCAST)
    {
        for(j = 0; j < numtasks; j++)
            if(routing_table[j] == j && j != rank && j != Stat.MPI_SOURCE)
            {
                rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
            }
        k--;
    }
}

/* Incepere algoritm de detectie a liderului */

/* Initializez variabilele pentru lider si adjunct */
out_cand_1.id = rank;
out_cand_1.num_neighbours = num_neighbours;
out_cand_2.id = rank;
out_cand_2.num_neighbours = num_neighbours;


/* Astept mesaje de la toate nodurile 'copil' */
k = (rank == 0) ? num_neighbours : num_neighbours - 1;
while(k)
{
    rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat); 
    memcpy(&in_message, buffer, sizeof(msg));   
    memcpy(&in_cand_1, in_message.payload, sizeof(candidate));   
    memcpy(&in_cand_2, in_message.payload + sizeof(candidate), sizeof(candidate));

    /* Compar informatia din mesaj cu informatia proprie si actualizez
    (daca este necesar) liderul si adjunctul */
    if(in_message.type = LEAD_ID)
    {
        if(compare_candidate(out_cand_1, in_cand_1) > 0)
        {
            if(compare_candidate(out_cand_2, in_cand_1) < 0 || out_cand_2.id == out_cand_1.id)
            {
                out_cand_2.id = in_cand_1.id;
                out_cand_2.num_neighbours = in_cand_1.num_neighbours;
            }
        }
        else
        {
            if(compare_candidate(out_cand_1, in_cand_2) > 0)
            {
                out_cand_2.id = out_cand_1.id;
                out_cand_2.num_neighbours = out_cand_1.num_neighbours;
            }
            else
            {
                out_cand_2.id = in_cand_2.id;
                out_cand_2.num_neighbours = in_cand_2.num_neighbours;
            }

            out_cand_1.id = in_cand_1.id;
            out_cand_1.num_neighbours = in_cand_1.num_neighbours;   
        }

        k--;
        
    }
}

/* Trimit valorile finale obtinute(pentru lider si adjunct) nodului parinte */
if(rank != 0)
{
    out_message.type == LEAD_ID;
    memcpy(out_message.payload, &out_cand_1, sizeof(candidate));   
    memcpy(out_message.payload + sizeof(candidate), &out_cand_2, sizeof(candidate));
    memcpy(buffer, &out_message, sizeof(msg));
    rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, root_link, tag, MPI_COMM_WORLD); 
}

/* O data ce radacina a decis asupra identitatilor liderului si adjunctului
notifica toate nodurile din topologie cu privire la rezultat */
if(rank == 0)
{
    out_message.type == LEAD_OK;
    out_message.dest_id = BCAST;
    memcpy(out_message.payload, &out_cand_1, sizeof(candidate));   
    memcpy(out_message.payload + sizeof(candidate), &out_cand_2, sizeof(candidate));
    memcpy(buffer, &out_message, sizeof(msg));

    for(j = 0; j < numtasks; j++)
        if(routing_table[j] == j && j != rank)
        {
            rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
        }
}

if(rank != 0)
{
    while(1)
    {
        rc = MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat);

        memcpy(&in_message, buffer, sizeof(msg));   
        
        /* Daca am primit mesaj cu privire la identitate lider('LEAD_OK')
        actualizez datele proprii si trimit mesajul mai departe 'copiilor'
        */   
        if(in_message.type = LEAD_OK && in_message.dest_id == BCAST)
        {
            
            memcpy(&out_cand_1, in_message.payload, sizeof(candidate));   
            memcpy(&out_cand_2, in_message.payload + sizeof(candidate), sizeof(candidate));

            for(j = 0; j < numtasks; j++)
                if(routing_table[j] == j && j != rank && Stat.MPI_SOURCE != j)
                {
                    rc = MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, j, tag, MPI_COMM_WORLD); 
                }
            break;
        }
    }
}

if(rank == 0)
    printf("\nNod  Leader  Adjunct:\n");
/* Astept ca toate nodurile sa fi fost notificate */
MPI_Barrier(MPI_COMM_WORLD);
fflush(stdout);

/* Afisez lider si adjunct din perspectiva fiecaru nod */
for(i = 0; i < numtasks; i++)
{
    if(rank == i)
        printf("%i\t%i\t%i\n", rank, out_cand_1.id, out_cand_2.id);
    MPI_Barrier(MPI_COMM_WORLD);
}

if(rank == 0)
    printf("\n----------------\nThe End\n----------------\n");



MPI_Finalize();
}
