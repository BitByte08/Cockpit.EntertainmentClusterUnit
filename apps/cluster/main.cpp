#include <QApplication>
#include "mainwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationVersion(APP_VERSION);

    MainWindow window;

    // --fullscreen 또는 환경변수 CLUSTER_KIOSK=1 로 키오스크 모드
    bool kiosk = qApp->arguments().contains("--fullscreen") ||
                 qEnvironmentVariable("CLUSTER_KIOSK") == "1";
    if (kiosk) {
        window.showFullScreen();
    } else {
        window.show();
    }

    return app.exec();
}
