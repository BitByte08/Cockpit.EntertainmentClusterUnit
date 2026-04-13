#include "UpdateManager.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>

static constexpr const char *kApiBase = "https://api.github.com/repos/";
static constexpr const char *kAssetName = "cluster-arm64";
static constexpr const char *kUpdateScript = "/opt/cluster/update.sh";

UpdateManager::UpdateManager(const QString &currentVersion,
                             const QString &githubRepo,
                             QObject *parent)
    : QObject(parent),
      net_(new QNetworkAccessManager(this)),
      current_version_(currentVersion),
      github_repo_(githubRepo) {}

void UpdateManager::checkForUpdate() {
    QUrl url(QString("%1%2/releases/latest").arg(kApiBase, github_repo_));
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "cluster-app");

    QNetworkReply *reply = net_->get(req);
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onCheckReplyFinished(reply); });
}

void UpdateManager::onCheckReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit updateError(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull()) {
        emit updateError("JSON 파싱 실패");
        return;
    }

    QString tag = doc.object().value("tag_name").toString();
    latest_version_ = tag.startsWith('v') ? tag.mid(1) : tag;

    if (latest_version_.isEmpty()) {
        emit updateError("릴리스 태그 없음");
        return;
    }

    if (!isNewerVersion(latest_version_, current_version_)) {
        emit alreadyUpToDate();
        return;
    }

    // 에셋 URL 탐색
    const QJsonArray assets = doc.object().value("assets").toArray();
    for (const QJsonValue &v : assets) {
        QJsonObject asset = v.toObject();
        if (asset.value("name").toString() == kAssetName) {
            download_url_ = asset.value("browser_download_url").toString();
            break;
        }
    }

    if (download_url_.isEmpty()) {
        emit updateError(QString("릴리스에서 '%1' 에셋 없음").arg(kAssetName));
        return;
    }

    emit updateAvailable(latest_version_);
}

void UpdateManager::downloadAndApply() {
    if (download_url_.isEmpty()) return;

    QNetworkRequest req{QUrl(download_url_)};
    req.setRawHeader("User-Agent", "cluster-app");
    QNetworkReply *reply = net_->get(req);

    connect(reply, &QNetworkReply::downloadProgress,
            this, &UpdateManager::onDownloadProgress);
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onDownloadFinished(reply); });
}

void UpdateManager::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0)
        emit updateProgress(static_cast<int>(received * 100 / total));
}

void UpdateManager::onDownloadFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit updateError(reply->errorString());
        return;
    }

    // 임시 파일 저장
    QString tmpPath = QDir::tempPath() + "/cluster-update";
    QFile f(tmpPath);
    if (!f.open(QIODevice::WriteOnly)) {
        emit updateError("임시 파일 쓰기 실패");
        return;
    }
    f.write(reply->readAll());
    f.close();

    // update.sh 스크립트가 설치되어 있으면 위임, 아니면 직접 교체
    if (QFile::exists(kUpdateScript)) {
        // update.sh는 부팅 시 OTA 담당 — 앱에서는 재시작만 트리거
        emit updateReady();
    } else {
        // 직접 교체 시도 (sudo 필요)
        QStringList args = {
            "cp", tmpPath, "/opt/cluster/cluster"
        };
        int ret = QProcess::execute("sudo", args);
        if (ret == 0) {
            emit updateReady();
        } else {
            emit updateError("바이너리 교체 실패 (sudo 권한 확인)");
        }
    }
}

bool UpdateManager::isNewerVersion(const QString &remote, const QString &local) const {
    auto parse = [](const QString &v) -> QList<int> {
        QList<int> parts;
        for (const QString &p : v.split('.'))
            parts.append(p.toInt());
        while (parts.size() < 3) parts.append(0);
        return parts;
    };

    QList<int> r = parse(remote);
    QList<int> l = parse(local);
    for (int i = 0; i < 3; ++i) {
        if (r[i] > l[i]) return true;
        if (r[i] < l[i]) return false;
    }
    return false;
}
