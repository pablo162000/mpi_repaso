
#include <iostream>
#include <mpi.h>
#include <vector>
#define TAMANIO 10

int sumar (int *tmp, int n) {
    int suma = 0;
    for(int i=0;i<n;i++){
        suma += tmp[i];
    }

    return  suma;
}


//comunicacion sincrona
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;

    int bloq_size;

    int residuo;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);


    if(TAMANIO%nprocs==0){
        bloq_size=TAMANIO/nprocs;
        residuo =  TAMANIO%nprocs;
    }else {

        residuo =  TAMANIO%nprocs;
        bloq_size = (TAMANIO - residuo)/nprocs;

        // MPI_Send(&data[start],bloq_size+residuo,MPI_INT,0,0,MPI_COMM_WORLD);
    }

    if (rank == 0) {
        int data[TAMANIO]; //vector de 100 elemento sin inicializar

        std::printf("total ranks:%d\n", nprocs);
        for (int i = 0; i < TAMANIO; i++) {
            data[i] = i;
        }



        for (int rank_id =1; rank_id < nprocs; rank_id++) {
            std::printf("Rank_0 Enviando Datos a RANK_%d\n", rank_id);
            int start = (rank_id *bloq_size)+residuo;


            MPI_Send(&data[start],bloq_size,MPI_INT,rank_id,0,MPI_COMM_WORLD);
        }

        //std::vector
        std::printf("block size %d y residuo %d \n",bloq_size,residuo);
        int suma_ranks[4];


        suma_ranks[0] = sumar(data,bloq_size+residuo);

        for (int rank_id = 1; rank_id < nprocs; rank_id++) {
            MPI_Recv(&suma_ranks[rank_id],1,MPI_INT,rank_id,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        std::printf("sumas parciales de : %d , %d , %d , %d\n", suma_ranks[0], suma_ranks[1], suma_ranks[2], suma_ranks[3]);
        int suma_total = sumar(suma_ranks,4);
        std::printf("SUMA TOTAL: %d\n", suma_total);

    } else {
        int  datos[bloq_size];
        std::printf("Rank_%d recibiendo datos\n", rank);

        MPI_Recv(datos, bloq_size, MPI_INT, 0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);


        int suma_parcial = sumar(datos,bloq_size);

        std::printf("RANK_%d enviando suma parcial %d\n", rank, suma_parcial);
        MPI_Send(&suma_parcial,1,MPI_INT,0,0,MPI_COMM_WORLD);
    }

    //std::printf("Rank %d of %d procs\n",rank,nprocs);

    MPI_Finalize();

    return 0;
}