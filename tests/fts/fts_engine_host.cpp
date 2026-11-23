// Host driver for src/fts_engine.cpp, used by tests/fts/test_fts.py.
//
// stdin frames:  'F' u32le(len) bytes  -> feed;   'R' -> reset
// stdout, after each frame: every reply message as u32le(len) bytes, then u32le(0xFFFFFFFF).
#include "fts_engine.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static void put32(uint32_t v)
{
    unsigned char b[4] = {uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16), uint8_t(v >> 24)};
    fwrite(b, 1, 4, stdout);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s ROOT FREE_SPACE MAX_READ\n", argv[0]);
        return 2;
    }
    FtsEngine engine(argv[1], uint32_t(strtoul(argv[2], nullptr, 10)), uint32_t(strtoul(argv[3], nullptr, 10)));
    engine.onMessage = [](const std::string &m) {
        put32(uint32_t(m.size()));
        fwrite(m.data(), 1, m.size(), stdout);
    };
    int op;
    while ((op = getchar()) != EOF) {
        if (op == 'F') {
            unsigned char h[4];
            if (fread(h, 1, 4, stdin) != 4)
                return 1;
            const uint32_t n = h[0] | h[1] << 8 | h[2] << 16 | uint32_t(h[3]) << 24;
            std::string data(n, '\0');
            if (n && fread(&data[0], 1, n, stdin) != n)
                return 1;
            engine.feed(reinterpret_cast<const uint8_t *>(data.data()), data.size());
        } else if (op == 'R') {
            engine.reset();
        } else {
            return 1;
        }
        put32(0xFFFFFFFFu);
        fflush(stdout);
    }
    return 0;
}
