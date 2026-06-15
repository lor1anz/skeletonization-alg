// thin_benchmark.cpp

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
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

struct Result {
    Image skeleton;
    int iterations = 0;
    double time_ms = 0.0;
};

bool Inside(const Image& img, int y, int x) {
    return 0 <= y && y < img.height && 0 <= x && x < img.width;
}

std::string ReadToken(std::istream& in) {
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
        throw std::runtime_error("Cannot open file: " + path);
    }

    const std::string magic = ReadToken(in);
    if (magic != "P2" && magic != "P5") {
        throw std::runtime_error("Only P2 and P5 PGM files are supported");
    }

    const int width = std::stoi(ReadToken(in));
    const int height = std::stoi(ReadToken(in));
    const int max_value = std::stoi(ReadToken(in));

    if (width <= 0 || height <= 0 || max_value <= 0 || max_value > 255) {
        throw std::runtime_error("Invalid PGM header");
    }

    Image img(width, height, 0);

    if (magic == "P2") {
        for (int i = 0; i < width * height; ++i) {
            const int value = std::stoi(ReadToken(in));
            img.data[static_cast<size_t>(i)] = value > 0 ? 1 : 0;
        }
    } else {
        in.get();
        std::vector<uint8_t> raw(static_cast<size_t>(width * height));
        in.read(reinterpret_cast<char*>(raw.data()), raw.size());

        if (!in) {
            throw std::runtime_error("Cannot read PGM data");
        }

        for (int i = 0; i < width * height; ++i) {
            img.data[static_cast<size_t>(i)] = raw[static_cast<size_t>(i)] > 0 ? 1 : 0;
        }
    }

    return img;
}

void WritePgm(const Image& img, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot write file: " + path);
    }

    out << "P5\n";
    out << img.width << " " << img.height << "\n";
    out << "255\n";

    for (uint8_t v : img.data) {
        const uint8_t out_v = v ? 255 : 0;
        out.write(reinterpret_cast<const char*>(&out_v), 1);
    }
}

std::string Stem(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);

    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) {
        name = name.substr(0, dot);
    }

    return name;
}

int CountForeground(const Image& img) {
    int count = 0;

    for (uint8_t v : img.data) {
        if (v != 0) {
            ++count;
        }
    }

    return count;
}

int CountComponents8(const Image& img) {
    std::vector<uint8_t> visited(img.data.size(), 0);
    int components = 0;

    const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const size_t idx = static_cast<size_t>(y * img.width + x);

            if (img(y, x) == 0 || visited[idx]) {
                continue;
            }

            ++components;

            std::queue<std::pair<int, int>> q;
            q.push({y, x});
            visited[idx] = 1;

            while (!q.empty()) {
                const auto [cy, cx] = q.front();
                q.pop();

                for (int k = 0; k < 8; ++k) {
                    const int ny = cy + dy[k];
                    const int nx = cx + dx[k];

                    if (!Inside(img, ny, nx)) {
                        continue;
                    }

                    const size_t nidx = static_cast<size_t>(ny * img.width + nx);

                    if (img(ny, nx) != 0 && !visited[nidx]) {
                        visited[nidx] = 1;
                        q.push({ny, nx});
                    }
                }
            }
        }
    }

    return components;
}

int P(const Image& img, int y, int x) {
    if (!Inside(img, y, x)) {
        return 0;
    }

    return img(y, x) ? 1 : 0;
}

int ZhangB(const Image& img, int y, int x) {
    return P(img, y - 1, x) +     // P2
           P(img, y - 1, x + 1) + // P3
           P(img, y, x + 1) +     // P4
           P(img, y + 1, x + 1) + // P5
           P(img, y + 1, x) +     // P6
           P(img, y + 1, x - 1) + // P7
           P(img, y, x - 1) +     // P8
           P(img, y - 1, x - 1);  // P9
}

int ZhangA(const Image& img, int y, int x) {
    const int p2 = P(img, y - 1, x);
    const int p3 = P(img, y - 1, x + 1);
    const int p4 = P(img, y, x + 1);
    const int p5 = P(img, y + 1, x + 1);
    const int p6 = P(img, y + 1, x);
    const int p7 = P(img, y + 1, x - 1);
    const int p8 = P(img, y, x - 1);
    const int p9 = P(img, y - 1, x - 1);

    return (!p2 && p3) +
           (!p3 && p4) +
           (!p4 && p5) +
           (!p5 && p6) +
           (!p6 && p7) +
           (!p7 && p8) +
           (!p8 && p9) +
           (!p9 && p2);
}

bool ZhangShouldDelete(const Image& img, int y, int x, int subiteration) {
    if (img(y, x) == 0) {
        return false;
    }

    const int p2 = P(img, y - 1, x);
    const int p4 = P(img, y, x + 1);
    const int p6 = P(img, y + 1, x);
    const int p8 = P(img, y, x - 1);

    const int b = ZhangB(img, y, x);
    const int a = ZhangA(img, y, x);

    if (!(2 <= b && b <= 6)) {
        return false;
    }

    if (a != 1) {
        return false;
    }

    if (subiteration == 0) {
        return p2 * p4 * p6 == 0 &&
               p4 * p6 * p8 == 0;
    }

    return p2 * p4 * p8 == 0 &&
           p2 * p6 * p8 == 0;
}

Result RunZhangSuen(const Image& input) {
    Image img = input;
    int iterations = 0;

    auto start = std::chrono::steady_clock::now();

    bool changed = true;

    while (changed) {
        changed = false;
        ++iterations;

        for (int sub = 0; sub < 2; ++sub) {
            std::vector<uint8_t> mark(img.data.size(), 0);

            for (int y = 0; y < img.height; ++y) {
                for (int x = 0; x < img.width; ++x) {
                    if (ZhangShouldDelete(img, y, x, sub)) {
                        mark[static_cast<size_t>(y * img.width + x)] = 1;
                    }
                }
            }

            for (size_t i = 0; i < img.data.size(); ++i) {
                if (mark[i]) {
                    img.data[i] = 0;
                    changed = true;
                }
            }
        }
    }

    auto end = std::chrono::steady_clock::now();

    Result result;
    result.skeleton = std::move(img);
    result.iterations = iterations;
    result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

int GuoN(const Image& img, int y, int x) {
    const int p1 = P(img, y - 1, x);     // north
    const int p2 = P(img, y - 1, x + 1); // north-east
    const int p3 = P(img, y, x + 1);     // east
    const int p4 = P(img, y + 1, x + 1); // south-east
    const int p5 = P(img, y + 1, x);     // south
    const int p6 = P(img, y + 1, x - 1); // south-west
    const int p7 = P(img, y, x - 1);     // west
    const int p8 = P(img, y - 1, x - 1); // north-west

    const int n1 = (p1 || p2) + (p3 || p4) + (p5 || p6) + (p7 || p8);
    const int n2 = (p2 || p3) + (p4 || p5) + (p6 || p7) + (p8 || p1);

    return std::min(n1, n2);
}

int GuoC(const Image& img, int y, int x) {
    const int p1 = P(img, y - 1, x);     // north
    const int p2 = P(img, y - 1, x + 1); // north-east
    const int p3 = P(img, y, x + 1);     // east
    const int p4 = P(img, y + 1, x + 1); // south-east
    const int p5 = P(img, y + 1, x);     // south
    const int p6 = P(img, y + 1, x - 1); // south-west
    const int p7 = P(img, y, x - 1);     // west
    const int p8 = P(img, y - 1, x - 1); // north-west

    return ((!p1) && (p2 || p3)) +
           ((!p3) && (p4 || p5)) +
           ((!p5) && (p6 || p7)) +
           ((!p7) && (p8 || p1));
}

bool GuoShouldDelete(const Image& img, int y, int x, int subiteration) {
    if (img(y, x) == 0) {
        return false;
    }

    const int p1 = P(img, y - 1, x);     // north
    const int p3 = P(img, y, x + 1);     // east
    const int p5 = P(img, y + 1, x);     // south
    const int p7 = P(img, y, x - 1);     // west

    if (GuoC(img, y, x) != 1) {
        return false;
    }

    const int n = GuoN(img, y, x);

    if (!(2 <= n && n <= 3)) {
        return false;
    }

    if (subiteration == 0) {
        return (p1 || p3 || !p5) && p7 == 0;
    }

    return (p5 || p7 || !p1) && p3 == 0;
}

Result RunGuoHall(const Image& input) {
    Image img = input;
    int iterations = 0;

    auto start = std::chrono::steady_clock::now();

    bool changed = true;

    while (changed) {
        changed = false;
        ++iterations;

        for (int sub = 0; sub < 2; ++sub) {
            std::vector<uint8_t> mark(img.data.size(), 0);

            for (int y = 0; y < img.height; ++y) {
                for (int x = 0; x < img.width; ++x) {
                    if (GuoShouldDelete(img, y, x, sub)) {
                        mark[static_cast<size_t>(y * img.width + x)] = 1;
                    }
                }
            }

            for (size_t i = 0; i < img.data.size(); ++i) {
                if (mark[i]) {
                    img.data[i] = 0;
                    changed = true;
                }
            }
        }
    }

    auto end = std::chrono::steady_clock::now();

    Result result;
    result.skeleton = std::move(img);
    result.iterations = iterations;
    result.time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

void PrintCsvHeader() {
    std::cout << "algorithm,input,width,height,input_pixels,skeleton_pixels,"
                 "iterations,time_ms,components_before,components_after,"
                 "connectivity_preserved\n";
}

void PrintCsvRow(
    const std::string& algorithm,
    const std::string& input_name,
    const Image& input,
    const Result& result
) {
    const int input_pixels = CountForeground(input);
    const int skeleton_pixels = CountForeground(result.skeleton);
    const int components_before = CountComponents8(input);
    const int components_after = CountComponents8(result.skeleton);
    const bool connectivity_preserved = components_before == components_after;

    std::cout << algorithm << ","
              << input_name << ","
              << input.width << ","
              << input.height << ","
              << input_pixels << ","
              << skeleton_pixels << ","
              << result.iterations << ","
              << result.time_ms << ","
              << components_before << ","
              << components_after << ","
              << (connectivity_preserved ? "yes" : "no")
              << "\n";
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage:\n";
            std::cerr << "  " << argv[0] << " input.pgm\n";
            return 1;
        }

        const std::string input_path = argv[1];
        const std::string stem = Stem(input_path);

        const Image input = ReadPgm(input_path);

        const Result zhang = RunZhangSuen(input);
        const Result guo = RunGuoHall(input);

        WritePgm(zhang.skeleton, stem + "_zhang_suen.pgm");
        WritePgm(guo.skeleton, stem + "_guo_hall.pgm");

        PrintCsvHeader();
        PrintCsvRow("ZhangSuen", input_path, input, zhang);
        PrintCsvRow("GuoHall", input_path, input, guo);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
