#include <iostream>
#include <vector>
#include <mpi.h>
#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <chrono>
namespace ch = std::chrono;

#define BLUR_RADIO  21
static int image_width;
static int image_height;
static int image_channels=4;
double last_time = 0;

std::vector<sf::Uint8> blur_image_pixels;

std::tuple<sf::Uint8,sf::Uint8,sf::Uint8> process_pixel(const sf::Uint8* image, int width, int height, int x, int y) {

    int r = 0;
    int g = 0;
    int b = 0;

    int cc = 0;

    int ratio = BLUR_RADIO/2;

    for(int i=x-ratio;i<=x+ratio;i++) {
        for(int j=y-ratio;j<=y+ratio;j++) {
            int index = (j * width + i)*image_channels;

            //if(index>=0 && index<=image_width*image_height*image_channels) {
            if(i>=0 && i<width && j>=0 && j<height) {
                r = r + image[index];
                g = g + image[index + 1];
                b = b + image[index + 2];
                cc++;
            }
        }
    }

    return {r/cc, g/cc, b/cc};
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    auto start = ch::high_resolution_clock::now();
    // Proceso raíz carga la imagen
    sf::Image image;
    if (rank == 0) {
        image.loadFromFile("D:/Programacion Paralela/mpi_repaso/image01.jpg");
    }

    // Broadcast del tamaño de la imagen
    int width, height;
    if (rank == 0) {
        width = image.getSize().x;
        height = image.getSize().y;
        blur_image_pixels.resize(width*height*image_channels);
    }
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso calcula qué filas de la imagen procesará
    int rows_per_process = height / size;
    int start_row = rank * rows_per_process;
    int end_row = start_row + rows_per_process;

    // Asegurarse de que el último proceso obtenga las filas restantes
    if (rank == size - 1) {
        end_row = height;
    }

    fmt::println("Filas por proceso: {}, start_row: {}, end_row: {}",rows_per_process,start_row,end_row);

    // Calcular la cantidad de píxeles por fila
    int pixels_per_row = width * 4; // 4 canales (RGBA)

    // Crear buffer para las filas a procesar
    std::vector<sf::Uint8> buffer((end_row - start_row) * pixels_per_row);

    // Distribuir filas de la imagen a cada proceso
    MPI_Scatter(image.getPixelsPtr(), pixels_per_row * rows_per_process, MPI_UNSIGNED_CHAR,
                buffer.data(), pixels_per_row * rows_per_process, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    // Procesamiento de los píxeles en cada proceso
    for (int i = 0; i < rows_per_process; i++) {
        for (int j = 0; j < width; j++) {

            int index = (i*width+j)*image_channels;
            unsigned int intensidad = buffer[index]*0.299+buffer[index+1]*0.587+buffer[index+2]*0.114;
            buffer[index] =  intensidad;
            buffer[index+1] = intensidad;
            buffer[index+2] = intensidad;
            buffer[index+3] = 255;
        }
    }

    MPI_Gather(buffer.data(), pixels_per_row * rows_per_process, MPI_UNSIGNED_CHAR,
               blur_image_pixels.data(), pixels_per_row * rows_per_process, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);
    auto end = ch::high_resolution_clock::now();

    MPI_Finalize();

    if (rank == 0) {
        image.saveToFile("image_gris.jpg");
        sf::Text text;
        sf::Font font;
        {
            font.loadFromFile("D:/Programacion Paralela/mpi_repaso/arial.ttf");
            text.setFont(font);
            text.setString("Mandelbrot set");
            text.setCharacterSize(24); // in pixels, not points!
            text.setFillColor(sf::Color::White);
            text.setStyle(sf::Text::Bold);
            text.setPosition(10,10);
        }

        sf::Text textOptions;
        {
            font.loadFromFile("D:/Programacion Paralela/mpi_repaso/arial.ttf");
            textOptions.setFont(font);
            textOptions.setCharacterSize(24);
            textOptions.setFillColor(sf::Color::White);
            textOptions.setStyle(sf::Text::Bold);
            textOptions.setString("OPTIONS: [R] Reset [B] Blur");
        }

        image_width = image.getSize().x;
        image_height = image.getSize().y;
        image_channels = 4;

        sf::Texture texture;
        texture.create(image_width, image_height);
        texture.update(image.getPixelsPtr());

        int w = 1600;
        int h = 900;

        sf::RenderWindow  window(sf::VideoMode(w, h), "OMP Blur example");

        sf::Sprite sprite;
        {
            sprite.setTexture(texture);

            float scaleFactorX = w * 1.0 / image.getSize().x;
            float scaleFactorY = h * 1.0 / image.getSize().y;
            sprite.scale(scaleFactorX, scaleFactorY);
        }

        sf::Clock clock;

        sf::Clock clockFrames;
        int frames = 0;
        int fps = 0;

        //textOptions.setPosition(10, window.getView().getSize().y-40);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                else if(event.type==sf::Event::KeyReleased) {
                    if(event.key.scancode==sf::Keyboard::Scan::R) {
                        texture.update(image.getPixelsPtr());
                        last_time = 0;
                    }
                    else if(event.key.scancode==sf::Keyboard::Scan::B) {


                        ch::duration<double, std::milli> tiempo = end - start;
                        last_time = tiempo.count();
                        texture.update(blur_image_pixels.data());
                    }
                }
                else if(event.type==sf::Event::Resized) {
                    float scaleFactorX = event.size.width *1.0 / image.getSize().x;
                    float scaleFactorY = event.size.height *1.0 /image.getSize().y;

                    sprite = sf::Sprite();
                    sprite.setTexture(texture);
                    sprite.scale(scaleFactorX, scaleFactorY);
                }
            }

            if(clockFrames.getElapsedTime().asSeconds()>=1.0) {
                fps = frames;
                frames = 0;
                clockFrames.restart();
            }
            frames++;

            window.clear(sf::Color::Black);
            window.draw(sprite);
            window.draw(text);
            window.draw(textOptions);
            window.display();
        }
    }

    return 0;
}