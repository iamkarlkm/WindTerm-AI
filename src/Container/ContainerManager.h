#ifndef CONTAINER_MANAGER_H
#define CONTAINER_MANAGER_H

#include <QObject>
#include <QMap>
#include <QList>

struct ContainerInfo {
    QString id;
    QString name;
    QString image;
    QString status;  // running, stopped, paused, exited
    QString state;
    QString created;
    QString started;
    QString finished;
    QStringList ports;
    QString network;
    QString ipAddress;
    QString pid;
    int cpuPercent;
    int memoryPercent;
    int64_t memoryUsage;
    int64_t memoryLimit;
    QStringList volumes;
    QStringList labels;
    QString command;
};

struct ImageInfo {
    QString id;
    QString repository;
    QString tag;
    int64_t size;
    QString created;
    QStringList labels;
    QString os;
    QString architecture;
};

struct PodInfo {
    QString id;
    QString name;
    QString namespace_;
    QString status;
    int readyContainers;
    int totalContainers;
    int restarts;
    QString age;
    QString node;
    QString ip;
    QStringList containers;
};

struct ContainerLog {
    QString containerId;
    QString content;
    qint64 timestamp;
    bool isStderr;
};

class ContainerManager : public QObject {
    Q_OBJECT
public:
    explicit ContainerManager(QObject* parent = nullptr);
    
    static ContainerManager* instance();
    
    // Docker 容器管理
    QList<ContainerInfo> listContainers(bool all = false, const QString& filter = "") const;
    ContainerInfo getContainerInfo(const QString& containerId) const;
    bool startContainer(const QString& containerId);
    bool stopContainer(const QString& containerId, int timeout = 10);
    bool restartContainer(const QString& containerId, int timeout = 10);
    bool pauseContainer(const QString& containerId);
    bool unpauseContainer(const QString& containerId);
    bool removeContainer(const QString& containerId, bool force = false);
    
    // 容器创建
    QString createContainer(const QString& image, const QString& name = "", 
                           const QStringList& volumes = QStringList(),
                           const QStringList& ports = QStringList(),
                           const QStringList& env = QStringList(),
                           const QString& command = "");
    
    // 容器执行
    int executeInContainer(const QString& containerId, const QString& command, 
                          QString& output, QString& error);
    
    // 日志
    QList<ContainerLog> getContainerLogs(const QString& containerId, int lines = 100, 
                                        bool follow = false, bool timestamps = true) const;
    
    // 统计
    struct ContainerStats {
        QString containerId;
        int cpuPercent;
        int memoryPercent;
        int64_t memoryUsage;
        int64_t networkRx;
        int64_t networkTx;
        int64_t blockRead;
        int64_t blockWrite;
        qint64 timestamp;
    };
    ContainerStats getContainerStats(const QString& containerId) const;
    
    // Docker 镜像
    QList<ImageInfo> listImages() const;
    bool pullImage(const QString& image, const QString& tag = "latest");
    bool removeImage(const QString& imageId, bool force = false);
    bool tagImage(const QString& imageId, const QString& newTag);
    bool buildImage(const QString& dockerfilePath, const QString& tag, 
                   const QString& contextPath = ".");
    
    // Docker volume
    QStringList listVolumes() const;
    bool createVolume(const QString& name);
    bool removeVolume(const QString& name);
    
    // Docker network
    QStringList listNetworks() const;
    bool createNetwork(const QString& name, const QString& driver = "bridge");
    bool removeNetwork(const QString& networkId);
    bool connectNetwork(const QString& containerId, const QString& networkId);
    bool disconnectNetwork(const QString& containerId, const QString& networkId);
    
    // Kubernetes Pods
    QList<PodInfo> listPods(const QString& namespace_ = "default") const;
    PodInfo getPodInfo(const QString& podId, const QString& namespace_ = "default") const;
    bool deletePod(const QString& podId, const QString& namespace_ = "default");
    bool restartPod(const QString& podId, const QString& namespace_ = "default");
    
    // Kubernetes 日志
    QList<ContainerLog> getPodLogs(const QString& podId, const QString& namespace_ = "default",
                                  int lines = 100) const;
    
    // Kubernetes 执行
    int executeInPod(const QString& podId, const QString& command,
                    const QString& namespace_ = "default",
                    QString& output = "", QString& error = "");
    
    // 环境检测
    static bool isDockerAvailable();
    static bool isKubectlAvailable();
    static QString getDockerVersion();
    static QString getKubectlVersion();
    static QString getCurrentKubeContext();
    static QStringList listKubeNamespaces();
    
    // Docker Compose
    bool deployCompose(const QString& composeFile, const QString& project = "");
    bool stopCompose(const QString& composeFile, const QString& project = "");
    bool removeCompose(const QString& composeFile, const QString& project = "");
    
signals:
    void containerStarted(const QString& containerId);
    void containerStopped(const QString& containerId);
    void containerRemoved(const QString& containerId);
    void imagePulled(const QString& image);
    void errorOccurred(const QString& message);

private:
    static ContainerManager* s_instance;
    
    QStringList executeDocker(const QStringList& args) const;
    QStringList executeKubectl(const QStringList& args, const QString& namespace_ = "default") const;
    ContainerInfo parseContainerInfo(const QString& json) const;
    ImageInfo parseImageInfo(const QString& json) const;
    PodInfo parsePodInfo(const QString& json) const;
};

#endif
