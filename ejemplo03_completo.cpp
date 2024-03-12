
#include <iostream>
#include <mpi.h>
#include <vector>
#include <cmath>

#define MAX_ITEMS 25

int sumar(int *tmp, int n) {
    int suma = 0;
    for (int i = 0; i < n; i++) {
        suma += tmp[i];
    }

    return suma;
}


//comunicacion sincrona
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;

    int n;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int block_size;
    int real_size;
    int padding;


    if (MAX_ITEMS % nprocs != 0) {
        real_size = std::ceil((double) MAX_ITEMS / nprocs) * nprocs;
        block_size = real_size / nprocs;
        padding = real_size - MAX_ITEMS;
    }

    std::vector<int> data;
    std::vector<int> data_local(block_size);

    if (rank == 0) {
        data.resize(real_size);

        std::printf("Dimension: %d, real_size: %d, block_size: %d, padding: %d\n",
                    MAX_ITEMS, real_size, block_size, padding);

        for (int i = 0; i < MAX_ITEMS; i++) {
            data[i] = i;
        }
    }

    //enviar los datos

    MPI_Scatter(data.data(), block_size, MPI_INT, data_local.data(), block_size, MPI_INT, 0, MPI_COMM_WORLD);

    //CALCULÑAR CADA UNO LA SUMA PÁRCIAL

    if (rank == nprocs - 1) {
        block_size = block_size - padding;
    }

    int suma_parcial = sumar(data_local.data(), block_size);


    std::printf("RANK_%d: suma parcial =%d\n", rank, suma_parcial);

    //enviar la suma parcial al rank '0
    int suma_total = 0;
    MPI_Reduce(&suma_parcial //buffer que se enviara
               , &suma_total //buffer en donde se recibira
               , 1 //cantidad
               , MPI_INT //tipo
               , MPI_SUM //operacion
               ,0 //root
                , MPI_COMM_WORLD);

    //imprimir el resultado

    if(rank== 0 ) {
        std::printf("SUMA TOTAL: %d\n", suma_total);
    }

    MPI_Finalize();
    return 0;
}