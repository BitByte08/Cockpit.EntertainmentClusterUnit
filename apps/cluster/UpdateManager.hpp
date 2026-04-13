#ifndef CLUSTER_UPDATEMANAGER_HPP
#define CLUSTER_UPDATEMANAGER_HPP

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class UpdateManager : public QObject {
    Q_OBJECT
public:
    explicit UpdateManager(const QString &currentVersion,
                           const QString &githubRepo,
                           QObject *parent = nullptr);

    void checkForUpdate();
    void downloadAndApply();

    QString latestVersion() const { return latest_version_; }
    QString downloadUrl() const { return download_url_; }

signals:
    void updateAvailable(const QString &version);
    void updateProgress(int percent);
    void updateReady();
    void updateError(const QString &message);
    void alreadyUpToDate();

private slots:
    void onCheckReplyFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(QNetworkReply *reply);

private:
    bool isNewerVersion(const QString &remote, const QString &local) const;

    QNetworkAccessManager *net_;
    QString current_version_;
    QString github_repo_;
    QString latest_version_;
    QString download_url_;
};

#endif // CLUSTER_UPDATEMANAGER_HPP
