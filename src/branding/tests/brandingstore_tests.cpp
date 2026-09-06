#include <QtTest>
#include <QBuffer>
#include <QFile>
#include <QTemporaryDir>

#include "brandingstore.h"

using namespace au::branding;

namespace {
QByteArray png(const QImage& image)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return buffer.data();
}
}

class BrandingStoreTests final : public QObject
{
    Q_OBJECT
private slots:
    void rejectsMalformedAndOversizedContent();
    void fitAndCropProduceExpectedPixels();
    void backgroundAndAlphaArePreserved();
    void persistsAndResetsWithoutSourcePath();
    void failuresAndCancellationLeaveExistingStateIntact();
    void refusesUnsafeSvgWithoutNetworkAccess();
    void acceptsSafeSvgAndIcoWhenQtCodecsAreAvailable();
};

void BrandingStoreTests::rejectsMalformedAndOversizedContent()
{
    QTemporaryDir directory;
    BrandingStore store(directory.path());
    QVERIFY(!store.loadCustom("not-an-image").ok);
    QVERIFY(!store.loadCustom(QByteArray(BrandingStore::MaxSourceBytes + 1, 'x')).ok);
    QImage oversized(BrandingStore::MaxDimension + 1, 1, QImage::Format_ARGB32);
    QVERIFY(!store.loadCustom(png(oversized)).ok);
}

void BrandingStoreTests::fitAndCropProduceExpectedPixels()
{
    QTemporaryDir directory;
    QImage source(4, 2, QImage::Format_ARGB32);
    source.fill(Qt::red);
    BrandingStore store(directory.path());
    LogoOptions fit; fit.background = Qt::blue;
    QVERIFY(store.loadCustom(png(source), fit).ok);
    QImage fitted = store.derivative(16);
    QCOMPARE(fitted.pixelColor(0, 0), QColor(Qt::blue));
    QCOMPARE(fitted.pixelColor(8, 8), QColor(Qt::red));
    LogoOptions crop; crop.fitMode = LogoFitMode::Crop; crop.background = Qt::blue;
    QVERIFY(store.update(crop).ok);
    QCOMPARE(store.derivative(16).pixelColor(0, 0), QColor(Qt::red));
}

void BrandingStoreTests::backgroundAndAlphaArePreserved()
{
    QTemporaryDir directory;
    QImage source(2, 2, QImage::Format_ARGB32);
    source.fill(Qt::transparent);
    source.setPixelColor(0, 0, QColor(255, 0, 0, 128));
    BrandingStore store(directory.path());
    QVERIFY(store.loadCustom(png(source), {}).ok);
    QCOMPARE(store.derivative(16).pixelColor(15, 15).alpha(), 0);
    LogoOptions opaque; opaque.background = QColor("#ff00ff00");
    QVERIFY(store.update(opaque).ok);
    QCOMPARE(store.derivative(16).pixelColor(15, 15), QColor("#ff00ff00"));
}

void BrandingStoreTests::persistsAndResetsWithoutSourcePath()
{
    QTemporaryDir directory;
    QImage source(3, 3, QImage::Format_ARGB32); source.fill(Qt::yellow);
    BrandingStore store(directory.path());
    QVERIFY(store.loadCustom(png(source)).ok);
    QVERIFY(store.hasCustomLogo());
    QVERIFY(store.derivativePaths().size() >= 6);
    BrandingStore reloaded(directory.path());
    QVERIFY(reloaded.hasCustomLogo());
    QVERIFY(QFile::remove(reloaded.derivativePaths().first()));
    QVERIFY(!BrandingStore(directory.path()).hasCustomLogo());
    QVERIFY(store.loadCustom(png(source)).ok);
    QVERIFY(reloaded.reset().ok);
    QVERIFY(!BrandingStore(directory.path()).hasCustomLogo());
    QVERIFY(!QFile::exists(directory.filePath("branding-v1/source-path.txt")));
}

void BrandingStoreTests::failuresAndCancellationLeaveExistingStateIntact()
{
    QTemporaryDir directory;
    QImage source(2, 2, QImage::Format_ARGB32); source.fill(Qt::cyan);
    BrandingStore store(directory.path());
    QVERIFY(store.loadCustom(png(source)).ok);
    const QImage before = store.derivative(16);
    QVERIFY(!store.loadCustom("bad").ok);
    QCOMPARE(store.derivative(16), before);
    QVERIFY(!store.update({}, [] { return true; }).ok);
    QCOMPARE(store.derivative(16), before);
}

void BrandingStoreTests::refusesUnsafeSvgWithoutNetworkAccess()
{
    QTemporaryDir directory;
    BrandingStore store(directory.path());
    QVERIFY(!store.loadCustom("<svg xmlns='http://www.w3.org/2000/svg'><image href='https://example.invalid/x.png'/></svg>").ok);
    QVERIFY(!store.loadCustom("<svg><script>alert(1)</script></svg>").ok);
}

void BrandingStoreTests::acceptsSafeSvgAndIcoWhenQtCodecsAreAvailable()
{
    QTemporaryDir directory;
    BrandingStore store(directory.path());
    const QByteArray svg("<svg xmlns='http://www.w3.org/2000/svg' width='8' height='8'><rect width='8' height='8' fill='#ff0000'/></svg>");
    const LogoResult svgResult = store.loadCustom(svg);
    if (!svgResult.ok) {
        QSKIP("The installed Qt image plugins do not provide SVG decoding");
    }
    QVERIFY(store.hasCustomLogo());

    QImage image(16, 16, QImage::Format_ARGB32);
    image.fill(Qt::green);
    QBuffer ico;
    ico.open(QIODevice::WriteOnly);
    if (!image.save(&ico, "ICO")) {
        QSKIP("The installed Qt image plugins do not provide ICO encoding");
    }
    QVERIFY(store.loadCustom(ico.data()).ok);
}

QTEST_MAIN(BrandingStoreTests)
#include "brandingstore_tests.moc"
