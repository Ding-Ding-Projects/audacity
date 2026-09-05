/*
* Audacity: A Digital Audio Editor
*/

#include "qrencoder.h"

#include <algorithm>

using namespace au::personalize;

namespace {
struct VersionInfo {
    int version;
    int totalDataCodewords;
    int eccCodewordsPerBlock;
};

// Byte mode, error correction level L, single error correction block.
// Versions beyond 5 would split into more than one block at level L, which
// this small encoder does not implement, so the caller is asked to keep the
// payload within version 5's data capacity instead.
const VersionInfo VERSIONS[] = {
    { 1, 19, 7 },
    { 2, 34, 10 },
    { 3, 55, 15 },
    { 4, 80, 20 },
    { 5, 108, 26 },
};

int moduleSize(int version)
{
    return 4 * version + 17;
}

// GF(256) with the QR primitive polynomial x^8 + x^4 + x^3 + x^2 + 1 (0x11D).
struct GaloisField {
    unsigned char expTable[512] = { 0 };
    unsigned char logTable[256] = { 0 };

    GaloisField()
    {
        int x = 1;
        for (int i = 0; i < 255; ++i) {
            expTable[i] = static_cast<unsigned char>(x);
            logTable[x] = static_cast<unsigned char>(i);
            x <<= 1;
            if (x & 0x100) {
                x ^= 0x11D;
            }
        }
        for (int i = 255; i < 512; ++i) {
            expTable[i] = expTable[i - 255];
        }
    }

    unsigned char multiply(unsigned char a, unsigned char b) const
    {
        if (a == 0 || b == 0) {
            return 0;
        }
        return expTable[logTable[a] + logTable[b]];
    }
};

const GaloisField GF;

QByteArray reedSolomonEcc(const QByteArray& data, int eccLength)
{
    // Build the generator polynomial (x - a^0)(x - a^1)...(x - a^(eccLength-1)).
    QVector<unsigned char> generator(1, 1);
    for (int i = 0; i < eccLength; ++i) {
        QVector<unsigned char> next(generator.size() + 1, 0);
        unsigned char root = GF.expTable[i];
        for (int j = 0; j < generator.size(); ++j) {
            next[j] = static_cast<unsigned char>(next[j] ^ GF.multiply(generator[j], root));
            next[j + 1] = static_cast<unsigned char>(next[j + 1] ^ generator[j]);
        }
        generator = next;
    }
    // The loop above builds the coefficients from constant term outward;
    // the division below expects them from the leading term outward.
    std::reverse(generator.begin(), generator.end());

    QVector<unsigned char> remainder(eccLength, 0);
    for (unsigned char byte : data) {
        unsigned char factor = static_cast<unsigned char>(byte ^ remainder[0]);
        remainder.remove(0);
        remainder.append(0);
        if (factor != 0) {
            for (int j = 0; j < eccLength; ++j) {
                remainder[j] = static_cast<unsigned char>(remainder[j] ^ GF.multiply(generator[j + 1], factor));
            }
        }
    }

    QByteArray result;
    for (unsigned char b : remainder) {
        result.append(static_cast<char>(b));
    }
    return result;
}

class BitWriter
{
public:
    void writeBits(quint32 value, int numBits)
    {
        for (int i = numBits - 1; i >= 0; --i) {
            m_bits.append((value >> i) & 1);
        }
    }

    QByteArray toBytes(int totalBytes) const
    {
        QByteArray out(totalBytes, '\0');
        for (int i = 0; i < m_bits.size(); ++i) {
            if (m_bits.at(i)) {
                out[i / 8] = static_cast<char>(out.at(i / 8) | (1 << (7 - (i % 8))));
            }
        }
        return out;
    }

    int bitCount() const { return m_bits.size(); }

private:
    QVector<bool> m_bits;
};

class Matrix
{
public:
    explicit Matrix(int size)
        : m_size(size)
        , m_dark(size * size, false)
        , m_reserved(size * size, false)
    {
    }

    int size() const { return m_size; }

    void set(int x, int y, bool dark)
    {
        if (x < 0 || y < 0 || x >= m_size || y >= m_size) {
            return;
        }
        m_dark[y * m_size + x] = dark;
        m_reserved[y * m_size + x] = true;
    }

    bool isReserved(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= m_size || y >= m_size) {
            return true;
        }
        return m_reserved[y * m_size + x];
    }

    bool isDark(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= m_size || y >= m_size) {
            return false;
        }
        return m_dark[y * m_size + x];
    }

    void setDataBit(int x, int y, bool dark)
    {
        m_dark[y * m_size + x] = dark;
    }

    QVector<bool> modules() const { return m_dark; }

private:
    int m_size;
    QVector<bool> m_dark;
    QVector<bool> m_reserved;
};

void drawFinderPattern(Matrix& matrix, int left, int top)
{
    for (int dy = -1; dy <= 7; ++dy) {
        for (int dx = -1; dx <= 7; ++dx) {
            int x = left + dx;
            int y = top + dy;
            if (x < 0 || y < 0 || x >= matrix.size() || y >= matrix.size()) {
                continue;
            }
            bool dark = false;
            if (dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6) {
                int ring = std::min(std::min(dx, 6 - dx), std::min(dy, 6 - dy));
                dark = (ring == 0) || (ring == 2);
            } else {
                dark = false; // the one module wide separator
            }
            matrix.set(x, y, dark);
        }
    }
}

void drawAlignmentPattern(Matrix& matrix, int centerX, int centerY)
{
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            int ring = std::max(std::abs(dx), std::abs(dy));
            bool dark = (ring != 1);
            matrix.set(centerX + dx, centerY + dy, dark);
        }
    }
}

QVector<int> alignmentCenters(int version)
{
    static const QVector<QVector<int> > TABLE = {
        {},                  // version 1: no alignment pattern
        { 6, 18 },           // version 2
        { 6, 22 },           // version 3
        { 6, 26 },           // version 4
        { 6, 30 },           // version 5
    };
    if (version < 1 || version > TABLE.size()) {
        return {};
    }
    return TABLE.at(version - 1);
}

void reserveFormatAreas(Matrix& matrix)
{
    int size = matrix.size();
    for (int i = 0; i <= 8; ++i) {
        if (i == 6) {
            // Column 6 and row 6 belong to the timing pattern, not to the
            // format information strip that surrounds the top left finder.
            continue;
        }
        matrix.set(8, i, false);
        matrix.set(i, 8, false);
    }
    for (int i = 0; i < 8; ++i) {
        matrix.set(size - 1 - i, 8, false);
        matrix.set(8, size - 1 - i, false);
    }
    // The dark module, always black, one below the bottom left finder pattern.
    matrix.set(8, size - 8, true);
}

void drawTimingPatterns(Matrix& matrix)
{
    int size = matrix.size();
    for (int i = 8; i < size - 8; ++i) {
        bool dark = (i % 2) == 0;
        matrix.set(i, 6, dark);
        matrix.set(6, i, dark);
    }
}

void applyFormatInfo(Matrix& matrix, int maskPattern)
{
    // Error correction level bits for level L, per the QR specification.
    const int eccLevelBits = 0b01;
    int data = (eccLevelBits << 3) | maskPattern;

    int value = data << 10;
    for (int i = 4; i >= 0; --i) {
        if ((value >> (i + 10)) & 1) {
            value ^= (0x537 << i);
        }
    }
    int format = ((data << 10) | value) ^ 0x5412;
    auto bitAt = [format](int i) { return ((format >> i) & 1) != 0; };

    // First copy, hugging the top left finder pattern.
    for (int i = 0; i <= 5; ++i) {
        matrix.set(8, i, bitAt(i));
    }
    matrix.set(8, 7, bitAt(6));
    matrix.set(8, 8, bitAt(7));
    matrix.set(7, 8, bitAt(8));
    for (int i = 9; i < 15; ++i) {
        matrix.set(14 - i, 8, bitAt(i));
    }

    // Second copy, split between the top right finder and the bottom left one.
    int size = matrix.size();
    for (int i = 0; i <= 7; ++i) {
        matrix.set(size - 1 - i, 8, bitAt(i));
    }
    for (int i = 8; i < 15; ++i) {
        matrix.set(8, size - 15 + i, bitAt(i));
    }
}

void placeData(Matrix& matrix, const QByteArray& codewords, int maskPattern)
{
    QVector<bool> bits;
    for (unsigned char byte : codewords) {
        for (int i = 7; i >= 0; --i) {
            bits.append((byte >> i) & 1);
        }
    }

    int size = matrix.size();
    int bitIndex = 0;
    bool goingUp = true;

    for (int right = size - 1; right > 0; right -= 2) {
        if (right == 6) {
            right = 5;
        }
        for (int rowStep = 0; rowStep < size; ++rowStep) {
            int y = goingUp ? (size - 1 - rowStep) : rowStep;
            for (int columnOffset = 0; columnOffset < 2; ++columnOffset) {
                int x = right - columnOffset;
                if (matrix.isReserved(x, y)) {
                    continue;
                }
                bool bit = bitIndex < bits.size() ? bits.at(bitIndex) : false;
                ++bitIndex;

                bool maskInvert = ((x + y) % 2) == 0;
                if (maskPattern == 0 && maskInvert) {
                    bit = !bit;
                }
                matrix.setDataBit(x, y, bit);
            }
        }
        goingUp = !goingUp;
    }
}
} // namespace

QrEncoder::Result QrEncoder::encode(const QByteArray& text)
{
    Result result;

    const VersionInfo* chosen = nullptr;
    for (const VersionInfo& info : VERSIONS) {
        // Mode indicator (4 bits) + 8 bit character count + one payload byte
        // per data byte must fit, with room for at least the terminator.
        int neededBits = 4 + 8 + text.size() * 8;
        int capacityBits = info.totalDataCodewords * 8;
        if (neededBits <= capacityBits) {
            chosen = &info;
            break;
        }
    }
    if (!chosen) {
        return result;
    }

    BitWriter writer;
    writer.writeBits(0b0100, 4); // byte mode
    writer.writeBits(static_cast<quint32>(text.size()), 8);
    for (unsigned char byte : text) {
        writer.writeBits(byte, 8);
    }

    int capacityBits = chosen->totalDataCodewords * 8;
    int terminatorBits = std::min(4, capacityBits - writer.bitCount());
    if (terminatorBits > 0) {
        writer.writeBits(0, terminatorBits);
    }

    QByteArray dataCodewords = writer.toBytes(chosen->totalDataCodewords);

    // Pad with the standard alternating pad bytes until the block is full.
    bool usePadA = true;
    for (int i = (writer.bitCount() + 7) / 8; i < chosen->totalDataCodewords; ++i) {
        dataCodewords[i] = usePadA ? static_cast<char>(0xEC) : static_cast<char>(0x11);
        usePadA = !usePadA;
    }

    QByteArray ecc = reedSolomonEcc(dataCodewords, chosen->eccCodewordsPerBlock);
    QByteArray allCodewords = dataCodewords + ecc;

    int size = moduleSize(chosen->version);
    Matrix matrix(size);

    drawFinderPattern(matrix, 0, 0);
    drawFinderPattern(matrix, size - 7, 0);
    drawFinderPattern(matrix, 0, size - 7);
    drawTimingPatterns(matrix);
    reserveFormatAreas(matrix);

    QVector<int> centers = alignmentCenters(chosen->version);
    for (int cy : centers) {
        for (int cx : centers) {
            bool nearFinder = (cx <= 8 && cy <= 8)
                              || (cx >= size - 9 && cy <= 8)
                              || (cx <= 8 && cy >= size - 9);
            if (nearFinder) {
                continue;
            }
            drawAlignmentPattern(matrix, cx, cy);
        }
    }

    const int maskPattern = 0;
    placeData(matrix, allCodewords, maskPattern);
    applyFormatInfo(matrix, maskPattern);

    result.ok = true;
    result.size = size;
    result.modules = matrix.modules();
    return result;
}
