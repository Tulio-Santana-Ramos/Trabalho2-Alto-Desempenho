#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_DIST 25
#define ROOT 0
#define min(a,b) (a) > (b) ? (b) : (a);

void copia_arr(int* dest, int* src, int n)
{
	for (int i = 0; i < n; i++)
	{
		dest[i] = src[i];
	}
}

void funcao_geradora(int** graph, int n)
{
	srand(12345);
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			if (i != j)
				graph[i][j] = rand() % MAX_DIST + 1;
			else
			{
				graph[i][j] = INT_MAX/2;
				//graph[i][j] /= 2;
			}
		}
	}
	return;
}

long caminho(int ** graph, int* visitados, int* melhor_caminho, const int n, const int vertice, const int posicao_caminho, const int posicao_inicial)
{
	if (posicao_caminho+1 == n)
	{
		return graph[vertice][posicao_inicial];
	}

	long dist = 0;
	long min_dist = LONG_MAX;
	int min_pos = -1;
	int* melhor_caminho_local = (int*)malloc(sizeof(int) * n);

	copia_arr(melhor_caminho_local, melhor_caminho, n);

	for (int i = 0; i < n; i++)
	{
		if (!visitados[i] && i != vertice)
		{
			visitados[i] = 1;
			
			long temp = caminho(graph, visitados, melhor_caminho_local, n, i, posicao_caminho + 1, posicao_inicial) + graph[vertice][i];
			if (temp < min_dist)
			{
				copia_arr(melhor_caminho, melhor_caminho_local, n);
				
				min_dist = temp;
				min_pos = i;
				melhor_caminho[posicao_caminho] = i;
			}

			visitados[i] = 0;
		}
	}

	if (min_pos == -1)
	{
		printf("Erro\n");
	}
	return min_dist;
}

int main(int argc, char const *argv[])
{
    int n = 11, rank, n_proc;
	MPI_Status status;
	srand(12345);
	
	int** graph = (int**)malloc(sizeof(int*) * n);
	for (int i = 0; i < n; i++)
	{
		graph[i] = (int*)malloc(sizeof(int) * n);
	}

	funcao_geradora(graph, n);

	int* visitados = (int*)malloc(sizeof(int) * n);
	int* melhor_caminho = (int*)malloc(sizeof(int) * n);
	long menor_caminho_dist = 0;

	for (int i = 0; i < n; i++)
	{
		visitados[i] = 0;
		melhor_caminho[i] = 0;
	}

	visitados[0] = 1;

	MPI_Init(&argc, (char***) &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &n_proc);
	
	int block_size = (n - 1)/n_proc;
	long local_menor_caminho_dist = LONG_MAX;
	int vertice_inicial = rank*block_size;
	int vertice_final = vertice_inicial + block_size;
	int min_pos;
	long temp_menor_dist = LONG_MAX;

	int* local_visitados = (int*)malloc(sizeof(int) * n);
	int* local_melhor_caminho_tmp = (int*)malloc(sizeof(int) * n);
	int* local_melhor_caminho = (int*)malloc(sizeof(int) * n);
	memset(local_visitados, 0, n*sizeof(int));
	memset(local_melhor_caminho_tmp, 0, n*sizeof(int));
	memset(local_melhor_caminho, 0, n*sizeof(int));

	local_visitados[0] = 1;

	if (rank == n_proc - 1) {
		vertice_final = n;
	}

	for (int i = vertice_inicial; i < vertice_final; i++) {
		local_visitados[i] = 1;
		local_melhor_caminho_tmp[0] = i;
		for (int vertice = 0; vertice < n; vertice++) {
			
			if (!local_visitados[vertice] && vertice != i) {
				local_visitados[vertice] = 1;
				temp_menor_dist = caminho(graph, local_visitados, local_melhor_caminho_tmp, n, vertice, 2, 0) + graph[i][vertice] + graph[0][i];
				if (temp_menor_dist < local_menor_caminho_dist)
				{
					copia_arr(local_melhor_caminho, local_melhor_caminho_tmp, n);
					
					local_menor_caminho_dist = temp_menor_dist;
					min_pos = vertice;
					local_melhor_caminho[1] = vertice;
				}
				local_visitados[vertice] = 0;
			}
		}
		local_visitados[i] = 0;
	}
	
	
	MPI_Reduce(&local_menor_caminho_dist, &menor_caminho_dist, 1, MPI_LONG, MPI_MIN, ROOT, MPI_COMM_WORLD);
	MPI_Bcast(&menor_caminho_dist, 1, MPI_LONG, ROOT, MPI_COMM_WORLD);

	if (menor_caminho_dist == local_menor_caminho_dist)
		MPI_Send(local_melhor_caminho, n, MPI_INT, ROOT, 0, MPI_COMM_WORLD);
	if (rank == ROOT)
		MPI_Recv(melhor_caminho, n, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);


	if (rank == ROOT) {
		printf("%ld\n", menor_caminho_dist);
		printf("0 -> %d : %d\n", melhor_caminho[0], graph[0][melhor_caminho[0]]);
		for (int i = 1; i < n; i++)
		{
			printf("%d -> %d : %d\n", melhor_caminho[i-1], melhor_caminho[i],graph[melhor_caminho[i-1]][melhor_caminho[i]]);
		}
	}


	MPI_Finalize();

	for (int i = 0; i < n; i++)
	{
		free(graph[i]);
	}
	free(graph);
    
    return 0;
}