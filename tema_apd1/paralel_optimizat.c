#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <omp.h>

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

/*
	Functia main.
*/
int main( int argc, char *argv[] )
{
	/* Matricea curenta si matricea saptamanii viitoare(vezi readme).*/
	int **a, **b;
	/* Vector de contorizare a culorilor senatorilor pentru saptamana curenta.
	(la indicele i vom calcula numarul de senatori de culoare Ci in saptamana curenta. */
	int configuration[100];
	
	/* Vector de flag-uri care indica daca o culoare a fost descoperita deja(vezi readme)*/
	char colourFlag[100];
	/* Dimensiune matrice patratica, numar culori existente la inceput.*/
	int N, num_colours;
	

	/* Variabile folosite pentru parcurgerea matricilor. */
	int i, j, k, l, p;
	/* Numar saptamani.*/
	int nr_weeks = atoi(argv[1]);
	
	/* Numar culori posibile(existente) in saptamana curenta, numar culori 'descoperite'. */
	int available_col, found_col;
	/* Dimensiune latura pt patratele concentrice*/
	int radius;

	int minY = 0, maxY = 0, minX = 0, maxX = 0;
	/*variabila auxiliara, dmax(vezi readme) si culoare aflata la distanta dmax*/
	int aux, max_loc, max_val;

	/* Fisier de intrare, fisier de iesire. */
	FILE *f_read = fopen(argv[2], "r");
	FILE *f_write = fopen(argv[3], "w");

	fscanf(f_read,"%i", &N);
	fscanf(f_read,"%i", &num_colours);

	/* initializam cele 2 matrici. */
	a = readMatrix(f_read, N);
	fclose(f_read);
	b = (int **)malloc(N * sizeof(int*));
	for(i = 0; i < N; i++)
		b[i] = (int*)malloc(N * sizeof(int));

	
	/* Initial toate culorile se pot gasi in matrice. */
	available_col = num_colours;

	/* Parcurgere saptamani. */
	for(p = 0; p < nr_weeks; p++)
	{
		/* Initializam cu 0 vectorul de contorizare a culorilor politice. */
		memset(configuration, 0, num_colours * sizeof(int));

		/* Pentru fiecare senator in parte vom determina culoarea pe care acesta o va avea in
		urmatoarea saptamana. Aici paralelizam.(deoarece transformarile senatorului x 
		nu afecteaza(in aceasta saptamana) transformarile senatorului y) */
		#pragma omp parallel for \
		shared(configuration, available_col, a, b, N) \
		private(i, j, radius, found_col, max_val, max_loc, l, k, colourFlag, aux, minX, minY, maxX, maxY)
		for(i = 0; i < N; i++)
			for(j = 0; j < N; j++)
			{
				/* Incepem cu un patrat concentric de raza 1.*/
				radius = 1;

				memset(colourFlag, 0, 100 * sizeof(char));
				
				/* Initializam numar culori gasite, dmax */
				found_col = 0;
				max_val = 0;
				max_loc = N;
				while(radius < N)
				{
					/* Determinam coordonatele colturilor stanga-jos, 
						respectiv dreapta-sus al patratelor concentrice */
					aux = i - radius;
					minY = 0 > aux ? 0 : aux;
					aux = i + radius + 1;
					maxY = N < aux ? N : aux;
					aux = j - radius + 1 ;
					minX = 0 > aux ? 0 : aux;
					aux = j + radius;
					maxX = N < aux ? N : aux;

					/* Parcurgem toti senatorii aflati la distanta +radius pe Ox de senatorul curent. */
					l = j + radius;
					if(l < N)
					{
						for(k = minY;k < maxY; k++)
						{
							/* Daca am descoperit un senator de culore neintalnita inca. */
							if(colourFlag[a[k][l]] == 0)
							{
								/* Setam culoarea ca fiind descoperita. */
								colourFlag[a[k][l]] = 1;
								/*Incrementam numarul de culori descoperite. */
								found_col++;

								/* Actualizam dmax. */
								if(radius > max_val)
								{
									max_val = radius;
									max_loc = a[k][l];
								}
								else if(radius == max_val && max_loc > a[k][l])
									max_loc = a[k][l];

								/*Daca toate culorile au fost descoperite, ne oprim. */
								if(found_col == available_col)
								{
									radius = 2 * N;
									/* Setam valoarea lui k astfel incat la urmatoarea iteratie
									sa iesim fortat din bucla. */
									k = maxY;
								}
							}
						}
					}

					/* Parcurgem toti senatorii aflati la distanta -radius pe Ox de senatorul curent. */
					l = j - radius;
					if(l >= 0)
					{
						for(k = minY;k < maxY; k++)
						{
							if(colourFlag[a[k][l]] == 0)
							{
								/* Setam culoarea ca fiind descoperita. */
								colourFlag[a[k][l]] = 1;
								/*Incrementam numarul de culori descoperite. */
								found_col++;

								/* Actualizam dmax. */
								if(radius > max_val)
								{
									max_val = radius;
									max_loc = a[k][l];
								}
								else if(radius == max_val && max_loc > a[k][l])
									max_loc = a[k][l];

								if(found_col == available_col)
								{
									radius = 2 * N;
									k = maxY;
								}
							}
						}
					}
	
					/* Parcurgem toti senatorii aflati la distanta +radius pe Oy de senatorul curent. */
					l = i + radius;
					if(l < N)
					{
						for(k = minX;k < maxX; k++)
						{
							
							if(colourFlag[a[l][k]] == 0)
							{
								/* Setam culoarea ca fiind descoperita. */
								colourFlag[a[l][k]] = 1;
								/*Incrementam numarul de culori descoperite. */
								found_col++;

								/* Actualizam dmax. */
								if(radius > max_val)
								{
									max_val = radius;
									max_loc = a[l][k];
								}
								else if(radius == max_val && max_loc > a[l][k])
									max_loc = a[l][k];

								if(found_col == available_col)
								{
									radius = 2 * N;
									k = maxX;
								}
							}
						}
					}

					/* Parcurgem toti senatorii aflati la distanta -radius pe Oy de senatorul curent. */
					l = i - radius;
					if(l >= 0)
					{
						for(k = minX;k < maxX; k++)
						{
							
							if(colourFlag[a[l][k]] == 0)
							{
								/* Setam culoarea ca fiind descoperita. */
								colourFlag[a[l][k]] = 1;
								/*Incrementam numarul de culori descoperite. */
								found_col++;

								/* Actualizam dmax. */
								if(radius > max_val)
								{
									max_val = radius;
									max_loc = a[l][k];
								}
								else if(radius == max_val && max_loc > a[l][k])
									max_loc = a[l][k];

								if(found_col == available_col)
								{
									radius = 2 * N;
									k = maxX;
								}
							}
						}
					}

					/* Incrementam raza. */
					radius++;
				}
				/* Variabila max_loc va contine culoarea aflata la dmax.*/
				
				
				/* Actualizam vectorul de contorizare a culorilor senatorilor.
				Avem grija ca aceasta operatie sa nu fie realizata simultan de 2 thread-uri.*/
				#pragma omp critical
				{
					configuration[max_loc]++;
				}
				/* Actualizam matricea saptamanii viitoare. */
				b[i][j] = max_loc;
			}
		/* Sfarsit paralelizare. */


		/* Copiem matricea saptamanii viitoare peste matricea de lucru*/
		for(i = 0; i < N; i++)
			for(j = 0; j < N; j++)
	    		a[i][j] = b[i][j];
		
	    /* Actualizam numar culori existente, scriem in fisier vectorul de contorizare
	    a culorilor senatorilor. */
	    available_col = num_colours;
		for(i = 0; i < num_colours; i++)
		{
			if(configuration[i] == 0)
				available_col--;
				
			fprintf(f_write,"%i ", configuration[i]);
		}
		fprintf(f_write,"\n");
		
	
	}
	/* Afisam matricea in forma finala.*/
	printMat(f_write,b,N);


	/* Inchidem fisiere si eliberam structuri alocate. */
	fclose(f_write);
	freeMatrix(a, N);
	freeMatrix(b, N);


	return 0;
}

