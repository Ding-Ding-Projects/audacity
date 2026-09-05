/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QByteArray>
#include <QVector>

namespace au::personalize {
/*!
 * \brief A small, self contained QR code encoder, run entirely in process.
 *
 * There is no network call anywhere in this class: the code is produced
 * from the bytes it is given and nothing else. It supports byte mode only,
 * at error correction level L, and picks the smallest of QR versions 1
 * through 10 that fits the given payload. That is enough for the
 * otpauth:// pairing URIs this module produces; it is not a general purpose
 * QR library.
 */
class QrEncoder
{
public:
    struct Result {
        bool ok = false;
        int size = 0;
        //! Row major matrix of modules: true is a dark module.
        QVector<bool> modules;

        bool moduleAt(int x, int y) const
        {
            if (x < 0 || y < 0 || x >= size || y >= size) {
                return false;
            }
            return modules.at(y * size + x);
        }
    };

    static Result encode(const QByteArray& text);
};
}
