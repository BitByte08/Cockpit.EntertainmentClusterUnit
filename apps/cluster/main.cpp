#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include "mainwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationVersion(APP_VERSION);

    // 한글 폰트 설정 (Noto Sans CJK KR 선호, 없으면 시스템 기본)
    QStringList preferredFonts = {
        "Noto Sans CJK KR",
        "NanumGothic",
        "Malgun Gothic",
        "Apple SD Gothic Neo",
        "Noto Sans CJK JP",
        "Noto Sans CJK SC",
        "Noto Sans CJK TC"
    };
    QFont appFont;
    for (const QString &name : preferredFonts) {
        if (QFontDatabase::hasFamily(name)) {
            appFont = QFont(name, 10);
            break;
        }
    }
    app.setFont(appFont);

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
