#include <iostream>
#include <vector>
#include <mpi.h>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>



std::vector<int> serial_frecuencias(int tamanio, std::vector<int> datos,int tamanio_vector) {

    std::vector<int> cc(tamanio + 1);



    std::vector<int> c1(tamanio_vector);

    for (int i=0;i<tamanio_vector;i++) {
        c1[i]=datos[i];
    }

    for (int dato: c1) {
        cc[dato]++;
    }

    return cc;
}


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


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    std::vector<int> data = read_file();

    int block_size = data.size()/nprocs;
    int real_size;
    int padding;

    int maximo_items = data.size();


    if (maximo_items % nprocs != 0) {
        real_size = std::ceil((double) maximo_items / nprocs) * nprocs;
        block_size = real_size / nprocs;
        padding = real_size - maximo_items;
    }




    if (rank == 0) {

        std::vector<int> data = read_file();
        for (int i = 1; i < nprocs; ++i) {
            int block_size= data.size()/(nprocs);
            int inicio=i*block_size;
            if(i==nprocs-1){
                block_size=block_size-padding;
            }
            MPI_Send(&data[inicio], block_size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

    }

    std::vector<int> datos_aleatorios_local(block_size);


    if (rank != 0) {

        MPI_Recv(data.data(), block_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    }

//    MPI_Scatter(data.data(), block_size, MPI_INT, datos_aleatorios_local.data(), block_size, MPI_INT, 0,
//                MPI_COMM_WORLD);



    if(rank==nprocs-1){
        block_size= block_size-padding;
    }

    std::vector<int> conteo_local(101);

    //conteo_local = serial_frecuencias(100, datos_aleatorios_local,block_size);

    conteo_local = serial_frecuencias(100, data,block_size);

    std::vector<int> conteo(101);


    MPI_Reduce(conteo_local.data(),conteo.data(),101,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);


    if(rank ==0) {
        for (int i = 0; i < 101; i++) {
            std::printf(" conteo%d:[%d]\n",i, conteo[i]);
        }

    }




    MPI_Finalize();

    return 0;
}



