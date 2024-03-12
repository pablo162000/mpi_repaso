#include <iostream>
#include <mpi.h>
#include <vector>
#include <cmath>
#include <functional>
#include <cstdlib>
#include <fmt/core.h>
#include <fmt/ranges.h>
 
int main(int argc, char** argv){
    MPI_Init(&argc,&argv);
 
    int rank, nprocs;
 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    std::vector<int> datos={8,23,19,67,45,35,1,24,13,30,3,5};
    int block_size=std::ceil(datos.size()/nprocs);
    
    std::vector<int> datos_local(block_size);

    MPI_Scatter( datos.data(), block_size , MPI_INT, datos_local.data() , block_size, MPI_INT , 0 , MPI_COMM_WORLD);

    std::sort(datos_local.begin(), datos_local.end());

    std::string str="";
    for(int i=0;i<datos_local.size();i++){
        str=str+std::to_string(datos_local[i])+",";
    }
    std::printf("Rank_%d datos recibidos===>%s \n",rank,str.c_str());

    std::vector<int> datos_ordenados(datos.size());
    MPI_Gather(datos_local.data(), block_size, MPI_INT, datos_ordenados.data(), block_size, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::sort(datos_ordenados.begin(), datos_ordenados.end());
        std::string str_ordenado = "";
        for (int i = 0; i < datos_ordenados.size(); i++) {
            str_ordenado = str_ordenado + std::to_string(datos_ordenados[i]) + ",";
        }
        std::printf("Vector completamente ordenado en Rank_0 ===> %s\n", str_ordenado.c_str());
    }

    MPI_Finalize();
 
    return 0;
}