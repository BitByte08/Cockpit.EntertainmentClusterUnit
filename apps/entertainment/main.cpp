#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include "EntertainmentWindow.hpp"
#include "models/EntertainmentModel.hpp"
#include "SocketCANInterface.hpp"
#include "StubCANInterface.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationVersion(APP_VERSION);

    // 폰트 설정 (클러스터와 동일)
    const QStringList preferredFonts = {
        "Noto Sans CJK KR", "NanumGothic", "Malgun Gothic",
        "Apple SD Gothic Neo", "Noto Sans CJK JP"
    };
    for (const QString &name : preferredFonts) {
        if (QFontDatabase::hasFamily(name)) {
            app.setFont(QFont(name, 10));
            break;
        }
    }

    // CAN 인터페이스 선택: --can <interface> 또는 ENTERTAINMENT_CAN_IF 환경변수
    QString canIf;
    const QStringList args = app.arguments();
    int idx = args.indexOf("--can");
    if (idx >= 0 && idx + 1 < args.size())
        canIf = args.at(idx + 1);
    else
        canIf = qEnvironmentVariable("ENTERTAINMENT_CAN_IF");

    // 모델 생성
    auto *model = new EntertainmentModel;

    if (!canIf.isEmpty()) {
        auto can = std::make_unique<SocketCANInterface>(canIf.toStdString());
        model->setCANInterface(std::move(can));
    } else {
        // 하드웨어 없이 실행: StubCANInterface (no-op)
        auto can = std::make_unique<StubCANInterface>();
        model->setCANInterface(std::move(can));
    }
    model->startReceiving();

    // 윈도우
    EntertainmentWindow window;
    window.setModel(model);

    bool kiosk = args.contains("--fullscreen") ||
                 qEnvironmentVariable("ENTERTAINMENT_KIOSK") == "1";
    if (kiosk) window.showFullScreen();
    else        window.show();

    return app.exec();
}
