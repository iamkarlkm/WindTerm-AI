#include <QtTest/QtTest>
#include "Buffer/CircularTextBuffer.h"

class CircularTextBufferTest : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase();
    void testAppendAndGetLines();
    void testCircularOverwrite();
    void testGetLastNLines();
    void testClear();
    void testSetCapacity();
    void testFindLineContaining();
    void cleanupTestCase();
    
private:
    CircularTextBuffer* m_buffer;
};

void CircularTextBufferTest::initTestCase() {
    m_buffer = new CircularTextBuffer(100);
}

void CircularTextBufferTest::testAppendAndGetLines() {
    m_buffer->append("Line 1\nLine 2\nLine 3");
    
    QCOMPARE(m_buffer->size(), 3);
    QCOMPARE(m_buffer->lineAt(0).text, QString("Line 1"));
    QCOMPARE(m_buffer->lineAt(1).text, QString("Line 2"));
    QCOMPARE(m_buffer->lineAt(2).text, QString("Line 3"));
    
    QStringList lines = m_buffer->getLines(0, 3);
    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines[0], QString("Line 1"));
    QCOMPARE(lines[1], QString("Line 2"));
    QCOMPARE(lines[2], QString("Line 3"));
}

void CircularTextBufferTest::testCircularOverwrite() {
    CircularTextBuffer smallBuffer(3);
    
    smallBuffer.append("A\nB\nC\nD");
    
    QCOMPARE(smallBuffer.size(), 3);
    QCOMPARE(smallBuffer.lineAt(0).text, QString("B"));
    QCOMPARE(smallBuffer.lineAt(1).text, QString("C"));
    QCOMPARE(smallBuffer.lineAt(2).text, QString("D"));
}

void CircularTextBufferTest::testGetLastNLines() {
    m_buffer->clear();
    m_buffer->append("1\n2\n3\n4\n5");
    
    QString last3 = m_buffer->getLastNLines(3);
    QCOMPARE(last3, QString("3\n4\n5"));
    
    QString last10 = m_buffer->getLastNLines(10);
    QStringList expectedLines = last10.split('\n');
    QCOMPARE(expectedLines.size(), 5);
}

void CircularTextBufferTest::testClear() {
    m_buffer->append("Test line");
    QVERIFY(!m_buffer->isEmpty());
    
    m_buffer->clear();
    QVERIFY(m_buffer->isEmpty());
    QCOMPARE(m_buffer->size(), 0);
}

void CircularTextBufferTest::testSetCapacity() {
    m_buffer->clear();
    m_buffer->append("A\nB\nC\nD\nE");
    
    m_buffer->setCapacity(3);
    
    QCOMPARE(m_buffer->size(), 3);
}

void CircularTextBufferTest::testFindLineContaining() {
    m_buffer->clear();
    m_buffer->append("Hello\nWorld\nTest");
    
    QCOMPARE(m_buffer->findLineContaining(0), 0);
    QCOMPARE(m_buffer->findLineContaining(6), 1);
    QCOMPARE(m_buffer->findLineContaining(12), 2);
}

void CircularTextBufferTest::cleanupTestCase() {
    delete m_buffer;
}

QTEST_MAIN(CircularTextBufferTest)
#include "CircularTextBufferTest.moc"
