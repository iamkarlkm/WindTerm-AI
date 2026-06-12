#ifndef APITESTCLIENT_H
#define APITESTCLIENT_H

#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief HTTP 请求配置
 */
struct ApiRequest {
    QString id;
    QString name;
    QString method;      // GET, POST, PUT, DELETE, PATCH
    QString url;
    QMap<QString, QString> headers;
    QString body;
    QString bodyType;    // json, form-data, x-www-form-urlencoded, xml
    int timeout = 30;
    bool followRedirects = true;
};

/**
 * @brief API 响应结果
 */
struct ApiResponse {
    QString requestId;
    int statusCode;
    QString statusText;
    QMap<QString, QString> headers;
    QByteArray body;
    QString contentType;
    qint64 responseTime;
    QString error;
    bool success;
};

/**
 * @brief API 测试客户端 - Postman 风格
 * 
 * 功能:
 * - HTTP 请求执行 (GET/POST/PUT/DELETE/PATCH)
 * - 请求头/请求体管理
 * - 响应解析 (JSON/XML/Text)
 * - 断言系统
 * - 测试集合管理
 * - 环境变量支持
 */
class ApiTestClient : public QObject {
    Q_OBJECT

public:
    explicit ApiTestClient(QObject* parent = nullptr);
    ~ApiTestClient();

    // 请求管理
    QString createRequest(const ApiRequest& request);
    bool updateRequest(const QString& requestId, const ApiRequest& request);
    bool deleteRequest(const QString& requestId);
    ApiRequest getRequest(const QString& requestId) const;
    QList<QString> listRequests() const;
    
    // 请求执行
    QString sendRequest(const QString& requestId);
    void sendRequestAsync(const QString& requestId);
    ApiResponse getResponse(const QString& responseId) const;
    
    // 断言系统
    bool assertStatusCode(const QString& responseId, int expectedCode);
    bool assertHeader(const QString& responseId, const QString& key, const QString& value);
    bool assertBodyContains(const QString& responseId, const QString& text);
    bool assertJsonPath(const QString& responseId, const QString& path, const QVariant& expected);
    bool assertResponseTime(const QString& responseId, int maxMs);
    
    // 测试集合
    QString createCollection(const QString& name);
    bool addToCollection(const QString& collectionId, const QString& requestId);
    QList<QString> runCollection(const QString& collectionId);
    
    // 环境变量
    void setEnvironmentVariable(const QString& key, const QString& value);
    QString getEnvironmentVariable(const QString& key) const;
    QMap<QString, QString> getEnvironmentVariables() const;
    
    // 导入导出
    bool exportCollection(const QString& collectionId, const QString& filePath, const QString& format = "json");
    bool importCollection(const QString& filePath, const QString& format = "json");

signals:
    void requestSent(const QString& requestId);
    void responseReceived(const QString& requestId, const ApiResponse& response);
    void assertionPassed(const QString& assertionName);
    void assertionFailed(const QString& assertionName, const QString& message);
    void collectionCompleted(const QString& collectionId, int passed, int failed);

private slots:
    void onReplyFinished(QNetworkReply* reply);
    void onReplyError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager* m_networkManager;
    QMap<QString, ApiRequest> m_requests;
    QMap<QString, ApiResponse> m_responses;
    QMap<QString, QList<QString>> m_collections;
    QMap<QString, QString> m_environmentVars;
    QMap<QString, QString> m_pendingRequests;  // requestId -> responseId
    
    QString generateId() const;
    QString replaceEnvVars(const QString& str) const;
    QNetworkRequest buildNetworkRequest(const ApiRequest& apiRequest);
    
    static ApiTestClient* s_instance;

public:
    static ApiTestClient* instance();
};

#endif // APITESTCLIENT_H
