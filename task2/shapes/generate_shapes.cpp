#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
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

    uint8_t operator()(int y, int x) const {
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

    for (uint8_t v : img.data) {
        const uint8_t out_v = v ? 255 : 0;
        out.write(reinterpret_cast<const char*>(&out_v), 1);
    }
}

void SetPixel(Image& img, int x, int y) {
    if (0 <= x && x < img.width && 0 <= y && y < img.height) {
        img(y, x) = 1;
    }
}

void FillCircle(Image& img, int cx, int cy, int radius) {
    const int r2 = radius * radius;

    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int x = cx - radius; x <= cx + radius; ++x) {
            if (0 <= x && x < img.width && 0 <= y && y < img.height) {
                const int dx = x - cx;
                const int dy = y - cy;

                if (dx * dx + dy * dy <= r2) {
                    img(y, x) = 1;
                }
            }
        }
    }
}

void FillEllipse(Image& img, int cx, int cy, int rx, int ry) {
    for (int y = cy - ry; y <= cy + ry; ++y) {
        for (int x = cx - rx; x <= cx + rx; ++x) {
            if (0 <= x && x < img.width && 0 <= y && y < img.height) {
                const double dx = static_cast<double>(x - cx) / rx;
                const double dy = static_cast<double>(y - cy) / ry;

                if (dx * dx + dy * dy <= 1.0) {
                    img(y, x) = 1;
                }
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
            img(y, x) = 1;
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
                img(y, x) = 1;
            }
        }
    }
}

Image MakeRectangle(int w, int h, int thickness_scale) {
    Image img(w, h, 0);
    const int margin_x = w / 5;
    const int margin_y = h / 3;
    const int half_h = std::max(2, thickness_scale);
    FillRectangle(img, margin_x, h / 2 - half_h, w - margin_x, h / 2 + half_h);
    return img;
}

Image MakeLine0(int w, int h, int thickness) {
    Image img(w, h, 0);
    FillThickLine(img, w / 6, h / 2, 5 * w / 6, h / 2, thickness);
    return img;
}

Image MakeLine45(int w, int h, int thickness) {
    Image img(w, h, 0);
    FillThickLine(img, w / 5, 4 * h / 5, 4 * w / 5, h / 5, thickness);
    return img;
}

Image MakeLine90(int w, int h, int thickness) {
    Image img(w, h, 0);
    FillThickLine(img, w / 2, h / 6, w / 2, 5 * h / 6, thickness);
    return img;
}

Image MakeLine135(int w, int h, int thickness) {
    Image img(w, h, 0);
    FillThickLine(img, w / 5, h / 5, 4 * w / 5, 4 * h / 5, thickness);
    return img;
}

Image MakeDumbbell(int w, int h) {
    Image img(w, h, 0);

    FillCircle(img, w / 3, h / 2, h / 5);
    FillCircle(img, 2 * w / 3, h / 2, h / 5);
    FillRectangle(img, w / 3, h / 2 - h / 18, 2 * w / 3, h / 2 + h / 18);

    return img;
}

Image MakeCross(int w, int h) {
    Image img(w, h, 0);

    const int thickness = std::max(2, std::min(w, h) / 20);

    FillRectangle(img, w / 2 - thickness, h / 5, w / 2 + thickness, 4 * h / 5);
    FillRectangle(img, w / 5, h / 2 - thickness, 4 * w / 5, h / 2 + thickness);

    return img;
}

Image MakeLetterH(int w, int h) {
    Image img(w, h, 0);

    const int t = std::max(2, std::min(w, h) / 18);

    FillRectangle(img, w / 4 - t, h / 5, w / 4 + t, 4 * h / 5);
    FillRectangle(img, 3 * w / 4 - t, h / 5, 3 * w / 4 + t, 4 * h / 5);
    FillRectangle(img, w / 4, h / 2 - t, 3 * w / 4, h / 2 + t);

    return img;
}

Image MakeLetterA(int w, int h) {
    Image img(w, h, 0);

    const int t = std::max(2, std::min(w, h) / 22);

    FillThickLine(img, w / 5, 4 * h / 5, w / 2, h / 5, t);
    FillThickLine(img, 4 * w / 5, 4 * h / 5, w / 2, h / 5, t);
    FillThickLine(img, w / 3, h / 2, 2 * w / 3, h / 2, t);

    return img;
}

Image MakeLetterB(int w, int h) {
    Image img(w, h, 0);

    const int t = std::max(2, std::min(w, h) / 22);

    FillRectangle(img, w / 4 - t, h / 5, w / 4 + t, 4 * h / 5);
    FillEllipse(img, w / 2, h / 3, w / 4, h / 7);
    FillEllipse(img, w / 2, 2 * h / 3, w / 4, h / 7);

    FillRectangle(img, w / 4, h / 5, w / 2, h / 5 + 2 * t);
    FillRectangle(img, w / 4, h / 2 - t, w / 2, h / 2 + t);
    FillRectangle(img, w / 4, 4 * h / 5 - 2 * t, w / 2, 4 * h / 5);

    return img;
}

Image MakeBranching(int w, int h) {
    Image img(w, h, 0);

    const int cx = w / 2;
    const int cy = h / 2;
    const int t = std::max(2, std::min(w, h) / 26);

    FillThickLine(img, cx, h / 5, cx, cy, t);
    FillThickLine(img, cx, cy, w / 4, 4 * h / 5, t);
    FillThickLine(img, cx, cy, 3 * w / 4, 4 * h / 5, t);
    FillCircle(img, cx, cy, 2 * t);

    return img;
}

Image MakeNoisyBlob(int w, int h) {
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

            const double r = base_radius + perturbation;

            if (dx * dx + dy * dy <= r * r) {
                img(y, x) = 1;
            }
        }
    }

    return img;
}

Image MakeSmall2x2() {
    Image img(32, 32, 0);
    FillRectangle(img, 15, 15, 16, 16);
    return img;
}

Image MakeSmall3x3() {
    Image img(32, 32, 0);
    FillRectangle(img, 15, 15, 17, 17);
    return img;
}

void GenerateSet(int width, int height, const std::string& dir) {
    std::filesystem::create_directories(dir);

    for (int thickness : {2, 4, 8, 12}) {
        WritePgm(MakeLine0(width, height, thickness), dir + "/line_0_t" + std::to_string(thickness) + ".pgm");
        WritePgm(MakeLine45(width, height, thickness), dir + "/line_45_t" + std::to_string(thickness) + ".pgm");
        WritePgm(MakeLine90(width, height, thickness), dir + "/line_90_t" + std::to_string(thickness) + ".pgm");
        WritePgm(MakeLine135(width, height, thickness), dir + "/line_135_t" + std::to_string(thickness) + ".pgm");
    }

    WritePgm(MakeRectangle(width, height, 4), dir + "/rectangle_thin.pgm");
    WritePgm(MakeRectangle(width, height, 12), dir + "/rectangle_thick.pgm");

    WritePgm(MakeDumbbell(width, height), dir + "/dumbbell.pgm");
    WritePgm(MakeCross(width, height), dir + "/cross.pgm");
    WritePgm(MakeBranching(width, height), dir + "/branching.pgm");
    WritePgm(MakeNoisyBlob(width, height), dir + "/noisy_blob.pgm");

    WritePgm(MakeLetterA(width, height), dir + "/letter_a.pgm");
    WritePgm(MakeLetterB(width, height), dir + "/letter_b.pgm");
    WritePgm(MakeLetterH(width, height), dir + "/letter_h.pgm");

    WritePgm(MakeSmall2x2(), dir + "/small_2x2.pgm");
    WritePgm(MakeSmall3x3(), dir + "/small_3x3.pgm");
}

int main(int argc, char** argv) {
    try {
        int width = 256;
        int height = 256;
        std::string dir = "thinning_tests";

        if (argc == 2) {
            dir = argv[1];
        } else if (argc == 4) {
            width = std::stoi(argv[1]);
            height = std::stoi(argv[2]);
            dir = argv[3];
        } else if (argc != 1) {
            std::cerr << "Usage:\n";
            std::cerr << "  " << argv[0] << "\n";
            std::cerr << "  " << argv[0] << " <output_dir>\n";
            std::cerr << "  " << argv[0] << " <width> <height> <output_dir>\n";
            return 1;
        }

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Width and height must be positive");
        }

        GenerateSet(width, height, dir);

        std::cout << "Generated thinning test images in directory: " << dir << "\n";
        std::cout << "Image size: " << width << "x" << height << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
