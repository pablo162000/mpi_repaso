#include <iostream>
#include <chrono>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <mpi.h>
#include <cmath>

namespace ch = std::chrono;

static int image_width;
static int image_height;
static int image_channels;

sf::Uint8* blur_image_pixels = nullptr;

#define BLUR_RADIO  21

double last_time = 0;

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

void blur_image(const sf::Uint8* image) {
    if(!blur_image_pixels)
        blur_image_pixels = new sf::Uint8[image_width*image_height*image_channels];

#pragma omp parallel for collapse(2)
    for(int i=0;i<image_width;i++) {
        for(int j=0;j<image_height;j++) {

            auto [r,g,b] = process_pixel(image, image_width, image_height, i,j);

            int index = (j*image_width+i)*image_channels;

            blur_image_pixels[index] = r;
            blur_image_pixels[index+1] = g;
            blur_image_pixels[index+2] = b;
            blur_image_pixels[index+3] = 255;
        }
    }

//    sf::Image im;
//    im.create(image_width, image_height, blur_image_pixels);
//    im.saveToFile("image-blur.png");
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank, nprocs;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    int real_size;
    int block_size;
    int padding;
    const sf::Uint8* image;
    int tamanio;
    if(rank==0){
        //load image
        std::string filename = "D:/Programacion Paralela/mpi_repaso/image01.jpg";

        sf::Image im;
        im.loadFromFile(filename);

        image_width = im.getSize().x;
        image_height = im.getSize().y;
        image_channels = 4;
        image=im.getPixelsPtr();

        tamanio= image_width*image_height*image_channels;
        std::printf("tamanio=%d \n",tamanio);
        real_size=tamanio;
        block_size=tamanio/nprocs;
        padding=0;
        if(tamanio%nprocs !=0){
            real_size=std::ceil((double)tamanio/nprocs)*nprocs;
            block_size=real_size/nprocs;
            padding=real_size-tamanio;
        }
    }
    MPI_Bcast(&block_size , 1 , MPI_INT , 0 , MPI_COMM_WORLD);

    std::printf("Block size=%d \n",block_size);

    sf::Uint8* image_local = new sf::Uint8[block_size/4];
    MPI_Scatter(image, block_size/4, MPI_UNSIGNED_CHAR, image_local , block_size/4, MPI_UNSIGNED_CHAR, 0 , MPI_COMM_WORLD);



    MPI_Finalize();

    /*sf::Text text;
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
    }*/

    /*sf::Texture texture;
    texture.create(image_width, image_height);
    texture.update(image);

    int w = 1600;
    int h = 900;

    sf::RenderWindow  window(sf::VideoMode(w, h), "OMP Blur example");

    sf::Sprite sprite;
    {
        sprite.setTexture(texture);

        float scaleFactorX = w * 1.0 / im.getSize().x;
        float scaleFactorY = h * 1.0 / im.getSize().y;
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
                    texture.update(im.getPixelsPtr());
                    last_time = 0;
                }
                else if(event.key.scancode==sf::Keyboard::Scan::B) {
                    auto start = ch::high_resolution_clock::now();
                    {
                        blur_image(im.getPixelsPtr());
                        auto end = ch::high_resolution_clock::now();
                        ch::duration<double, std::milli> tiempo = end - start;

                        last_time = tiempo.count();
                    }

                    texture.update(blur_image_pixels);
                }
            }
            else if(event.type==sf::Event::Resized) {
                float scaleFactorX = event.size.width *1.0 / im.getSize().x;
                float scaleFactorY = event.size.height *1.0 /im.getSize().y;

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
        //window.draw(text);
        //window.draw(textOptions);
        window.display();
    }*/

    return 0;
}