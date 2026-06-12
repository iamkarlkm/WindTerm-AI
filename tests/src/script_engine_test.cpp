#include <QTest>
#include <QObject>
#include "../../src/ScriptEngine/ScriptEngine.h"

class ScriptEngineTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testSingletonInstance();
    void cleanupTestCase();

private:
    ScriptEngineManager* m_engine;
};

void ScriptEngineTest::initTestCase() {
    m_engine = ScriptEngineManager::instance();
}

void ScriptEngineTest::testSingletonInstance() {
    ScriptEngineManager* instance1 = ScriptEngineManager::instance();
    ScriptEngineManager* instance2 = ScriptEngineManager::instance();
    QCOMPARE(instance1, instance2);
}

void ScriptEngineTest::cleanupTestCase() {
    delete m_engine;
}

QTEST_APPLESS_MAIN(ScriptEngineTest)
#include "script_engine_test.moc"
