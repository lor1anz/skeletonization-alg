#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data;

    Image(int w, int h, uint8_t value = 0)
        : width(w), height(h), data(static_cast<size_t>(w * h), value) {}

    uint8_t& operator()(int y, int x) {
        return data[static_cast<size_t>(y * width + x)];
    }
};

void WritePgm(const Image& img, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot open output file: " + path);
    }

    out << "P5\n";
    out << img.width << " " << img.height << "\n";
    out << "255\n";
    out.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
}

void FillCircle(Image& img, int cx, int cy, int radius) {
    const int r2 = radius * radius;

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;

            if (dx * dx + dy * dy <= r2) {
                img(y, x) = 255;
            }
        }
    }
}

void FillEllipse(Image& img, int cx, int cy, int rx, int ry) {
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const double dx = static_cast<double>(x - cx) / static_cast<double>(rx);
            const double dy = static_cast<double>(y - cy) / static_cast<double>(ry);

            if (dx * dx + dy * dy <= 1.0) {
                img(y, x) = 255;
            }
        }
    }
}

void FillRectangle(Image& img, int x0, int y0, int x1, int y1) {
    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);
    x1 = std::min(x1, img.width - 1);
    y1 = std::min(y1, img.height - 1);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            img(y, x) = 255;
        }
    }
}

void FillThickLine(Image& img, int x0, int y0, int x1, int y1, int radius) {
    const int min_x = std::max(0, std::min(x0, x1) - radius);
    const int max_x = std::min(img.width - 1, std::max(x0, x1) + radius);
    const int min_y = std::max(0, std::min(y0, y1) - radius);
    const int max_y = std::min(img.height - 1, std::max(y0, y1) + radius);

    const double vx = static_cast<double>(x1 - x0);
    const double vy = static_cast<double>(y1 - y0);
    const double len2 = vx * vx + vy * vy;

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double t = 0.0;
            if (len2 > 0.0) {
                t = ((x - x0) * vx + (y - y0) * vy) / len2;
                t = std::max(0.0, std::min(1.0, t));
            }

            const double px = x0 + t * vx;
            const double py = y0 + t * vy;

            const double dx = x - px;
            const double dy = y - py;

            if (dx * dx + dy * dy <= radius * radius) {
                img(y, x) = 255;
            }
        }
    }
}

Image MakeCircle(int w, int h) {
    Image img(w, h, 0);
    FillCircle(img, w / 2, h / 2, std::min(w, h) / 4);
    return img;
}

Image MakeEllipse(int w, int h) {
    Image img(w, h, 0);
    FillEllipse(img, w / 2, h / 2, w / 3, h / 6);
    return img;
}

Image MakeRectangle(int w, int h) {
    Image img(w, h, 0);
    FillRectangle(img, w / 4, h / 3, 3 * w / 4, 2 * h / 3);
    return img;
}

Image MakeDumbbell(int w, int h) {
    Image img(w, h, 0);

    FillCircle(img, w / 3, h / 2, h / 5);
    FillCircle(img, 2 * w / 3, h / 2, h / 5);
    FillRectangle(img, w / 3, h / 2 - h / 16, 2 * w / 3, h / 2 + h / 16);

    return img;
}

Image MakeCross(int w, int h) {
    Image img(w, h, 0);

    FillRectangle(img, w / 2 - w / 16, h / 5, w / 2 + w / 16, 4 * h / 5);
    FillRectangle(img, w / 5, h / 2 - h / 16, 4 * w / 5, h / 2 + h / 16);

    return img;
}

Image MakeBranchingShape(int w, int h) {
    Image img(w, h, 0);

    const int cx = w / 2;
    const int cy = h / 2;
    const int thickness = std::min(w, h) / 28;

    FillCircle(img, cx, cy, thickness * 2);

    FillThickLine(img, cx, h / 5, cx, cy, thickness);
    FillThickLine(img, cx, cy, w / 4, 4 * h / 5, thickness);
    FillThickLine(img, cx, cy, 3 * w / 4, 4 * h / 5, thickness);

    return img;
}

Image MakeNoisyCircle(int w, int h) {
    Image img(w, h, 0);

    const int cx = w / 2;
    const int cy = h / 2;
    const int base_radius = std::min(w, h) / 4;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;

            const double angle = std::atan2(static_cast<double>(dy), static_cast<double>(dx));
            const double perturbation =
                10.0 * std::sin(5.0 * angle) +
                6.0 * std::sin(11.0 * angle);

            const double radius = base_radius + perturbation;

            if (dx * dx + dy * dy <= radius * radius) {
                img(y, x) = 255;
            }
        }
    }

    return img;
}

void GenerateAll(int width, int height) {
    WritePgm(MakeCircle(width, height), "circle.pgm");
    WritePgm(MakeEllipse(width, height), "ellipse.pgm");
    WritePgm(MakeRectangle(width, height), "rectangle.pgm");
    WritePgm(MakeDumbbell(width, height), "dumbbell.pgm");
    WritePgm(MakeCross(width, height), "cross.pgm");
    WritePgm(MakeBranchingShape(width, height), "branching.pgm");
    WritePgm(MakeNoisyCircle(width, height), "noisy_circle.pgm");
}

int main(int argc, char** argv) {
    try {
        int width = 256;
        int height = 256;

        if (argc == 3) {
            width = std::stoi(argv[1]);
            height = std::stoi(argv[2]);
        } else if (argc != 1) {
            std::cerr << "Usage:\n";
            std::cerr << "  " << argv[0] << "\n";
            std::cerr << "  " << argv[0] << " <width> <height>\n";
            return 1;
        }

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Width and height must be positive");
        }

        GenerateAll(width, height);

        std::cout << "Generated PGM shapes:\n";
        std::cout << "  circle.pgm\n";
        std::cout << "  ellipse.pgm\n";
        std::cout << "  rectangle.pgm\n";
        std::cout << "  dumbbell.pgm\n";
        std::cout << "  cross.pgm\n";
        std::cout << "  branching.pgm\n";
        std::cout << "  noisy_circle.pgm\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
