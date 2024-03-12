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

std::vector<sf::Uint8> image_edge;
std::vector<sf::Uint8> image_sobel;

std::tuple<sf::Uint8, sf::Uint8, sf::Uint8> process_pixel_edge(const sf::Uint8 *image, int width, int height, int pos) {
    int r = 0;
    int g = 0;
    int b = 0;

    // Realce
    //int matriz[] = { 0, 0, 0, -1, 1, 0, 0, 0, 0 };

    // Detectar
    int matriz[] = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
    //repujado
    //int matriz[] = { -2,-1,0,-1,1,1,0,1,2 };

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int index = (pos * 4) + (i * 4) + (j * width * 4);

            if (index >= 0 && index <= width * height * 4) {
                int matrixIndex = (i + 1) * 3 + (j + 1);
                int weight = matriz[matrixIndex];

                r += image[index] * weight;
                g += image[index + 1] * weight;
                b += image[index + 2] * weight;
            }
        }
    }

    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);

    return { r, g, b };
}

std::vector<sf::Uint8> blur_image_edge(const sf::Uint8 *image, int width, int height) {
    std::vector<sf::Uint8> image_nueva(width * height * image_channels);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            auto [r, g, b] = process_pixel_edge(image, width, height, j * width + i);

            int index = (j * width + i) * image_channels;

            image_nueva[index] = r;
            image_nueva[index + 1] = g;
            image_nueva[index + 2] = b;
            image_nueva[index + 3] = 255;
        }
    }
    return image_nueva;
}

std::tuple<sf::Uint8, sf::Uint8, sf::Uint8>
process_pixel_sobel(const sf::Uint8 *image, int width, int height, int pos) {
    int r_x = 0;
    int g_x = 0;
    int b_x = 0;

    int r_y = 0;
    int g_y = 0;
    int b_y = 0;

    int sobel_x[] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
    int sobel_y[] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int index = (pos * 4) + (i * 4) + (j * width * 4);

            if (index >= 0 && index <= width * height * 4) {
                int matrixIndex = (i + 1) * 3 + (j + 1);

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

std::vector<sf::Uint8> blur_image_sobel(const sf::Uint8 *image, int width, int height) {
    std::vector<sf::Uint8>image_nueva(width * height * image_channels);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            auto [r, g, b] = process_pixel_sobel(image, width, height, j * width + i);

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
        image.loadFromFile("D:/Programacion Paralela/mpi_repaso/image02.jpg");
    }

    // Broadcast del tamaño de la imagen
    int width, height;
    int rows_per_process;
    if (rank == 0) {
        width = image.getSize().x;
        height = image.getSize().y;
        image_sobel.resize(width*height*image_channels);
        image_edge.resize(width*height*image_channels);
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
    buffer2=blur_image_edge(buffer.data(),width,real_rows);

    fmt::println("Filas por proceso: {}, real_row: {}",rows_per_process,real_rows);
    MPI_Gather(buffer2.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               image_edge.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    // Procesamiento de los píxeles en cada proceso
    buffer2=blur_image_sobel(buffer.data(),width,real_rows);

    fmt::println("Filas por proceso: {}, real_row: {}",rows_per_process,real_rows);
    MPI_Gather(buffer2.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
               image_sobel.data(), pixels_per_row * real_rows, MPI_UNSIGNED_CHAR,
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
                        texture.update(image_edge.data());
                    }
                    else if(event.key.scancode==sf::Keyboard::Scan::N) {
                        ch::duration<double, std::milli> tiempo = end - start;
                        last_time = tiempo.count();
                        texture.update(image_sobel.data());
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