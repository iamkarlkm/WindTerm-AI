#include <QtTest/QtTest>
#include "Terminal/AnsiParser.h"

class AnsiParserTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void testParsePlainText();
    void testParseCursorPosition();
    void testParseEraseInDisplay();
    void testParseGraphicsMode();
    void testParseSetWindowTitle();
    void testBracketedPasteDetection();

private:
    AnsiParser m_parser;
    int m_commandCount;
    AnsiCommand m_lastExecutedCommand;

    void onCommandCallback(const AnsiCommand& cmd) {
        m_lastExecutedCommand = cmd;
        m_commandCount++;
    }
};

void AnsiParserTest::init() {
    m_parser.clear();
    m_commandCount = 0;
    m_parser.commandCallback = [this](const AnsiCommand& cmd) {
        onCommandCallback(cmd);
    };
}

void AnsiParserTest::testParsePlainText() {
    m_parser.parse(QString("Hello"));
    
    QCOMPARE(m_commandCount, 0);
    QVERIFY(m_parser.currentLine().size() > 0);
    
    QString text;
    for (int i = 0; i < 5 && i < m_parser.currentLine().size(); i++) {
        text += m_parser.currentLine()[i].first;
    }
    QVERIFY(text.startsWith("Hello"));
}

void AnsiParserTest::testParseCursorPosition() {
    m_parser.parse(QByteArray("\x1b[5;10H"));
    
    QCOMPARE(m_commandCount, 1);
    QCOMPARE(m_lastExecutedCommand.command, TerminalCommand::CursorPosition);
    QCOMPARE(m_lastExecutedCommand.param(0, 0), 5);
    QCOMPARE(m_lastExecutedCommand.param(1, 0), 10);
}

void AnsiParserTest::testParseEraseInDisplay() {
    m_parser.parse(QByteArray("\x1b[2J"));
    
    QCOMPARE(m_commandCount, 1);
    QCOMPARE(m_lastExecutedCommand.command, TerminalCommand::EraseInDisplay);
    QCOMPARE(m_lastExecutedCommand.param(0, 0), 2);
}

void AnsiParserTest::testParseGraphicsMode() {
    m_parser.parse(QByteArray("\x1b[31;42;1m"));
    
    QCOMPARE(m_commandCount, 1);
    QCOMPARE(m_lastExecutedCommand.command, TerminalCommand::SetGraphicsMode);
    QCOMPARE(m_lastExecutedCommand.parameters.size(), 3);
    QCOMPARE(m_lastExecutedCommand.parameters[0], 31);
    QCOMPARE(m_lastExecutedCommand.parameters[1], 42);
    QCOMPARE(m_lastExecutedCommand.parameters[2], 1);
}

void AnsiParserTest::testParseSetWindowTitle() {
    m_parser.parse(QByteArray("\x1b]0;Test Title\x07"));
    
    QCOMPARE(m_commandCount, 1);
    QCOMPARE(m_lastExecutedCommand.command, TerminalCommand::SetWindowTitle);
    QVERIFY(m_lastExecutedCommand.oscString.contains("Test Title"));
}

void AnsiParserTest::testBracketedPasteDetection() {
    QByteArray enableSeq = "\x1b[?2004h";
    QByteArray disableSeq = "\x1b[?2004l";
    
    bool detectedEnable = enableSeq.contains("\x1b[?2004h");
    bool detectedDisable = disableSeq.contains("\x1b[?2004l");
    
    QVERIFY(detectedEnable);
    QVERIFY(detectedDisable);
}

QTEST_MAIN(AnsiParserTest)
#include "AnsiParserTest.moc"
