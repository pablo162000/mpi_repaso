#include <iostream>
#include <vector>
#include <mpi.h>
#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <chrono>

namespace ch = std::chrono;
static int image_width;
static int image_height;
static int image_channels = 4;
double last_time = 0;

std::vector<sf::Uint8> espejo_image_pixels;

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    auto start = ch::high_resolution_clock::now();

    sf::Image image;
    int width , height ;
    int padding = 0 ;
    if (rank == 0 ) {
        image.loadFromFile("C:/Users/pbsun/Downloads/mpi2324-main/mpi2324_main/im1.jpg");
        width = image.getSize().x;
        height = image.getSize().y;
        espejo_image_pixels.resize(width*height*image_channels);

    }

    MPI_Bcast(&width,1, MPI_INT, 0 , MPI_COMM_WORLD);
    MPI_Bcast(&height,1, MPI_INT, 0 , MPI_COMM_WORLD);
    int height_real = height;


    if(height % nprocs != 0 ){
        height_real  =  std::ceil((double)height/nprocs) * nprocs;
        padding= height_real  - height;
        fmt::println("height real {}",height_real);
        fmt::println("padding {}",padding);

    }

    int rows_per_rank = height_real/ nprocs;
    //int tmp_rows_per_rank = rows_per_rank;

    if (rank == nprocs - 1) {
        rows_per_rank = height-(rank*rows_per_rank) ;
    }



    int start1 = rank*rows_per_rank;
    int end1 = start1+ rows_per_rank;
    fmt::println("Filas por proceso: {}, start_row: {}, end_row: {}",rows_per_rank,start1,end1);
    // Calcular la cantidad de píxeles por fila
    int pixels_per_row = width * 4; // 4 canales (RGBA)

    // Crear buffer para las filas a procesar
    std::vector<sf::Uint8> buffer( (rows_per_rank)* pixels_per_row);
    std::vector<sf::Uint8> buffer2((rows_per_rank) * pixels_per_row);


    // Distribuir filas de la imagen a cada proceso
    MPI_Scatter(image.getPixelsPtr(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
                buffer.data(), pixels_per_row *  (rows_per_rank), MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);



    // Procesamiento de los píxeles en cada proceso
    for (int i = 0; i < (rows_per_rank); i++) {
        int start =(i*width*4);
        int end = start+(width*4)-1;
        int condicion = end;
        while (start < condicion) {
            buffer2[start]=buffer[end-3];
            buffer2[start+1]=buffer[end-2];
            buffer2[start+2]=buffer[end-1];
            buffer2[start+3]=buffer[end];
            start+=4;
            end-=4;
        }
    }

    MPI_Gather(buffer2.data(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
               espejo_image_pixels.data(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);
    auto end = ch::high_resolution_clock::now();

    MPI_Finalize();
    if (rank == 0) {
        image.saveToFile("image_gris.jpg");
        sf::Text text;
        sf::Font font;
        {
            font.loadFromFile("arial.ttf");
            text.setFont(font);
            text.setString("Mandelbrot set");
            text.setCharacterSize(24); // in pixels, not points!
            text.setFillColor(sf::Color::White);
            text.setStyle(sf::Text::Bold);
            text.setPosition(10,10);
        }

        sf::Text textOptions;
        {
            font.loadFromFile("arial.ttf");
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
                        texture.update(espejo_image_pixels.data());
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


