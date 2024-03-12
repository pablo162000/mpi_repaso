#include <iostream>
#include <vector>
#include <mpi.h>
#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <chrono>
namespace ch = std::chrono;

#define BLUR_RADIO  21
static int image_channels=4;
double last_time = 0;

std::vector<sf::Uint8> blur_image_pixels;


std::tuple<sf::Uint8, sf::Uint8, sf::Uint8> process_pixel_sobel(const sf::Uint8 *image, int width, int height, int x, int y) {
    int r_x = 0;
    int g_x = 0;
    int b_x = 0;

    int r_y = 0;
    int g_y = 0;
    int b_y = 0;

    int sobel_x[] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    int sobel_y[] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

    for (int i = x-1; i <= x+1; i++) {
        for (int j = y-1; j <=y+ 1; j++) {
            int index = (j * width + i)*image_channels;

            if (index >= 0 && index <= width * height * 4) {
                int matrixIndex = (i-x + 1) * 3 + (j-y + 1);

                int weight_x = sobel_x[matrixIndex];
                int weight_y = sobel_y[matrixIndex];

                r_x += image[index] * weight_x;
                g_x += image[index + 1] * weight_x;
                b_x += image[index + 2] * weight_x;

                r_y += image[index] * weight_y;
                g_y += image[index + 1] * weight_y;
                b_y += image[index + 2] * weight_y;
            }
        }
    }

    int r = std::clamp((std::abs(r_x) + std::abs(r_y))/2, 0, 255);
    int g = std::clamp((std::abs(g_x) + std::abs(g_y))/2, 0, 255);
    int b = std::clamp((std::abs(b_x) + std::abs(b_y))/2, 0, 255);

    return { r, g, b };
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    auto start = ch::high_resolution_clock::now();
    // Broadcast del tamaño de la imagen
    int width, height;
    // Proceso raíz carga la imagen
    sf::Image image;
    //sf::Image image2;
    int padding = 0 ;

    if (rank == 0) {
        image.loadFromFile("C:/Users/pbsun/Downloads/mpi2324-main/mpi2324_main/lobo.jpeg");
        width = image.getSize().x;
        height = image.getSize().y;
        blur_image_pixels.resize(width*height*image_channels);
    }
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int height_real = height;
    if(height % nprocs != 0 ){
        height_real  =  std::ceil((double)height/nprocs) * nprocs;
        padding= height_real  - height;
        fmt::println("height real {}",height_real);
        fmt::println("padding {}",padding);

    }
    int rows_per_rank = height_real/ nprocs;

    if (rank == nprocs - 1) {
        rows_per_rank = height-(rank*rows_per_rank) ;
    }

    int start1 = rank*rows_per_rank;
    int end1 = start1+ rows_per_rank;
    fmt::println("Filas por proceso: {}, start_row: {}, end_row: {}",rows_per_rank,start1,end1);

    int suma= suma + rows_per_rank;
    fmt::println("valor: {}",suma);


    // Cada proceso calcula qué filas de la imagen procesará
    int pixels_per_row = width * 4; // 4 canales (RGBA)



    // Crear buffer para las filas a procesar
    std::vector<sf::Uint8> buffer((rows_per_rank) * pixels_per_row);
    std::vector<sf::Uint8> buffer2((rows_per_rank) * pixels_per_row);

    // Distribuir filas de la imagen a cada proceso
    MPI_Scatter(image.getPixelsPtr(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
                buffer.data(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    // Procesamiento de los píxeles en cada proceso
    for (int i = 0; i < (rows_per_rank); ++i) {
        for (int j = 0; j < width; ++j) {
            // Procesar píxeles aquí

            auto [r, g, b] = process_pixel_sobel(buffer.data(), width, (rows_per_rank), j, i);

            int index = (i * width + j) * image_channels;

            buffer2[index] = r;
            buffer2[index + 1] = g;
            buffer2[index + 2] = b;
            buffer2[index + 3] = 255;
        }
    }

    MPI_Gather(buffer2.data(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
               blur_image_pixels.data(), pixels_per_row * (rows_per_rank), MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        //image.saveToFile("image_blurred.jpg");

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

        sf::Texture texture;

        texture.create(image.getSize().x, image.getSize().y);
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
        textOptions.setPosition(10, window.getView().getSize().y-40);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                else if(event.type==sf::Event::KeyReleased) {
                    if(event.key.scancode==sf::Keyboard::Scan::R) {
                        image.loadFromFile("C:/Users/pbsun/Downloads/mpi2324-main/mpi2324_main/lobo.jpeg");

                        texture.update(image.getPixelsPtr());
                        last_time = 0;
                    }
                    else if(event.key.scancode==sf::Keyboard::Scan::B) {
                        auto start = ch::high_resolution_clock::now();
                        {
                            //blur_image(im.getPixelsPtr());
                            auto end = ch::high_resolution_clock::now();
                            ch::duration<double, std::milli> tiempo = end - start;

                            last_time = tiempo.count();
                        }

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

            auto msg = fmt::format("Blur time: {}ms, FPS: {}", last_time, fps);
            text.setString(msg);

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

    MPI_Finalize();



//mpiexec -n 4 ./cmake-build-release/sobel

}