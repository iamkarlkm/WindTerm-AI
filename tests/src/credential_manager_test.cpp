#include <QTest>
#include <QObject>
#include "../../src/Security/CredentialManager.h"

class CredentialManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testSingletonInstance();
    void testCredentialStructure();
    void testMasterPassword();
    void cleanupTestCase();

private:
    CredentialManager* m_manager;
};

void CredentialManagerTest::initTestCase() {
    m_manager = CredentialManager::instance();
}

void CredentialManagerTest::testSingletonInstance() {
    CredentialManager* instance1 = CredentialManager::instance();
    CredentialManager* instance2 = CredentialManager::instance();
    QCOMPARE(instance1, instance2);
}

void CredentialManagerTest::testCredentialStructure() {
    Credential cred;
    cred.host = "test.example.com";
    cred.username = "testuser";
    cred.password = "secret";
    cred.port = 22;
    
    QCOMPARE(cred.host, QString("test.example.com"));
    QCOMPARE(cred.username, QString("testuser"));
    QCOMPARE(cred.port, 22);
}

void CredentialManagerTest::testMasterPassword() {
    QVERIFY(m_manager->isLocked());
    m_manager->lock();
    QVERIFY(m_manager->isLocked());
}

void CredentialManagerTest::cleanupTestCase() {
    m_manager->lock();
}

QTEST_APPLESS_MAIN(CredentialManagerTest)
#include "credential_manager_test.moc"
