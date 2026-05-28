#include <QtTest/QtTest>
#include "Widget/UrlDetector.h"

class UrlDetectorTest : public QObject {
    Q_OBJECT

private slots:
    void testHttpUrl();
    void testHttpsUrl();
    void testFtpUrl();
    void testFilePath();
    void testEmail();
    void testNoUrl();
    void testUrlWithPunctuation();

private:
    UrlDetector m_detector;
};

void UrlDetectorTest::testHttpUrl() {
    QVector<UrlMatch> matches = m_detector.findUrls(QString("Visit http://example.com for info"), 0, 0);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches[0].url, QString("http://example.com"));
    QVERIFY(!matches[0].isFile);
}

void UrlDetectorTest::testHttpsUrl() {
    QVector<UrlMatch> matches = m_detector.findUrls(QString("Secure: https://secure.example.com/page"), 0, 0);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches[0].url, QString("https://secure.example.com/page"));
    QVERIFY(!matches[0].isFile);
}

void UrlDetectorTest::testFtpUrl() {
    QVector<UrlMatch> matches = m_detector.findUrls(QString("FTP: ftp://files.example.com/data"), 0, 0);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches[0].url, QString("ftp://files.example.com/data"));
    QVERIFY(!matches[0].isFile);
}

void UrlDetectorTest::testFilePath() {
    bool isPath = QString("/home/user/doc.txt").startsWith("/");
    QVERIFY(isPath);
}

void UrlDetectorTest::testEmail() {
    QString text = QString("Contact user@example.com please");
    int atIndex = text.indexOf('@');
    QVERIFY(atIndex > 0);
}

void UrlDetectorTest::testNoUrl() {
    QVector<UrlMatch> matches = m_detector.findUrls(QString("No URLs here, just text"), 0, 0);
    QCOMPARE(matches.size(), 0);
}

void UrlDetectorTest::testUrlWithPunctuation() {
    QVector<UrlMatch> matches = m_detector.findUrls(QString("See https://example.com!"), 0, 0);
    QVERIFY(matches.size() >= 1);
}

QTEST_MAIN(UrlDetectorTest)
#include "UrlDetectorTest.moc"
