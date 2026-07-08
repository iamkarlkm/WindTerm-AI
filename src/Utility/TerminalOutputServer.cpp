#include "Utility/TerminalOutputServer.h"
#include "Utility/Logger.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QHostAddress>
#include <csignal>

TerminalOutputServer::TerminalOutputServer(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_running(false)
    , m_builtinsRegistered(false)
{
}

TerminalOutputServer::~TerminalOutputServer()
{
    stop();
}

TerminalOutputServer &TerminalOutputServer::instance()
{
    static TerminalOutputServer s_instance;
    return s_instance;
}

bool TerminalOutputServer::isRunning() const
{
    return m_running && m_server != nullptr;
}

void TerminalOutputServer::registerBuiltinFunctions()
{
    if (m_builtinsRegistered) return;
    m_builtinsRegistered = true;

    m_functions["sendCtrlA"]   = QByteArray("\x01", 1);
    m_functions["sendCtrlB"]   = QByteArray("\x02", 1);
    m_functions["sendCtrlC"]   = QByteArray("\x03", 1);
    m_functions["sendCtrlD"]   = QByteArray("\x04", 1);
    m_functions["sendCtrlE"]   = QByteArray("\x05", 1);
    m_functions["sendCtrlL"]   = QByteArray("\x0c", 1);
    m_functions["sendCtrlR"]   = QByteArray("\x12", 1);
    m_functions["sendCtrlU"]   = QByteArray("\x15", 1);
    m_functions["sendCtrlW"]   = QByteArray("\x17", 1);
    m_functions["sendCtrlZ"]   = QByteArray("\x1a", 1);
    m_functions["sendBackspace"] = QByteArray("\x7f", 1);
    m_functions["sendTab"]     = QByteArray("\t", 1);
    m_functions["sendEnter"]   = QByteArray("\r", 1);
    m_functions["sendEsc"]     = QByteArray("\x1b", 1);
    m_functions["sendUp"]      = QByteArray("\x1b[A", 3);
    m_functions["sendDown"]    = QByteArray("\x1b[B", 3);
    m_functions["sendRight"]   = QByteArray("\x1b[C", 3);
    m_functions["sendLeft"]    = QByteArray("\x1b[D", 3);
    m_functions["sendHome"]    = QByteArray("\x1b[H", 3);
    m_functions["sendEnd"]     = QByteArray("\x1b[F", 3);
    m_functions["sendPgUp"]    = QByteArray("\x1b[5~", 4);
    m_functions["sendPgDown"]  = QByteArray("\x1b[6~", 4);
    m_functions["sendInsert"]  = QByteArray("\x1b[2~", 4);
    m_functions["sendDelete"]  = QByteArray("\x1b[3~", 4);

    m_commandFunctions["git_status"]   = "git status {args}";
    m_commandFunctions["git_log"]      = "git log {args}";
    m_commandFunctions["git_diff"]     = "git diff {args}";
    m_commandFunctions["git_add"]      = "git add {args}";
    m_commandFunctions["git_commit"]   = "git commit {args}";
    m_commandFunctions["git_push"]     = "git push {args}";
    m_commandFunctions["git_pull"]     = "git pull {args}";
    m_commandFunctions["git_fetch"]    = "git fetch {args}";
    m_commandFunctions["git_branch"]   = "git branch {args}";
    m_commandFunctions["git_checkout"] = "git checkout {args}";
    m_commandFunctions["git_merge"]    = "git merge {args}";
    m_commandFunctions["git_rebase"]   = "git rebase {args}";
    m_commandFunctions["git_stash"]    = "git stash {args}";
    m_commandFunctions["git_reset"]    = "git reset {args}";
    m_commandFunctions["git_remote"]   = "git remote {args}";
    m_commandFunctions["git_tag"]      = "git tag {args}";
    m_commandFunctions["git_clone"]    = "git clone {args}";
    m_commandFunctions["git_init"]     = "git init {args}";
    m_commandFunctions["git_switch"]   = "git switch {args}";
    m_commandFunctions["git_restore"]  = "git restore {args}";
    m_commandFunctions["git_cherrypick"] = "git cherry-pick {args}";
    m_commandFunctions["git_bisect"]   = "git bisect {args}";
    m_commandFunctions["git_blame"]    = "git blame {args}";
    m_commandFunctions["git_show"]     = "git show {args}";
    m_commandFunctions["git_describe"] = "git describe {args}";
    m_commandFunctions["git_revparse"] = "git rev-parse {args}";
    m_commandFunctions["git_clean"]    = "git clean {args}";
    m_commandFunctions["git_mv"]       = "git mv {args}";
    m_commandFunctions["git_rm"]       = "git rm {args}";
    m_commandFunctions["git_config"]   = "git config {args}";
    m_commandFunctions["git_submodule"] = "git submodule {args}";
    m_commandFunctions["git_worktree"] = "git worktree {args}";
    m_commandFunctions["git_archive"]  = "git archive {args}";
    m_commandFunctions["git_gc"]       = "git gc {args}";
    m_commandFunctions["git_fsck"]     = "git fsck {args}";
    m_commandFunctions["git_prune"]    = "git prune {args}";

    m_commandFunctions["docker_ps"]          = "docker ps {args}";
    m_commandFunctions["docker_images"]      = "docker images {args}";
    m_commandFunctions["docker_logs"]        = "docker logs {args}";
    m_commandFunctions["docker_exec"]        = "docker exec {args}";
    m_commandFunctions["docker_build"]       = "docker build {args}";
    m_commandFunctions["docker_run"]         = "docker run {args}";
    m_commandFunctions["docker_stop"]        = "docker stop {args}";
    m_commandFunctions["docker_start"]       = "docker start {args}";
    m_commandFunctions["docker_restart"]     = "docker restart {args}";
    m_commandFunctions["docker_rm"]          = "docker rm {args}";
    m_commandFunctions["docker_rmi"]         = "docker rmi {args}";
    m_commandFunctions["docker_inspect"]     = "docker inspect {args}";
    m_commandFunctions["docker_compose_up"]    = "docker-compose up {args}";
    m_commandFunctions["docker_compose_down"]  = "docker-compose down {args}";
    m_commandFunctions["docker_compose_logs"]  = "docker-compose logs {args}";
    m_commandFunctions["docker_compose_ps"]    = "docker-compose ps {args}";
    m_commandFunctions["docker_system_prune"]  = "docker system prune {args}";
    m_commandFunctions["docker_volume_ls"]     = "docker volume ls {args}";
    m_commandFunctions["docker_network_ls"]    = "docker network ls {args}";
    m_commandFunctions["docker_cp"]          = "docker cp {args}";

    m_commandFunctions["kubectl_get"]         = "kubectl get {args}";
    m_commandFunctions["kubectl_describe"]    = "kubectl describe {args}";
    m_commandFunctions["kubectl_logs"]        = "kubectl logs {args}";
    m_commandFunctions["kubectl_apply"]       = "kubectl apply {args}";
    m_commandFunctions["kubectl_delete"]      = "kubectl delete {args}";
    m_commandFunctions["kubectl_exec"]        = "kubectl exec {args}";
    m_commandFunctions["kubectl_create"]      = "kubectl create {args}";
    m_commandFunctions["kubectl_edit"]        = "kubectl edit {args}";
    m_commandFunctions["kubectl_rollout"]     = "kubectl rollout {args}";
    m_commandFunctions["kubectl_scale"]       = "kubectl scale {args}";
    m_commandFunctions["kubectl_portforward"] = "kubectl port-forward {args}";
    m_commandFunctions["kubectl_top"]         = "kubectl top {args}";

    m_commandFunctions["sys_ps"]      = "ps {args}";
    m_commandFunctions["sys_df"]      = "df {args}";
    m_commandFunctions["sys_du"]      = "du {args}";
    m_commandFunctions["sys_free"]    = "free {args}";
    m_commandFunctions["sys_top"]     = "top {args}";
    m_commandFunctions["sys_netstat"] = "netstat {args}";
    m_commandFunctions["sys_ss"]      = "ss {args}";
    m_commandFunctions["sys_lsof"]    = "lsof {args}";
    m_commandFunctions["sys_uptime"]  = "uptime {args}";
    m_commandFunctions["sys_uname"]   = "uname {args}";
    m_commandFunctions["sys_dmesg"]   = "dmesg {args}";
    m_commandFunctions["sys_lsblk"]   = "lsblk {args}";
    m_commandFunctions["sys_iostat"]  = "iostat {args}";
    m_commandFunctions["sys_vmstat"]  = "vmstat {args}";

    m_commandFunctions["file_find"]  = "find {args}";
    m_commandFunctions["file_grep"]  = "grep {args}";
    m_commandFunctions["file_tail"]  = "tail {args}";
    m_commandFunctions["file_cat"]   = "cat {args}";
    m_commandFunctions["file_head"]  = "head {args}";
    m_commandFunctions["file_ls"]    = "ls {args}";
    m_commandFunctions["file_wc"]    = "wc {args}";
    m_commandFunctions["file_sort"]  = "sort {args}";
    m_commandFunctions["file_awk"]   = "awk {args}";
    m_commandFunctions["file_sed"]   = "sed {args}";

    m_commandFunctions["npm_run"]     = "npm run {args}";
    m_commandFunctions["npm_install"] = "npm install {args}";
    m_commandFunctions["npm_test"]    = "npm test {args}";
    m_commandFunctions["yarn_run"]    = "yarn {args}";
    m_commandFunctions["pip_install"] = "pip install {args}";
    m_commandFunctions["cargo_build"] = "cargo build {args}";
    m_commandFunctions["cargo_run"]   = "cargo run {args}";
    m_commandFunctions["cargo_test"]  = "cargo test {args}";
    m_commandFunctions["go_build"]    = "go build {args}";
    m_commandFunctions["go_run"]      = "go run {args}";
    m_commandFunctions["go_test"]     = "go test {args}";
    m_commandFunctions["make_cmd"]    = "make {args}";
    m_commandFunctions["cmake_cmd"]   = "cmake {args}";

    m_commandFunctions["net_curl"] = "curl {args}";
    m_commandFunctions["net_wget"] = "wget {args}";
    m_commandFunctions["net_ping"] = "ping {args}";
    m_commandFunctions["net_ssh"]  = "ssh {args}";
    m_commandFunctions["net_scp"]  = "scp {args}";
    m_commandFunctions["net_rsync"] = "rsync {args}";
    m_commandFunctions["net_nc"]   = "nc {args}";
    m_commandFunctions["net_nslookup"] = "nslookup {args}";

    m_commandFunctions["proc_kill"]  = "kill {args}";
    m_commandFunctions["proc_pgrep"] = "pgrep {args}";
    m_commandFunctions["proc_pkill"] = "pkill {args}";
    m_commandFunctions["proc_nice"]  = "nice {args}";
    m_commandFunctions["proc_renice"] = "renice {args}";
}

void TerminalOutputServer::loadMacros(const QJsonObject &macrosObj)
{
    QMutexLocker locker(&m_mutex);

    for (auto it = macrosObj.begin(); it != macrosObj.end(); ++it) {
        if (it.value().isString()) {
            m_macros[it.key()] = it.value().toString();
            LOG_DEBUG("OutputServer")
                << "Loaded macro:" << it.key();
        }
    }

    LOG_INFO("OutputServer")
        << "Loaded" << m_macros.size() << "command macros";
}

void TerminalOutputServer::registerFunction(const QString &name,
                                             const QByteArray &code)
{
    QMutexLocker locker(&m_mutex);
    m_functions[name] = code;
}

bool TerminalOutputServer::start(quint16 port)
{
    QMutexLocker locker(&m_mutex);

    if (m_server) {
        LOG_WARN("OutputServer") << "Server already running, stopping first";
        locker.unlock();
        stop();
        locker.relock();
    }

    registerBuiltinFunctions();

    m_server = new QWebSocketServer(
        "WindTerm Output Filter Server",
        QWebSocketServer::NonSecureMode,
        this);

    if (!m_server->listen(QHostAddress::Any, port)) {
        LOG_ERROR("OutputServer")
            << "Failed to listen on port" << port
            << ":" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    QObject::connect(m_server, &QWebSocketServer::newConnection,
                     this, &TerminalOutputServer::onNewConnection);

    m_running = true;

    LOG_INFO("OutputServer")
        << "WebSocket server started on port"
        << m_server->serverPort();

    emit serverStarted(m_server->serverPort());
    return true;
}

void TerminalOutputServer::stop()
{
    QMutexLocker locker(&m_mutex);

    if (m_server) {
        m_server->close();

        for (auto &session : m_sessions) {
            if (session.socket) {
                session.socket->close();
                session.socket->deleteLater();
            }
        }
        m_sessions.clear();

        m_server->deleteLater();
        m_server = nullptr;
    }

    m_macros.clear();
    m_functions.clear();
    m_commandFunctions.clear();
    m_builtinsRegistered = false;
    m_running = false;

    LOG_INFO("OutputServer") << "Server stopped";
    emit serverStopped();
}

void TerminalOutputServer::feedLine(const QString &line)
{
    QMutexLocker locker(&m_mutex);

    if (!m_running || line.isEmpty()) {
        return;
    }

    for (auto &session : m_sessions) {
        if (!session.socket
            || session.socket->state() != QAbstractSocket::ConnectedState) {
            continue;
        }

        for (const auto &sub : session.subscriptions) {
            QRegularExpressionMatch match = sub.regex.match(line);
            if (!match.hasMatch()) {
                continue;
            }

            QString matchedText = match.captured(0);
            pushToClient(session, sub, matchedText);
        }
    }
}

void TerminalOutputServer::onNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    ClientSession session(socket);
    session.remoteId = socket->peerAddress().toString() + ":"
                       + QString::number(socket->peerPort());

    QObject::connect(socket, &QWebSocket::textMessageReceived,
                     this, [this, socket](const QString &msg) {
                         onTextMessageReceived(socket, msg);
                     });
    QObject::connect(socket, &QWebSocket::disconnected,
                     this, [this, socket]() {
                         onClientDisconnected(socket);
                     });

    m_sessions.append(session);

    LOG_INFO("OutputServer")
        << "Client connected:" << session.remoteId;
    emit clientConnected(session.remoteId);
}

void TerminalOutputServer::onTextMessageReceived(
    QWebSocket *socket, const QString &message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "Invalid JSON: " + parseError.errorString();
        sendJson(socket, err);
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    QMutexLocker locker(&m_mutex);
    ClientSession *session = findSession(socket);
    if (!session) {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "Session not found";
        sendJson(socket, err);
        return;
    }

    if (type == "subscribe") {
        QString name = obj["name"].toString();
        QString pattern = obj["pattern"].toString();

        if (name.isEmpty() || pattern.isEmpty()) {
            QJsonObject err;
            err["type"] = "error";
            err["message"] = "subscribe requires 'name' and 'pattern'";
            sendJson(socket, err);
            return;
        }

        for (int i = 0; i < session->subscriptions.size(); ++i) {
            if (session->subscriptions[i].name == name) {
                session->subscriptions.removeAt(i);
                break;
            }
        }

        QRegularExpression regex(pattern);
        if (!regex.isValid()) {
            QJsonObject err;
            err["type"] = "error";
            err["message"] = "Invalid regex: " + regex.errorString();
            sendJson(socket, err);
            return;
        }

        session->subscriptions.append(ServerSubscription(name, regex));

        QJsonObject ack;
        ack["type"] = "subscribed";
        ack["name"] = name;
        sendJson(socket, ack);

        LOG_DEBUG("OutputServer")
            << "Client" << session->remoteId
            << "subscribed:" << name << pattern;

        emit subscriptionAdded(session->remoteId, name);

    } else if (type == "unsubscribe") {
        QString name = obj["name"].toString();

        if (name.isEmpty()) {
            QJsonObject err;
            err["type"] = "error";
            err["message"] = "unsubscribe requires 'name'";
            sendJson(socket, err);
            return;
        }

        bool removed = false;
        for (int i = 0; i < session->subscriptions.size(); ++i) {
            if (session->subscriptions[i].name == name) {
                session->subscriptions.removeAt(i);
                removed = true;
                break;
            }
        }

        QJsonObject ack;
        ack["type"] = removed ? "unsubscribed" : "error";
        if (removed) {
            ack["name"] = name;
        } else {
            ack["message"] = "Subscription not found: " + name;
        }
        sendJson(socket, ack);

        if (removed) {
            LOG_DEBUG("OutputServer")
                << "Client" << session->remoteId
                << "unsubscribed:" << name;
            emit subscriptionRemoved(session->remoteId, name);
        }

    } else if (type == "list") {
        QJsonObject resp;
        resp["type"] = "subscription_list";

        QJsonArray arr;
        for (const auto &sub : session->subscriptions) {
            QJsonObject s;
            s["name"] = sub.name;
            arr.append(s);
        }
        resp["subscriptions"] = arr;
        sendJson(socket, resp);

    } else if (type == "exec") {
        locker.unlock();
        handleExec(socket, obj);

    } else if (type == "macro") {
        locker.unlock();
        handleMacro(socket, obj);

    } else if (type == "call") {
        locker.unlock();
        handleCall(socket, obj);

    } else {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "Unknown message type: " + type
                         + ". Supported: subscribe, unsubscribe, list, exec, macro, call";
        sendJson(socket, err);
    }
}

void TerminalOutputServer::handleExec(QWebSocket *socket,
                                       const QJsonObject &obj)
{
    QString command = obj["command"].toString();
    if (command.isEmpty()) {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "exec requires 'command'";
        sendJson(socket, err);
        return;
    }

    if (!command.endsWith('\r') && !command.endsWith('\n')) {
        command.append('\r');
    }

    LOG_DEBUG("OutputServer") << "Executing command:" << command.trimmed();

    emit commandRequested(command);

    QJsonObject ack;
    ack["type"] = "executed";
    ack["command"] = command.trimmed();
    sendJson(socket, ack);
}

void TerminalOutputServer::handleMacro(QWebSocket *socket,
                                        const QJsonObject &obj)
{
    QString name = obj["name"].toString();
    if (name.isEmpty()) {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "macro requires 'name'";
        sendJson(socket, err);
        return;
    }

    QMutexLocker locker(&m_mutex);
    auto it = m_macros.find(name);
    if (it == m_macros.end()) {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "Macro not found: " + name;
        err["available"] = QJsonArray::fromStringList(m_macros.keys());
        sendJson(socket, err);
        return;
    }

    QString command = it.value();
    locker.unlock();

    if (!command.endsWith('\r') && !command.endsWith('\n')) {
        command.append('\r');
    }

    LOG_DEBUG("OutputServer") << "Executing macro:" << name << command.trimmed();

    emit commandRequested(command);

    QJsonObject ack;
    ack["type"] = "executed";
    ack["macro"] = name;
    ack["command"] = command.trimmed();
    sendJson(socket, ack);
}

void TerminalOutputServer::handleCall(QWebSocket *socket,
                                       const QJsonObject &obj)
{
    QString name = obj["function"].toString();
    if (name.isEmpty()) {
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "call requires 'function'";
        sendJson(socket, err);
        return;
    }

    QString args = obj["args"].toString().trimmed();

    QMutexLocker locker(&m_mutex);

    auto cmdIt = m_commandFunctions.find(name);
    if (cmdIt != m_commandFunctions.end()) {
        QString templ = cmdIt.value();
        QString command = templ;
        QString resolvedArgs = args;

        if (templ.contains("{args}")) {
            if (!args.isEmpty()) {
                command = QString(templ).replace("{args}", args);
            } else {
                command = QString(templ).replace("{args}", "");
            }
        } else if (!args.isEmpty()) {
            command = templ + " " + args;
            resolvedArgs = args;
        } else {
            command = templ;
        }

        command = command.trimmed();
        if (!command.endsWith('\r') && !command.endsWith('\n')) {
            command.append('\r');
        }

        locker.unlock();

        LOG_DEBUG("OutputServer") << "Executing command function:"
                                  << name << command.trimmed();

        emit commandRequested(command);

        QJsonObject ack;
        ack["type"] = "called";
        ack["function"] = name;
        if (!resolvedArgs.isEmpty()) {
            ack["args"] = resolvedArgs;
        }
        ack["command"] = command.trimmed();
        sendJson(socket, ack);
        return;
    }

    auto it = m_functions.find(name);
    if (it == m_functions.end()) {
        QStringList allNames = m_functions.keys() + m_commandFunctions.keys();
        QJsonObject err;
        err["type"] = "error";
        err["message"] = "Function not found: " + name;
        err["available"] = QJsonArray::fromStringList(allNames);
        sendJson(socket, err);
        return;
    }

    QByteArray code = it.value();
    locker.unlock();

    if (name == "sendCtrlC" || name == "sendSigint") {
        LOG_DEBUG("OutputServer") << "Sending SIGINT";
        emit signalRequested(SIGINT);
    } else if (name == "sendCtrlD" || name == "sendEof") {
        LOG_DEBUG("OutputServer") << "Sending EOF (Ctrl+D)";
        emit rawBytesRequested(QByteArray("\x04", 1));
    } else if (name == "sendCtrlZ" || name == "sendSigstop") {
        LOG_DEBUG("OutputServer") << "Sending SIGTSTP";
        emit signalRequested(SIGTSTP);
    } else if (name == "sendSigterm") {
        LOG_DEBUG("OutputServer") << "Sending SIGTERM";
        emit signalRequested(SIGTERM);
    } else {
        LOG_DEBUG("OutputServer") << "Sending raw bytes for function:" << name;
        emit rawBytesRequested(code);
    }

    QJsonObject ack;
    ack["type"] = "called";
    ack["function"] = name;
    sendJson(socket, ack);
}

void TerminalOutputServer::onClientDisconnected(QWebSocket *socket)
{
    QMutexLocker locker(&m_mutex);

    ClientSession *session = findSession(socket);
    if (session) {
        QString remoteId = session->remoteId;
        for (int i = 0; i < m_sessions.size(); ++i) {
            if (m_sessions[i].socket == socket) {
                m_sessions.removeAt(i);
                break;
            }
        }

        LOG_INFO("OutputServer")
            << "Client disconnected:" << remoteId;
        emit clientDisconnected(remoteId);
    }

    socket->deleteLater();
}

ClientSession *TerminalOutputServer::findSession(QWebSocket *socket)
{
    for (auto &session : m_sessions) {
        if (session.socket == socket) {
            return &session;
        }
    }
    return nullptr;
}

void TerminalOutputServer::sendJson(
    QWebSocket *socket, const QJsonObject &obj)
{
    if (!socket
        || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    QJsonDocument doc(obj);
    socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void TerminalOutputServer::pushToClient(
    ClientSession &session, const ServerSubscription &sub,
    const QString &matchedText)
{
    QJsonObject msg;
    msg["type"] = "match";
    msg["name"] = sub.name;
    msg["text"] = matchedText;
    msg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    sendJson(session.socket, msg);

    Q_EMIT pushMessage(session.remoteId, sub.name, matchedText);
}
