#include <QtTest/QtTest>
#include "Utils/UnicodeUtil.h"

class UnicodeUtilTest : public QObject {
    Q_OBJECT

private slots:
    void testAsciiWidth();
    void testCjkWidth();
    void testEmojiWidth();
    void testCombiningChars();
    void testZeroWidthChars();
    void testSurrogatePairs();
    void testStringWidth();
};

void UnicodeUtilTest::testAsciiWidth() {
    QCOMPARE(UnicodeUtil::charDisplayWidth('A'), 1);
    QCOMPARE(UnicodeUtil::charDisplayWidth('z'), 1);
    QCOMPARE(UnicodeUtil::charDisplayWidth('0'), 1);
    QCOMPARE(UnicodeUtil::charDisplayWidth(' '), 1);
}

void UnicodeUtilTest::testCjkWidth() {
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x4E00), 2);
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x4E2D), 2);
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x9FFF), 2);
}

void UnicodeUtilTest::testEmojiWidth() {
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x1F600), 2);
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x1F602), 2);
    QVERIFY(UnicodeUtil::isEmoji(0x1F600));
    QVERIFY(UnicodeUtil::isEmoji(0x2764));
}

void UnicodeUtilTest::testCombiningChars() {
    QVERIFY(UnicodeUtil::isCombiningCharacter(0x0301));
    QVERIFY(UnicodeUtil::isCombiningCharacter(0x0308));
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x0301), 0);
}

void UnicodeUtilTest::testZeroWidthChars() {
    QVERIFY(UnicodeUtil::isZeroWidth(0x200B));
    QVERIFY(UnicodeUtil::isZeroWidth(0x200C));
    QVERIFY(UnicodeUtil::isZeroWidth(0x200D));
    QCOMPARE(UnicodeUtil::charDisplayWidth(0x200B), 0);
}

void UnicodeUtilTest::testSurrogatePairs() {
    QString emoji = QString::fromUtf8("\xF0\x9F\x98\x80");
    QVector<UnicodeChar> chars = UnicodeUtil::parseString(emoji);
    QCOMPARE(chars.size(), 1);
    QCOMPARE(chars[0].codepoint, quint32(0x1F600));
    QVERIFY(chars[0].isEmoji);
}

void UnicodeUtilTest::testStringWidth() {
    QCOMPARE(UnicodeUtil::stringDisplayWidth(QString("Hello")), 5);
    QCOMPARE(UnicodeUtil::stringDisplayWidth(QString("你好")), 4);
    QCOMPARE(UnicodeUtil::stringDisplayWidth(QString("Hello 世界")), 10);
}

QTEST_MAIN(UnicodeUtilTest)
#include "UnicodeUtilTest.moc"
