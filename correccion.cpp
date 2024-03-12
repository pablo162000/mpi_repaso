#include <iostream>
#include <mpi.h>
#include <vector>
#include <math.h>
#include <fstream>
#include <string>

std::vector<int> read_file() {
    std::fstream fs("datos.txt", std::ios::in);
    std::string line;

    std::vector<int> ret;

    while (std::getline(fs, line)) {
        ret.push_back(std::stoi(line));
    }

    fs.close();

    return ret;
}

std::vector<int> calculo_frecuencias(int* datos, int size){

    std::vector<int> frecuencias(101);

    for(int i = 0; i < size; i++){
        frecuencias[datos[i]] ++;
    }

    return frecuencias;
}

void tabla(std::vector<int> frec) {
    std::printf("+-------+--------+ \n");
    std::printf("| Valor | Conteo | \n");
    std::printf("+-------+--------+ \n");

    for (int i = 0; i <= 100; i++) {
        std::printf("|  %3d  |  %5d | \n", i, frec[i]);
    }

    std::printf("+-------+--------+ \n");
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank, nprocs;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int block_size;
    int real_size;
    int padding = 0;

    std::vector<int> datos;
    std::vector<int> datos_local;

    if(rank == 0){
        datos = read_file();

        int max_items = datos.size();

        real_size = max_items;
        
        if(max_items % nprocs != 0){
            real_size = std::ceil((double)max_items/nprocs) * nprocs;
            padding = real_size - max_items;
        }

        block_size = real_size / nprocs;

        std::printf("Dimension: %d, real_size: %d, block_size: %d, padding: %d \n",
                    max_items, real_size, block_size, padding);
    }

    // Enviar el tamaño del bloque
    MPI_Bcast(&block_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0){
        MPI_Send(&padding, 1, MPI_INT, nprocs-1, 0, MPI_COMM_WORLD);
    } else if(rank == nprocs -1){
        MPI_Recv(&padding, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Reserva de memoria para cada rank
    datos_local.resize(block_size);

    // Enviar los datos
    MPI_Scatter(datos.data(), block_size, MPI_INT,
                datos_local.data(), block_size, MPI_INT,
                0, MPI_COMM_WORLD);

    // Realizar el cálculo
    int new_size = block_size;
    if (rank == nprocs - 1){
        new_size = block_size - padding;
    }

    std::printf("RANK_%d calculando histograma, block_size = %d", rank, new_size);

    auto H_local = calculo_frecuencias(datos_local.data(), new_size);

    // Recibir los datos
    std::vector<int> H;

    if(rank == 0){
        H.resize(101);
    }
    
    MPI_Reduce(H_local.data(), H.data(), 101, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(rank == 0){
        // Imprimir el resultado
        tabla(H);
    }

    MPI_Finalize();

    return 0;

}