#include <iostream>
#include <mpi.h>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

std::vector<int> read_file() {
    std::fstream fs("/workspace/mpi2324/datos.txt", std::ios::in );
    std::string line;
    std::vector<int> ret;
    while( std::getline(fs, line) ){
        ret.push_back( std::stoi(line) );
    }
    fs.close();
    return ret;
}


int maxNumero(int* lista, int tamanio){
    int max=lista[0];
    for (int i = 1; i < tamanio; ++i) {
        if(max<lista[i]){
            max=lista[i];
        }
    }
    return max;
}

int minNumero(int* lista, int tamanio){
    int min=lista[0];
    for (int i = 1; i < tamanio; ++i) {
        if(min>lista[i]){
            min=lista[i];
        }
    }
    return min;
}

double serial_promedio( std::vector<int> data) {
    double suma = 0;
    for (int dato : data) {
        suma = suma + dato;
    }

    return suma / data.size();
}


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;

    int n;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    std::vector<int> data = read_file();

    int block_size = data.size()/nprocs;
    int real_size;
    int padding;

    int maximo_items = data.size();

    int max;
    int min;
    int promedio;

    if (maximo_items % nprocs != 0) {
        real_size = std::ceil((double) maximo_items / nprocs) * nprocs;
        block_size = real_size / nprocs;
        padding = real_size - maximo_items;
    }


    if (rank == 0) {
        std::vector<int> data = read_file();

        std::printf("\n");
        for (int i = 1; i < nprocs; ++i) {
            int block_size= data.size()/(nprocs);
            int inicio=i*block_size;
            if(i==nprocs-1){
                block_size=block_size-padding;
            }
            MPI_Send(&data[inicio], block_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        int max_parciales[nprocs];
        int min_parciales[nprocs];
        std::vector<int> promedios_parciales(nprocs);
        max_parciales[0] = maxNumero(data.data(), block_size);
        min_parciales[0] = minNumero(data.data(), block_size);
        promedios_parciales[0]  = serial_promedio(data);
        MPI_Gather(MPI_IN_PLACE, 0, MPI_INT,
                   max_parciales, 1, MPI_INT,
                   0, MPI_COMM_WORLD);

        MPI_Gather(MPI_IN_PLACE, 0, MPI_INT,
                   min_parciales, 1, MPI_INT,
                   0, MPI_COMM_WORLD);
        MPI_Gather(MPI_IN_PLACE, 0, MPI_INT,
                   promedios_parciales.data(), 1, MPI_INT,
                   0, MPI_COMM_WORLD);

        max = maxNumero(max_parciales, nprocs);
        min = minNumero(min_parciales, nprocs);
        promedio = serial_promedio(promedios_parciales);
        std::printf("Maximo Total=%d\n", max);
        std::printf("Minimo Total=%d\n", min);
        std::printf("Promedio Total=%d\n", promedio);


    } else {
        if (rank==nprocs-1){
            block_size=block_size-padding;
        }
        MPI_Recv(data.data(), block_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int max_parcial = maxNumero(data.data(), block_size);
        MPI_Gather(&max_parcial, 1, MPI_INT,
                   nullptr, 0, MPI_INT,
                   0, MPI_COMM_WORLD);
        int min_parcial = minNumero(data.data(), block_size);
        MPI_Gather(&max_parcial, 1, MPI_INT,
                   nullptr, 0, MPI_INT,
                   0, MPI_COMM_WORLD);
        int promedios_parcial  = serial_promedio(data);
        MPI_Gather(&promedios_parcial, 1, MPI_INT,
                   nullptr, 0, MPI_INT,
                   0, MPI_COMM_WORLD);



    }
    MPI_Finalize();
    return 0;
}