#include <QTest>
#include <QObject>
#include "../src/Widget/TerminalMultiplexer.h"
#include "../src/Widget/TerminalWidget.h"

class TerminalMultiplexerTest : public QObject {
    Q_OBJECT
    
private slots:
    void testCreatePane();
    void testSplitPane();
    void testFocusNavigation();
    void testPresetLayouts();
    
private:
    TerminalMultiplexer* multiplexer;
};

void TerminalMultiplexerTest::testCreatePane() {
    multiplexer = new TerminalMultiplexer();
    
    TerminalWidget* pane1 = multiplexer->createPane("Test Pane 1");
    QCOMPARE(multiplexer->paneCount(), 1);
    QVERIFY(pane1 != nullptr);
    QCOMPARE(pane1->windowTitle(), QString("Test Pane 1"));
    
    TerminalWidget* pane2 = multiplexer->createPane("Test Pane 2");
    QCOMPARE(multiplexer->paneCount(), 2);
    
    delete multiplexer;
}

void TerminalMultiplexerTest::testSplitPane() {
    multiplexer = new TerminalMultiplexer();
    
    TerminalWidget* pane = multiplexer->createPane("Main Pane");
    QCOMPARE(multiplexer->paneCount(), 1);
    
    multiplexer->splitPane(pane, PaneLayout::Horizontal);
    QCOMPARE(multiplexer->paneCount(), 2);
    
    multiplexer->splitPane(pane, PaneLayout::Vertical);
    QCOMPARE(multiplexer->paneCount(), 3);
    
    delete multiplexer;
}

void TerminalMultiplexerTest::testFocusNavigation() {
    multiplexer = new TerminalMultiplexer();
    
    multiplexer->createPane("Pane 1");
    multiplexer->createPane("Pane 2");
    multiplexer->createPane("Pane 3");
    
    QCOMPARE(multiplexer->paneCount(), 3);
    
    multiplexer->focusPane(0);
    QCOMPARE(multiplexer->currentPane()->windowTitle(), QString("Pane 1"));
    
    multiplexer->focusNextPane();
    QCOMPARE(multiplexer->currentPane()->windowTitle(), QString("Pane 2"));
    
    multiplexer->focusNextPane();
    QCOMPARE(multiplexer->currentPane()->windowTitle(), QString("Pane 3"));
    
    multiplexer->focusNextPane();
    QCOMPARE(multiplexer->currentPane()->windowTitle(), QString("Pane 1"));
    
    multiplexer->focusPreviousPane();
    QCOMPARE(multiplexer->currentPane()->windowTitle(), QString("Pane 3"));
    
    delete multiplexer;
}

void TerminalMultiplexerTest::testPresetLayouts() {
    multiplexer = new TerminalMultiplexer();
    
    multiplexer->layoutTwoHorizontal();
    QCOMPARE(multiplexer->paneCount(), 2);
    
    delete multiplexer;
    multiplexer = new TerminalMultiplexer();
    
    multiplexer->layoutTwoVertical();
    QCOMPARE(multiplexer->paneCount(), 2);
    
    delete multiplexer;
    multiplexer = new TerminalMultiplexer();
    
    multiplexer->layoutThreeColumns();
    QCOMPARE(multiplexer->paneCount(), 3);
    
    delete multiplexer;
    multiplexer = new TerminalMultiplexer();
    
    multiplexer->layoutGrid2x2();
    QCOMPARE(multiplexer->paneCount(), 4);
    
    delete multiplexer;
}

QTEST_MAIN(TerminalMultiplexerTest)
#include "TerminalMultiplexerTest.moc"
