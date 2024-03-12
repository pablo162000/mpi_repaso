#include <iostream>
#include <vector>
#include <mpi.h>
#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <chrono>git push -u origin main
namespace ch = std::chrono;

#define BLUR_RADIO  21
static int image_width;
static int image_height;
static int image_channels=4;
double last_time = 0;

std::vector<sf::Uint8> image_modificada;
std::vector<sf::Uint8> image_min_mod;
std::vector<sf::Uint8> image_max_mod;

std::tuple<sf::Uint8,sf::Uint8,sf::Uint8> process_pixel_mediana(const sf::Uint8* image, int width, int height, int x, int y) {

    int r = 0;
    int g = 0;
    int b = 0;
    std::vector<sf::Uint8> r_pixels;
    std::vector<sf::Uint8> g_pixels;
    std::vector<sf::Uint8> b_pixels;
    int cc = 0;

    for(int i=x-1;i<=x+1;i++) {
        for(int j=y-1;j<=y+1;j++) {
            int index = (j * width + i)*image_channels;

            if(i>=0 && i<width && j>=0 && j<height) {
                r_pixels.push_back(image[index]);
                g_pixels.push_back(image[index + 1]);
                b_pixels.push_back(image[index + 2]);
            }
        }
    }
    std::sort(r_pixels.begin(), r_pixels.end());
    std::sort(g_pixels.begin(), g_pixels.end());
    std::sort(b_pixels.begin(), b_pixels.end());

    int indiceMediana= r_pixels.size()/2;
    return {r_pixels[indiceMediana], g_pixels[indiceMediana], b_pixels[indiceMediana]};
}

std::tuple<sf::Uint8,sf::Uint8,sf::Uint8> process_pixel_min(const sf::Uint8* image, int width, int height, int x, int y) {

    int r = 0;
    int g = 0;
    int b = 0;
    std::vector<sf::Uint8> r_pixels;
    std::vector<sf::Uint8> g_pixels;
    std::vector<sf::Uint8> b_pixels;
    int cc = 0;

    for(int i=x-1;i<=x+1;i++) {
        for(int j=y-1;j<=y+1;j++) {
            int index = (j * width + i)*image_channels;

            if(i>=0 && i<width && j>=0 && j<height) {
                r_pixels.push_back(image[index]);
                g_pixels.push_back(image[index + 1]);
                b_pixels.push_back(image[index + 2]);
            }
        }
    }
    std::sort(r_pixels.begin(), r_pixels.end());
    std::sort(g_pixels.begin(), g_pixels.end());
    std::sort(b_pixels.begin(), b_pixels.end());

    return {r_pixels[0], g_pixels[0], b_pixels[0]};
}

std::tuple<sf::Uint8,sf::Uint8,sf::Uint8> process_pixel_max(const sf::Uint8* image, int width, int height, int x, int y) {

    int r = 0;
    int g = 0;
    int b = 0;
    std::vector<sf::Uint8> r_pixels;
    std::vector<sf::Uint8> g_pixels;
    std::vector<sf::Uint8> b_pixels;
    int cc = 0;

    for(int i=x-1;i<=x+1;i++) {
        for(int j=y-1;j<=y+1;j++) {
            int index = (j * width + i)*image_channels;

            if(i>=0 && i<width && j>=0 && j<height) {
                r_pixels.push_back(image[index]);
                g_pixels.push_back(image[index + 1]);
                b_pixels.push_back(image[index + 2]);
            }
        }
    }
    std::sort(r_pixels.begin(), r_pixels.end(), std::greater<int>());
    std::sort(g_pixels.begin(), g_pixels.end(),std::greater<int>());
    std::sort(b_pixels.begin(), b_pixels.end(),std::greater<int>());

    return {r_pixels[0], g_pixels[0], b_pixels[0]};
}



std::vector<sf::Uint8> image_median(const sf::Uint8 *image, int width, int height) {
    std::vector<sf::Uint8> image_nueva(width * height * image_channels);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            auto [r, g, b] = process_pixel_mediana(image, width, height, i,j);

            int index = (j * width + i) * image_channels;

            image_nueva[index] = r;
            image_nueva[index + 1] = g;
            image_nueva[index + 2] = b;
            image_nueva[index + 3] = 255;
        }
    }
    return image_nueva;
}

std::vector<sf::Uint8> image_min(const sf::Uint8 *image, int width, int height) {
    std::vector<sf::Uint8>image_nueva(width * height * image_channels);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            auto [r, g, b] = process_pixel_min(image, width, height, i,j);

            int index = (j * width + i) * image_channels;

            image_nueva[index] = r;
            image_nueva[index + 1] = g;
            image_nueva[index + 2] = b;
            image_nueva[index + 3] = 255;
        }
    }
    return image_nueva;
}

std::vector<sf::Uint8> image_max(const sf::Uint8 *image, int width, int height) {
    std::vector<sf::Uint8>image_nueva(width * height * image_channels);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            auto [r, g, b] = process_pixel_max(image, width, height, i,j);

            int index = (j * width + i) * image_channels;

            image_nueva[index] = r;
            image_nueva[index + 1] = g;
            image_nueva[index + 2] = b;
            image_nueva[index + 3] = 255;
        }
    }
    return image_nueva;
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
        image.loadFromFile("D:/Programacion Paralela/mpi_repaso/imagen.jpg");
    }

    // Broadcast del tamaño de la imagen
    int width, height;
    int rows_per_process;
    if (rank == 0) {
        width = image.getSize().x;
        height = image.getSize().y;
        image_modificada.resize(width*height*image_channels);
        image_min_mod.resize(width*height*image_channels);
        image_max_mod.resize(width*height*image_channels);
        rows_per_process = std::ceil((double) height / size);
    }
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&rows_per_process, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Cada proceso calcula qué filas de la imagen procesará

    int start_row = rank * rows_per_process;
    int end_row = start_row + rows_per_process;

    // Asegurarse de que el último proceso obtenga las filas restantes
    if (rank == size - 1) {
        end_row = height;
    }

    fmt::println("Filas por proceso: {}, start_row: {}, end_row: {}",rows_per_process,start_row,end_row);

    // Calcular la cantidad de píxeles por fila
    int pixels_per_row = width * 4; // 4 canales (RGBA)
    int real_rows=(end_row - start_row);
    // Crear buffer para las filas a procesar
    std::vector<sf::Uint8> buffer(real_rows * pixels_per_row);
    std::vector<sf::Uint8> buffer2(real_rows * pixels_per_row);

    // Distribuir filas de la imagen a cada proceso
    MPI_Scatter(image.getPixelsPtr(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
                buffer.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    // Procesamiento de los píxeles en cada proceso
    buffer2=image_median(buffer.data(),width,real_rows);

    fmt::println("Filas por proceso: {}, real_row: {}",rows_per_process,real_rows);
    MPI_Gather(buffer2.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               image_modificada.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    // Procesamiento de los píxeles en cada proceso
    buffer2=image_min(buffer.data(),width,real_rows);

    fmt::println("Filas por proceso: {}, real_row: {}",rows_per_process,real_rows);
    MPI_Gather(buffer2.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               image_min_mod.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    buffer2=image_max(buffer.data(),width,real_rows);

    fmt::println("Filas por proceso: {}, real_row: {}",rows_per_process,real_rows);
    MPI_Gather(buffer2.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               image_max_mod.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
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
                    else if(event.key.scancode==sf::Keyboard::Scan::M) {
                        ch::duration<double, std::milli> tiempo = end - start;
                        last_time = tiempo.count();
                        texture.update(image_modificada.data());
                    }
                    else if(event.key.scancode==sf::Keyboard::Scan::B) {
                        ch::duration<double, std::milli> tiempo = end - start;
                        last_time = tiempo.count();
                        texture.update(image_min_mod.data());
                    }
                    else if(event.key.scancode==sf::Keyboard::Scan::N) {
                        ch::duration<double, std::milli> tiempo = end - start;
                        last_time = tiempo.count();
                        texture.update(image_max_mod.data());
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