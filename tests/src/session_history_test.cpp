#include <QTest>
#include <QObject>
#include "../../src/History/SessionHistoryManager.h"

class SessionHistoryTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testSingletonInstance();
    void testRecordAndRetrieve();
    void testSearch();
    void testExport();
    void cleanupTestCase();

private:
    SessionHistoryManager* m_manager;
};

void SessionHistoryTest::initTestCase() {
    m_manager = SessionHistoryManager::instance();
}

void SessionHistoryTest::testSingletonInstance() {
    SessionHistoryManager* instance1 = SessionHistoryManager::instance();
    SessionHistoryManager* instance2 = SessionHistoryManager::instance();
    QCOMPARE(instance1, instance2);
}

void SessionHistoryTest::testRecordAndRetrieve() {
    QString sessionId = "test_session_hist_1";
    
    m_manager->recordCommand(sessionId, "ls -la");
    m_manager->recordCommand(sessionId, "cd /tmp");
    
    QList<CommandHistory> history = m_manager->getHistory(sessionId);
    QVERIFY(history.size() >= 2);
}

void SessionHistoryTest::testSearch() {
    m_manager->recordCommand("test_session_hist_2", "docker ps");
    m_manager->recordCommand("test_session_hist_2", "git status");
    
    QList<CommandHistory> dockerResults = m_manager->searchCommands("docker");
    QVERIFY(dockerResults.size() > 0);
}

void SessionHistoryTest::testExport() {
    QString sessionId = "test_session_hist_1";
    
    QString json = m_manager->exportToJson(sessionId);
    QVERIFY(!json.isEmpty());
    
    QString md = m_manager->exportToMarkdown(sessionId);
    QVERIFY(!md.isEmpty());
}

void SessionHistoryTest::cleanupTestCase() {
    m_manager->clearHistory("test_session_hist_1");
    m_manager->clearHistory("test_session_hist_2");
}

QTEST_APPLESS_MAIN(SessionHistoryTest)
#include "session_history_test.moc"
