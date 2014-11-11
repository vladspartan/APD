#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

/*
	Functie folosita pentru dezalocarea matricilor.
*/
void freeMatrix(int** mat, int rows)
{
	int i;
	for(i = 0; i < rows; i++)
		free(mat[i]);
	free(mat);
}

/*
	Functie folosita pentru scrierea unei matrici intr-un fisier.
*/
void printMat(FILE *f, int** mat, int rows)
{
	int i,j;
	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < rows; j++)
			fprintf(f,"%i ", mat[i][j]);
		fprintf(f,"\n");
	}
}

/*
	Functie folosita pentru citirea unei matrici dintr-un fisier.
	Intoarce ca rezultat adresa de inceput a matricei.
*/
int** readMatrix(FILE *f, int rows)
{
	int i, j;
	int **mat = (int **)malloc(rows * sizeof(int*));
	for(i = 0; i < rows; i++)
		mat[i] = (int*)malloc(rows * sizeof(int));

	for(i = 0; i < rows; i++)
		for(j = 0; j < rows; j++)
			fscanf(f,"%i", &mat[i][j]);


	return mat;
}

/* Functie care intoarce o copie a matricii date ca parametru. */
int** copyMatrix(int** orig, int rows)
{
	int i, j;

	int **mat = (int **)malloc(rows * sizeof(int*));
	for(i = 0; i < rows; i++)
		mat[i] = (int*)malloc(rows * sizeof(int));

	for(i = 0; i < rows; i++)
		for(j = 0; j < rows; j++)
			mat[i][j] = orig[i][j];

	return mat;
}

/* Functie care copiaza matB peste matA. */
void copyAtoB(int** matA, int** matB, int rows)
{
	int i, j;
	for(i = 0; i<rows; i++)
		for(j = 0; j < rows; j++)
	    	matB[i][j] = matA[i][j];
}

/*
	Functia main.
*/
int main( int argc, char *argv[] )
{
	/* Matrice saptamana curenta, matrice saptamana viitoare.*/
	int **a, **b;
	/* Vector distante culori, vector contorizare a culorilor senatorilor pentru saptamana curenta.*/
	int c[100], configuration[100];
	/* Dimensiune matrice patratica, numar culori existente la inceput.*/
	int N, num_colours;

	/* Variabile folosite pentru parcurgerea matricilor. */
	int i, j, k, l, p;
	/* Numar saptamani.*/
	int nr_weeks = atoi(argv[1]);

	/* Variabile auxiliare. */
	int aux, maxX, maxY;
	/* Culoare aflata la dmax, dmax(vezi readme). */
	int max_loc, max_val;

	/* Fisier de intrare, fisier de iesire. */
	FILE *f_read = fopen(argv[2], "r");
	FILE *f_write = fopen(argv[3], "w");

	fscanf(f_read,"%i", &N);
	fscanf(f_read,"%i", &num_colours);
	/* Initializam cele 2 matrici. */
	a = readMatrix(f_read, N);
	b = copyMatrix(a, N);

	/* Initializam vector distante culori. */
	for(k = 0; k < num_colours; k++)
		c[k] = N + 1;

	/* Parcurgere saptamani. */
	for(p = 0; p < nr_weeks; p++)
	{
		memset(configuration, 0, num_colours * sizeof(int));

		/* Parcurgem unul cate unul toti senatorii. */
		for(i = 0; i < N; i++)
			for(j = 0; j < N; j++)
			{
				
				/* Parcurgem toti senatorii determinand pentru fiecare culoare distanta minima
				fata de senatorul curent(a[i][j]). */
				for(k = 0; k < N; k++)
					for(l = 0; l < N; l++)
					{

						/* Determinam distanta la care se afla senatorul k,l fata de i,j*/
						maxX = (i - k) > 0 ? (i - k) : (k - i);
						maxY = (j - l) > 0 ? (j - l) : (l - j);
						aux = maxX > maxY ? maxX : maxY;
						
						/* Daca am gasit o culoare la o distanta mai mica decat ce aveam
						pana atunci, actualizam. */
						if(aux < c[a[k][l]] && !(k == i && l == j))
							c[a[k][l]] = aux;
					}

				
				max_val = 0;
				/* Parcurgem vectorul de distante culori pentru a determina dmax(vez readme) 
				si culoarea aflata la dmax.*/
				for(k = 0; k < num_colours; k++)
				{

					if(c[k] > max_val && c[k] < N + 1)
					{
						max_loc = k;
						max_val = c[k];
					}
					/* Dupa parcurgere reinitializam vectorul de distante culori. */
					c[k] = N + 1;
				}
				/* Actualizam vectorul de contorizare a culorilor senatorilor*/
				configuration[max_loc]++;
				/* Actualizare matrice saptamana viitoare. */
				b[i][j] = max_loc;
			}

		/* Copiem matricea saptamanii viitoare peste matricea curenta. */
		for(i = 0; i < N; i++)
			for(j = 0; j < N; j++)
	    		a[i][j] = b[i][j];

	    /* Scriem in fisier vectorul de contorizare a culorilor senatorilor*/
		for(i = 0; i < num_colours; i++)
		{
			fprintf(f_write,"%i ", configuration[i]);
		}
		fprintf(f_write,"\n");
	}
	/* Scriem in fisier matricea finala. */
	printMat(f_write,b,N);

	/* Inchidem fisiere si dezalocam structuri. */
	fclose(f_read);
	fclose(f_write);
	freeMatrix(a, N);
	freeMatrix(b, N);
	
	return 0;
}


