#ifndef CLUSTER_MAINWINDOW_HPP
#define CLUSTER_MAINWINDOW_HPP

#include <QMainWindow>
#include <QLabel>
#include <memory>

namespace Ui {
class MainWindow;
}

class ClusterModel;
class UpdateManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSpeedChanged(int speed);
    void onFuelChanged(int fuel);
    void onTempChanged(int temp);

    void onUpdateAvailable(const QString &version);
    void onUpdateReady();
    void onUpdateProgress(int percent);
    void onUpdateError(const QString &msg);

private:
    void setupGauges();
    void setupInfoPanel();
    void connectSignals();
    void setupUpdateBanner();
    void applyUpdate();

    Ui::MainWindow   *ui;
    ClusterModel     *cluster_model_;
    UpdateManager    *update_manager_;
    QLabel           *update_banner_{nullptr};
};
#endif // CLUSTER_MAINWINDOW_HPP
