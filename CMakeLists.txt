cmake_minimum_required(VERSION 3.26)
project(mpi2324_main)

set(CMAKE_CXX_STANDARD 17)

find_package(MPI REQUIRED)

find_package(fmt)
find_package(SFML)

add_executable(repaso_examen repaso_examen.cpp)
target_link_libraries(repaso_examen fmt::fmt sfml::sfml MPI::MPI_CXX )


add_executable(correccion correccion.cpp)
target_link_libraries(correccion fmt::fmt sfml::sfml MPI::MPI_CXX )


add_executable(suma_imagenes suma_imagenes.cpp)
target_link_libraries(suma_imagenes fmt::fmt sfml::sfml MPI::MPI_CXX )


add_executable(espejo espejo.cpp)
target_link_libraries(espejo fmt::fmt sfml::sfml MPI::MPI_CXX )

add_executable(sobel sobel.cpp)
target_link_libraries(sobel fmt::fmt sfml::sfml MPI::MPI_CXX )
