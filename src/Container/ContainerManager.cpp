#include "ContainerManager.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QStandardPaths>

ContainerManager* ContainerManager::s_instance = nullptr;

ContainerManager::ContainerManager(QObject* parent) : QObject(parent) {}

ContainerManager* ContainerManager::instance() {
    if (!s_instance) s_instance = new ContainerManager();
    return s_instance;
}

bool ContainerManager::isDockerAvailable() {
    return !QStandardPaths::findExecutable("docker").isEmpty();
}

bool ContainerManager::isKubectlAvailable() {
    return !QStandardPaths::findExecutable("kubectl").isEmpty();
}

QString ContainerManager::getDockerVersion() {
    QProcess p; p.start("docker", {"--version"}); p.waitForFinished(5000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

QString ContainerManager::getKubectlVersion() {
    QProcess p; p.start("kubectl", {"version", "--client"}); p.waitForFinished(5000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

QStringList ContainerManager::executeDocker(const QStringList& args) const {
    QProcess p;
    p.start("docker", args);
    p.waitForFinished(30000);
    QString output = QString::fromUtf8(p.readAllStandardOutput());
    return output.split('\n', Qt::SkipEmptyParts);
}

QStringList ContainerManager::executeKubectl(const QStringList& args, const QString& ns) const {
    QStringList fullArgs = args;
    if (!ns.isEmpty()) fullArgs << "-n" << ns;
    QProcess p;
    p.start("kubectl", fullArgs);
    p.waitForFinished(30000);
    QString output = QString::fromUtf8(p.readAllStandardOutput());
    return output.split('\n', Qt::SkipEmptyParts);
}

QList<ContainerInfo> ContainerManager::listContainers(bool all, const QString& filter) const {
    QList<ContainerInfo> containers;
    if (!isDockerAvailable()) return containers;
    
    QStringList args = {"ps", "--format", "json"};
    if (all) args << "-a";
    if (!filter.isEmpty()) args << "--filter" << filter;
    
    QStringList lines = executeDocker(args);
    for (const QString& line : lines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isNull()) continue;
        QJsonObject json = doc.object();
        
        ContainerInfo info;
        info.id = json["ID"].toString().left(12);
        info.name = json["Names"].toString();
        info.image = json["Image"].toString();
        info.status = json["State"].toString();
        info.ports = QStringList(json["Ports"].toArray().toVariantList().toStringList());
        info.command = json["Command"].toString();
        info.created = json["CreatedAt"].toString();
        info.labels = QStringList(json["Labels"].toObject().keys());
        
        containers.append(info);
    }
    return containers;
}

ContainerInfo ContainerManager::getContainerInfo(const QString& containerId) const {
    ContainerInfo info;
    if (!isDockerAvailable()) return info;
    
    QStringList lines = executeDocker({"inspect", containerId});
    if (lines.isEmpty()) return info;
    
    QJsonDocument doc = QJsonDocument::fromJson(lines.join("").toUtf8());
    if (doc.isNull() || !doc.isArray()) return info;
    QJsonArray arr = doc.array();
    if (arr.isEmpty()) return info;
    
    QJsonObject json = arr[0].toObject();
    info.id = json["Id"].toString().left(12);
    info.name = json["Name"].toString().replace("/", "");
    info.image = json["Config"].toObject()["Image"].toString();
    info.state = json["State"].toObject()["Status"].toString();
    info.created = json["Created"].toString();
    info.started = json["State"].toObject()["StartedAt"].toString();
    info.pid = QString::number(json["State"].toObject()["Pid"].toInt());
    
    // Network
    QJsonObject network = json["NetworkSettings"].toObject();
    info.ipAddress = network["IPAddress"].toString();
    
    return info;
}

bool ContainerManager::startContainer(const QString& containerId) {
    if (!isDockerAvailable()) return false;
    QStringList res = executeDocker({"start", containerId});
    emit containerStarted(containerId);
    return !res.isEmpty();
}

bool ContainerManager::stopContainer(const QString& containerId, int timeout) {
    if (!isDockerAvailable()) return false;
    executeDocker({"stop", "-t", QString::number(timeout), containerId});
    emit containerStopped(containerId);
    return true;
}

bool ContainerManager::restartContainer(const QString& containerId, int timeout) {
    if (!isDockerAvailable()) return false;
    executeDocker({"restart", "-t", QString::number(timeout), containerId});
    return true;
}

bool ContainerManager::removeContainer(const QString& containerId, bool force) {
    if (!isDockerAvailable()) return false;
    QStringList args = {"rm"};
    if (force) args << "-f";
    args << containerId;
    executeDocker(args);
    emit containerRemoved(containerId);
    return true;
}

QString ContainerManager::createContainer(const QString& image, const QString& name,
    const QStringList& volumes, const QStringList& ports,
    const QStringList& env, const QString& command) {
    if (!isDockerAvailable()) return QString();
    
    QStringList args = {"create"};
    if (!name.isEmpty()) args << "--name" << name;
    for (const QString& v : volumes) args << "-v" << v;
    for (const QString& p : ports) args << "-p" << p;
    for (const QString& e : env) args << "-e" << e;
    args << image;
    if (!command.isEmpty()) args << "sh" << "-c" << command;
    
    QStringList res = executeDocker(args);
    return res.isEmpty() ? QString() : res[0].left(12);
}

int ContainerManager::executeInContainer(const QString& containerId, const QString& command,
                                        QString& output, QString& error) {
    if (!isDockerAvailable()) return -1;
    
    QProcess p;
    p.start("docker", {"exec", containerId, "sh", "-c", command});
    p.waitForFinished(60000);
    output = QString::fromUtf8(p.readAllStandardOutput());
    error = QString::fromUtf8(p.readAllStandardError());
    return p.exitCode();
}

QList<ContainerLog> ContainerManager::getContainerLogs(const QString& containerId,
    int lines, bool follow, bool timestamps) const {
    QList<ContainerLog> logs;
    if (!isDockerAvailable()) return logs;
    
    QStringList args = {"logs", "--tail", QString::number(lines)};
    if (timestamps) args << "-t";
    args << containerId;
    
    QStringList lines_list = executeDocker(args);
    for (const QString& line : lines_list) {
        ContainerLog log;
        log.containerId = containerId;
        log.content = line;
        log.timestamp = QDateTime::currentMSecsSinceEpoch();
        logs.append(log);
    }
    return logs;
}

QList<ImageInfo> ContainerManager::listImages() const {
    QList<ImageInfo> images;
    if (!isDockerAvailable()) return images;
    
    QStringList lines = executeDocker({"images", "--format", "json"});
    for (const QString& line : lines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isNull()) continue;
        QJsonObject json = doc.object();
        
        ImageInfo info;
        info.id = json["ID"].toString().left(12);
        info.repository = json["Repository"].toString();
        info.tag = json["Tag"].toString("latest");
        info.size = json["Size"].toInt(0);
        info.created = json["CreatedAt"].toString();
        images.append(info);
    }
    return images;
}

bool ContainerManager::pullImage(const QString& image, const QString& tag) {
    if (!isDockerAvailable()) return false;
    QString fullImage = tag.isEmpty() ? image : image + ":" + tag;
    executeDocker({"pull", fullImage});
    emit imagePulled(fullImage);
    return true;
}

bool ContainerManager::removeImage(const QString& imageId, bool force) {
    if (!isDockerAvailable()) return false;
    QStringList args = {"rmi"};
    if (force) args << "-f";
    args << imageId;
    executeDocker(args);
    return true;
}

QStringList ContainerManager::listVolumes() const {
    if (!isDockerAvailable()) return QStringList();
    return executeDocker({"volume", "ls", "--format", "{{.Name}}"});
}

bool ContainerManager::createVolume(const QString& name) {
    if (!isDockerAvailable()) return false;
    executeDocker({"volume", "create", name});
    return true;
}

QStringList ContainerManager::listNetworks() const {
    if (!isDockerAvailable()) return QStringList();
    return executeDocker({"network", "ls", "--format", "{{.Name}}"});
}

bool ContainerManager::createNetwork(const QString& name, const QString& driver) {
    if (!isDockerAvailable()) return false;
    executeDocker({"network", "create", "-d", driver, name});
    return true;
}

QString ContainerManager::getCurrentKubeContext() {
    if (!isKubectlAvailable()) return QString();
    QProcess p; p.start("kubectl", {"config", "current-context"}); p.waitForFinished(5000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

QStringList ContainerManager::listKubeNamespaces() {
    QStringList ns;
    if (!isKubectlAvailable()) return ns;
    return executeKubectl({"get", "ns", "-o", "jsonpath={.items[*].metadata.name}"});
}

QList<PodInfo> ContainerManager::listPods(const QString& ns) const {
    QList<PodInfo> pods;
    if (!isKubectlAvailable()) return pods;
    
    QStringList lines = executeKubectl({"get", "pods", "-o", "json"}, ns);
    if (lines.isEmpty()) return pods;
    
    QJsonDocument doc = QJsonDocument::fromJson(lines.join("").toUtf8());
    if (doc.isNull()) return pods;
    
    QJsonObject json = doc.object()["items"].toArray().toVariant().toJsonObject();
    QJsonArray items = doc.object()["items"].toArray();
    
    for (const QJsonValue& val : items) {
        QJsonObject podJson = val.toObject();
        PodInfo info;
        info.id = podJson["metadata"].toObject()["uid"].toString();
        info.name = podJson["metadata"].toObject()["name"].toString();
        info.namespace_ = ns;
        info.status = podJson["status"].toObject()["phase"].toString();
        info.ip = podJson["status"].toObject()["podIP"].toString();
        info.node = podJson["spec"].toObject()["nodeName"].toString();
        info.age = podJson["metadata"].toObject()["creationTimestamp"].toString();
        pods.append(info);
    }
    return pods;
}

int ContainerManager::executeInPod(const QString& podId, const QString& command,
    const QString& ns, QString& output, QString& error) {
    if (!isKubectlAvailable()) return -1;
    
    QProcess p;
    p.start("kubectl", {"exec", podId, "-n", ns, "--", "sh", "-c", command});
    p.waitForFinished(60000);
    output = QString::fromUtf8(p.readAllStandardOutput());
    error = QString::fromUtf8(p.readAllStandardError());
    return p.exitCode();
}

QList<ContainerLog> ContainerManager::getPodLogs(const QString& podId, const QString& ns, int lines) const {
    QList<ContainerLog> logs;
    if (!isKubectlAvailable()) return logs;
    
    QStringList lines_list = executeKubectl({"logs", podId, "--tail", QString::number(lines)}, ns);
    for (const QString& line : lines_list) {
        ContainerLog log;
        log.containerId = podId;
        log.content = line;
        log.timestamp = QDateTime::currentMSecsSinceEpoch();
        logs.append(log);
    }
    return logs;
}

bool ContainerManager::deployCompose(const QString& composeFile, const QString& project) {
    if (!isDockerAvailable()) return false;
    QStringList args = {"-f", composeFile};
    if (!project.isEmpty()) args << "-p" << project;
    args << "up" << "-d";
    executeDocker(args);
    return true;
}

bool ContainerManager::stopCompose(const QString& composeFile, const QString& project) {
    if (!isDockerAvailable()) return false;
    QStringList args = {"-f", composeFile};
    if (!project.isEmpty()) args << "-p" << project;
    args << "stop";
    executeDocker(args);
    return true;
}

bool ContainerManager::buildImage(const QString& dockerfilePath, const QString& tag,
                                 const QString& contextPath) {
    if (!isDockerAvailable()) return false;
    QStringList args = {"build", "-f", dockerfilePath, "-t", tag, contextPath};
    executeDocker(args);
    return true;
}

bool ContainerManager::pauseContainer(const QString& containerId) {
    if (!isDockerAvailable()) return false;
    executeDocker({"pause", containerId});
    return true;
}

bool ContainerManager::unpauseContainer(const QString& containerId) {
    if (!isDockerAvailable()) return false;
    executeDocker({"unpause", containerId});
    return true;
}

bool ContainerManager::tagImage(const QString& imageId, const QString& newTag) {
    if (!isDockerAvailable()) return false;
    executeDocker({"tag", imageId, newTag});
    return true;
}

bool ContainerManager::removeVolume(const QString& name) {
    if (!isDockerAvailable()) return false;
    executeDocker({"volume", "remove", name});
    return true;
}

bool ContainerManager::removeNetwork(const QString& networkId) {
    if (!isDockerAvailable()) return false;
    executeDocker({"network", "remove", networkId});
    return true;
}

bool ContainerManager::connectNetwork(const QString& containerId, const QString& networkId) {
    if (!isDockerAvailable()) return false;
    executeDocker({"network", "connect", networkId, containerId});
    return true;
}

bool ContainerManager::disconnectNetwork(const QString& containerId, const QString& networkId) {
    if (!isDockerAvailable()) return false;
    executeDocker({"network", "disconnect", networkId, containerId});
    return true;
}

bool ContainerManager::deletePod(const QString& podId, const QString& ns) {
    if (!isKubectlAvailable()) return false;
    executeKubectl({"delete", "pod", podId}, ns);
    return true;
}

bool ContainerManager::restartPod(const QString& podId, const QString& ns) {
    if (!isKubectlAvailable()) return false;
    executeKubectl({"rollout", "restart", "pod", podId}, ns);
    return true;
}

ContainerManager::ContainerStats ContainerManager::getContainerStats(const QString& containerId) const {
    ContainerStats stats;
    if (!isDockerAvailable()) return stats;
    // 简化实现
    return stats;
}

bool ContainerManager::removeCompose(const QString& composeFile, const QString& project) {
    if (!isDockerAvailable()) return false;
    QStringList args = {"-f", composeFile};
    if (!project.isEmpty()) args << "-p" << project;
    args << "down";
    executeDocker(args);
    return true;
}

#include "ContainerManager.moc"
