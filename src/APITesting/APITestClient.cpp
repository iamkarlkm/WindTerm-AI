#include "APITestClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlDocument>
#include <QDateTime>
#include <QFile>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>
#include <QUuid>

ApiTestClient* ApiTestClient::s_instance = nullptr;

ApiTestClient::ApiTestClient(QObject* parent) : QObject(parent) {
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &ApiTestClient::onReplyFinished);
}

ApiTestClient::~ApiTestClient() {
}

ApiTestClient* ApiTestClient::instance() {
    if (!s_instance) s_instance = new ApiTestClient();
    return s_instance;
}

QString ApiTestClient::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

QString ApiTestClient::createRequest(const ApiRequest& request) {
    QString requestId = generateId();
    ApiRequest req = request;
    req.id = requestId;
    m_requests[requestId] = req;
    return requestId;
}

bool ApiTestClient::updateRequest(const QString& requestId, const ApiRequest& request) {
    if (!m_requests.contains(requestId)) return false;
    ApiRequest req = request;
    req.id = requestId;
    m_requests[requestId] = req;
    return true;
}

bool ApiTestClient::deleteRequest(const QString& requestId) {
    return m_requests.remove(requestId);
}

ApiRequest ApiTestClient::getRequest(const QString& requestId) const {
    return m_requests.value(requestId);
}

QList<QString> ApiTestClient::listRequests() const {
    return m_requests.keys();
}

QNetworkRequest ApiTestClient::buildNetworkRequest(const ApiRequest& apiRequest) {
    QNetworkRequest request;
    QString url = replaceEnvVars(apiRequest.url);
    request.setUrl(QUrl(url));
    
    // 设置请求头
    for (auto it = apiRequest.headers.begin(); it != apiRequest.headers.end(); ++it) {
        QString key = replaceEnvVars(it.key());
        QString value = replaceEnvVars(it.value());
        request.setRawHeader(key.toUtf8(), value.toUtf8());
    }
    
    // 设置超时
    request.setAttribute(QNetworkRequest::TimeoutAttribute, apiRequest.timeout * 1000);
    
    // 设置重定向
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, apiRequest.followRedirects);
    
    return request;
}

QString ApiTestClient::sendRequest(const QString& requestId) {
    if (!m_requests.contains(requestId)) return QString();
    
    const ApiRequest& req = m_requests[requestId];
    QNetworkRequest networkRequest = buildNetworkRequest(req);
    
    QString responseId = generateId();
    m_pendingRequests[requestId] = responseId;
    
    QByteArray body = replaceEnvVars(req.body).toUtf8();
    
    QNetworkReply* reply;
    if (req.method == "GET") {
        reply = m_networkManager->get(networkRequest);
    } else if (req.method == "POST") {
        reply = m_networkManager->post(networkRequest, body);
    } else if (req.method == "PUT") {
        reply = m_networkManager->put(networkRequest, body);
    } else if (req.method == "DELETE") {
        reply = m_networkManager->deleteResource(networkRequest);
    } else if (req.method == "PATCH") {
        reply = m_networkManager->sendCustomRequest(networkRequest, "PATCH", body);
    } else {
        reply = m_networkManager->sendCustomRequest(networkRequest, req.method.toUtf8(), body);
    }
    
    emit requestSent(requestId);
    return responseId;
}

void ApiTestClient::sendRequestAsync(const QString& requestId) {
    sendRequest(requestId);
}

void ApiTestClient::onReplyFinished(QNetworkReply* reply) {
    QString requestId;
    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        if (it.value() == reply->property("responseId").toString()) {
            requestId = it.key();
            break;
        }
    }
    
    if (requestId.isEmpty()) {
        reply->deleteLater();
        return;
    }
    
    ApiResponse response;
    response.requestId = requestId;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    response.body = reply->readAll();
    response.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    
    // 获取响应头
    for (const auto& header : reply->rawHeaderPairs()) {
        response.headers[QString::fromUtf8(header.first)] = QString::fromUtf8(header.second);
    }
    
    response.responseTime = reply->property("startTime").toLongLong();
    response.responseTime = QDateTime::currentMSecsSinceEpoch() - response.responseTime;
    
    if (reply->error() != QNetworkReply::NoError) {
        response.success = false;
        response.error = reply->errorString();
    } else {
        response.success = true;
    }
    
    m_responses[response.requestId] = response;
    emit responseReceived(requestId, response);
    
    reply->deleteLater();
}

void ApiTestClient::onReplyError(QNetworkReply::NetworkError error) {
    Q_UNUSED(error)
    // 错误已在 onReplyFinished 中处理
}

ApiResponse ApiTestClient::getResponse(const QString& responseId) const {
    return m_responses.value(responseId);
}

bool ApiTestClient::assertStatusCode(const QString& responseId, int expectedCode) {
    if (!m_responses.contains(responseId)) return false;
    
    const ApiResponse& resp = m_responses[responseId];
    bool passed = resp.statusCode == expectedCode;
    
    if (passed) {
        emit assertionPassed("Status Code");
    } else {
        emit assertionFailed("Status Code", 
            QString("Expected %1, got %2").arg(expectedCode).arg(resp.statusCode));
    }
    
    return passed;
}

bool ApiTestClient::assertHeader(const QString& responseId, const QString& key, const QString& value) {
    if (!m_responses.contains(responseId)) return false;
    
    const ApiResponse& resp = m_responses[responseId];
    QString actualValue = resp.headers.value(key);
    bool passed = actualValue == value;
    
    if (passed) {
        emit assertionPassed("Header: " + key);
    } else {
        emit assertionFailed("Header: " + key,
            QString("Expected '%1', got '%2'").arg(value, actualValue));
    }
    
    return passed;
}

bool ApiTestClient::assertBodyContains(const QString& responseId, const QString& text) {
    if (!m_responses.contains(responseId)) return false;
    
    const ApiResponse& resp = m_responses[responseId];
    bool passed = resp.body.contains(text.toUtf8());
    
    if (passed) {
        emit assertionPassed("Body Contains");
    } else {
        emit assertionFailed("Body Contains", "Response body does not contain: " + text);
    }
    
    return passed;
}

bool ApiTestClient::assertJsonPath(const QString& responseId, const QString& path, const QVariant& expected) {
    if (!m_responses.contains(responseId)) return false;
    
    const ApiResponse& resp = m_responses[responseId];
    QJsonDocument doc = QJsonDocument::fromJson(resp.body);
    
    if (doc.isNull()) return false;
    
    // 简单的 JSONPath 解析 (支持 . 和 [] 语法)
    QJsonValue current = doc.root();
    QStringList parts = path.split(QRegularExpression("[\\[\\].]+"), Qt::SkipEmptyParts);
    
    for (const QString& part : parts) {
        if (current.isObject()) {
            current = current.toObject().value(part);
        } else if (current.isArray()) {
            bool ok;
            int index = part.toInt(&ok);
            if (ok) {
                current = current.toArray().at(index);
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    
    bool passed = current.toVariant() == expected;
    
    if (passed) {
        emit assertionPassed("JSONPath: " + path);
    } else {
        emit assertionFailed("JSONPath: " + path,
            QString("Expected %1, got %2").arg(expected.toString(), current.toVariant().toString()));
    }
    
    return passed;
}

bool ApiTestClient::assertResponseTime(const QString& responseId, int maxMs) {
    if (!m_responses.contains(responseId)) return false;
    
    const ApiResponse& resp = m_responses[responseId];
    bool passed = resp.responseTime <= maxMs;
    
    if (passed) {
        emit assertionPassed("Response Time");
    } else {
        emit assertionFailed("Response Time",
            QString("Response time %1ms exceeds maximum %2ms")
                .arg(resp.responseTime).arg(maxMs));
    }
    
    return passed;
}

QString ApiTestClient::createCollection(const QString& name) {
    QString collectionId = generateId();
    m_collections[collectionId] = QList<QString>();
    m_collections[collectionId].append(name);  // 第一个元素作为名称
    return collectionId;
}

bool ApiTestClient::addToCollection(const QString& collectionId, const QString& requestId) {
    if (!m_collections.contains(collectionId)) return false;
    m_collections[collectionId].append(requestId);
    return true;
}

QList<QString> ApiTestClient::runCollection(const QString& collectionId) {
    QList<QString> results;
    if (!m_collections.contains(collectionId)) return results;
    
    const QList<QString>& requests = m_collections[collectionId];
    int passed = 0, failed = 0;
    
    for (int i = 1; i < requests.size(); i++) {  // 跳过名称
        QString responseId = sendRequest(requests[i]);
        results.append(responseId);
    }
    
    // 实际使用中需要等待所有请求完成
    emit collectionCompleted(collectionId, passed, failed);
    return results;
}

void ApiTestClient::setEnvironmentVariable(const QString& key, const QString& value) {
    m_environmentVars[key] = value;
}

QString ApiTestClient::getEnvironmentVariable(const QString& key) const {
    return m_environmentVars.value(key);
}

QMap<QString, QString> ApiTestClient::getEnvironmentVariables() const {
    return m_environmentVars;
}

QString ApiTestClient::replaceEnvVars(const QString& str) const {
    QString result = str;
    QRegularExpression re("\\{\\{(.*?)\\}\\}");
    QRegularExpressionMatchIterator i = re.globalMatch(result);
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString varName = match.captured(1);
        QString value = m_environmentVars.value(varName);
        if (!value.isEmpty()) {
            result.replace(match.captured(0), value);
        }
    }
    
    return result;
}

bool ApiTestClient::exportCollection(const QString& collectionId, const QString& filePath, const QString& format) {
    Q_UNUSED(format)
    if (!m_collections.contains(collectionId)) return false;
    
    QJsonObject exportData;
    exportData["collectionId"] = collectionId;
    
    QJsonArray requests;
    for (int i = 1; i < m_collections[collectionId].size(); i++) {
        QString reqId = m_collections[collectionId][i];
        ApiRequest req = m_requests.value(reqId);
        
        QJsonObject reqJson;
        reqJson["name"] = req.name;
        reqJson["method"] = req.method;
        reqJson["url"] = req.url;
        
        QJsonObject headers;
        for (auto it = req.headers.begin(); it != req.headers.end(); ++it) {
            headers[it.key()] = it.value();
        }
        reqJson["headers"] = headers;
        reqJson["body"] = req.body;
        
        requests.append(reqJson);
    }
    
    exportData["requests"] = requests;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    
    file.write(QJsonDocument(exportData).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ApiTestClient::importCollection(const QString& filePath, const QString& format) {
    Q_UNUSED(format)
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isNull()) return false;
    
    QJsonObject root = doc.object();
    QString collectionId = createCollection("Imported Collection");
    
    QJsonArray requests = root["requests"].toArray();
    for (const QJsonValue& val : requests) {
        QJsonObject reqJson = val.toObject();
        
        ApiRequest req;
        req.name = reqJson["name"].toString();
        req.method = reqJson["method"].toString();
        req.url = reqJson["url"].toString();
        req.body = reqJson["body"].toString();
        
        QJsonObject headers = reqJson["headers"].toObject();
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            req.headers[it.key()] = it.value().toString();
        }
        
        QString reqId = createRequest(req);
        addToCollection(collectionId, reqId);
    }
    
    return true;
}

#include "APITestClient.moc"
