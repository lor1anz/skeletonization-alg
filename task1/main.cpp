#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data;

    Image() = default;

    Image(int w, int h, uint8_t value = 0)
        : width(w), height(h), data(static_cast<size_t>(w * h), value) {}

    uint8_t& operator()(int y, int x) {
        return data[static_cast<size_t>(y * width + x)];
    }

    uint8_t operator()(int y, int x) const {
        return data[static_cast<size_t>(y * width + x)];
    }
};

struct DistanceMap {
    int width = 0;
    int height = 0;
    std::vector<int> data;

    DistanceMap(int w, int h, int value)
        : width(w), height(h), data(static_cast<size_t>(w * h), value) {}

    int& operator()(int y, int x) {
        return data[static_cast<size_t>(y * width + x)];
    }

    int operator()(int y, int x) const {
        return data[static_cast<size_t>(y * width + x)];
    }
};

static bool Inside(const Image& img, int y, int x) {
    return 0 <= y && y < img.height && 0 <= x && x < img.width;
}

static std::string ReadToken(std::istream& in) {
    std::string token;
    while (in >> token) {
        if (!token.empty() && token[0] == '#') {
            std::string dummy;
            std::getline(in, dummy);
            continue;
        }
        return token;
    }
    throw std::runtime_error("Unexpected end of file");
}

Image ReadPgm(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open input file: " + path);
    }

    const std::string magic = ReadToken(in);
    if (magic != "P2" && magic != "P5") {
        throw std::runtime_error("Only P2/P5 PGM files are supported");
    }

    const int width = std::stoi(ReadToken(in));
    const int height = std::stoi(ReadToken(in));
    const int max_value = std::stoi(ReadToken(in));

    if (width <= 0 || height <= 0 || max_value <= 0 || max_value > 255) {
        throw std::runtime_error("Invalid PGM header");
    }

    Image img(width, height);

    if (magic == "P2") {
        for (int i = 0; i < width * height; ++i) {
            const int value = std::stoi(ReadToken(in));
            img.data[static_cast<size_t>(i)] = static_cast<uint8_t>(value);
        }
    } else {
        in.get(); // consume one whitespace character after header
        in.read(reinterpret_cast<char*>(img.data.data()), img.data.size());
        if (!in) {
            throw std::runtime_error("Cannot read binary PGM data");
        }
    }

    return img;
}

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

Image Binarize(const Image& img, uint8_t threshold) {
    Image binary(img.width, img.height);

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            binary(y, x) = img(y, x) >= threshold ? 255 : 0;
        }
    }

    return binary;
}

Image ExtractBoundary(const Image& binary) {
    Image boundary(binary.width, binary.height, 0);

    const int dy[4] = {-1, 1, 0, 0};
    const int dx[4] = {0, 0, -1, 1};

    for (int y = 0; y < binary.height; ++y) {
        for (int x = 0; x < binary.width; ++x) {
            if (binary(y, x) == 0) {
                continue;
            }

            bool is_boundary = false;

            for (int k = 0; k < 4; ++k) {
                const int ny = y + dy[k];
                const int nx = x + dx[k];

                if (!Inside(binary, ny, nx) || binary(ny, nx) == 0) {
                    is_boundary = true;
                    break;
                }
            }

            if (is_boundary) {
                boundary(y, x) = 255;
            }
        }
    }

    return boundary;
}

DistanceMap ComputeDistanceTransform(const Image& binary, const Image& boundary) {
    constexpr int kInf = std::numeric_limits<int>::max() / 4;

    DistanceMap dist(binary.width, binary.height, kInf);
    std::queue<std::pair<int, int>> q;

    for (int y = 0; y < binary.height; ++y) {
        for (int x = 0; x < binary.width; ++x) {
            if (boundary(y, x) != 0) {
                dist(y, x) = 0;
                q.push({y, x});
            }

            if (binary(y, x) == 0) {
                dist(y, x) = 0;
            }
        }
    }

    const int dy[4] = {-1, 1, 0, 0};
    const int dx[4] = {0, 0, -1, 1};

    while (!q.empty()) {
        const auto [y, x] = q.front();
        q.pop();

        for (int k = 0; k < 4; ++k) {
            const int ny = y + dy[k];
            const int nx = x + dx[k];

            if (!Inside(binary, ny, nx)) {
                continue;
            }

            if (binary(ny, nx) == 0) {
                continue;
            }

            if (dist(ny, nx) > dist(y, x) + 1) {
                dist(ny, nx) = dist(y, x) + 1;
                q.push({ny, nx});
            }
        }
    }

    return dist;
}

Image DistanceToImage(const DistanceMap& dist) {
    int max_dist = 0;

    for (int value : dist.data) {
        if (value < std::numeric_limits<int>::max() / 8) {
            max_dist = std::max(max_dist, value);
        }
    }

    Image out(dist.width, dist.height, 0);

    if (max_dist == 0) {
        return out;
    }

    for (int y = 0; y < dist.height; ++y) {
        for (int x = 0; x < dist.width; ++x) {
            const int d = dist(y, x);
            out(y, x) = static_cast<uint8_t>(255 * d / max_dist);
        }
    }

    return out;
}

Image ExtractSkeletonLocalMaxima(const Image& binary, const DistanceMap& dist) {
    Image skeleton(binary.width, binary.height, 0);

    const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int y = 0; y < binary.height; ++y) {
        for (int x = 0; x < binary.width; ++x) {
            if (binary(y, x) == 0) {
                continue;
            }

            if (dist(y, x) <= 0) {
                continue;
            }

            bool is_local_maximum = true;

            for (int k = 0; k < 8; ++k) {
                const int ny = y + dy[k];
                const int nx = x + dx[k];

                if (!Inside(binary, ny, nx)) {
                    continue;
                }

                if (binary(ny, nx) != 0 && dist(ny, nx) > dist(y, x)) {
                    is_local_maximum = false;
                    break;
                }
            }

            if (is_local_maximum) {
                skeleton(y, x) = 255;
            }
        }
    }

    return skeleton;
}

Image MakeDemoImage(int width, int height) {
    Image img(width, height, 0);

    const int cx = width / 2;
    const int cy = height / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;

            const bool ellipse =
                (dx * dx) * 4 + (dy * dy) * 9 < width * height / 3;

            const bool vertical_bar =
                std::abs(x - cx) < width / 14 &&
                std::abs(y - cy) < height / 3;

            const bool horizontal_bar =
                std::abs(y - cy) < height / 14 &&
                std::abs(x - cx) < width / 3;

            if (ellipse || vertical_bar || horizontal_bar) {
                img(y, x) = 255;
            }
        }
    }

    return img;
}

int main(int argc, char** argv) {
    try {
        Image input;

        if (argc >= 2) {
            input = ReadPgm(argv[1]);
            input = Binarize(input, 128);
        } else {
            input = MakeDemoImage(256, 256);
        }

        const Image boundary = ExtractBoundary(input);
        const DistanceMap dist = ComputeDistanceTransform(input, boundary);
        const Image dist_image = DistanceToImage(dist);
        const Image skeleton = ExtractSkeletonLocalMaxima(input, dist);

        WritePgm(input, "01_binary.pgm");
        WritePgm(boundary, "02_boundary.pgm");
        WritePgm(dist_image, "03_distance.pgm");
        WritePgm(skeleton, "04_skeleton.pgm");

        std::cout << "Saved:\n";
        std::cout << "  01_binary.pgm\n";
        std::cout << "  02_boundary.pgm\n";
        std::cout << "  03_distance.pgm\n";
        std::cout << "  04_skeleton.pgm\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
