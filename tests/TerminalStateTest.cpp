#include <QtTest/QtTest>
#include "Terminal/TerminalState.h"

class TerminalStateTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void testInitialState();
    void testResize();
    void testClear();
    void testScrollRegion();

private:
    TerminalState* m_state;
};

void TerminalStateTest::init() {
    if (m_state) {
        delete m_state;
    }
    m_state = new TerminalState(24, 80);
}

void TerminalStateTest::testInitialState() {
    QCOMPARE(m_state->rows(), 24);
    QCOMPARE(m_state->cols(), 80);
    QCOMPARE(m_state->cursor().row, 0);
    QCOMPARE(m_state->cursor().col, 0);
    QCOMPARE(m_state->scrollbackSize(), 0);
}

void TerminalStateTest::testResize() {
    m_state->write(QString("Hello, World!"));
    m_state->resize(30, 100);
    
    QCOMPARE(m_state->rows(), 30);
    QCOMPARE(m_state->cols(), 100);
}

void TerminalStateTest::testClear() {
    m_state->write(QString("Hello"));
    m_state->clear();
    
    QCOMPARE(m_state->cursor().row, 0);
    QCOMPARE(m_state->cursor().col, 0);
}

void TerminalStateTest::testScrollRegion() {
    m_state->setScrollRegion(2, 20, 0, 80);
    
    ScrollRegion region = m_state->scrollRegion();
    QVERIFY(region.isValid());
    QCOMPARE(region.top, 2);
    QCOMPARE(region.bottom, 20);
}

QTEST_MAIN(TerminalStateTest)
#include "TerminalStateTest.moc"
