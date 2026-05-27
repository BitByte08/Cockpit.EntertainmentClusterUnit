#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QFile>
#include <QCoreApplication>
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

    bool canOk = false;
    if (!canIf.isEmpty()) {
        try {
            model->setCANInterface(std::make_unique<SocketCANInterface>(canIf.toStdString()));
            canOk = true;
        } catch (const std::exception &) {
            model->setCANInterface(std::make_unique<StubCANInterface>());
        }
    } else {
        // 하드웨어 없이 실행: StubCANInterface (no-op)
        model->setCANInterface(std::make_unique<StubCANInterface>());
    }
    model->startReceiving();

    // 윈도우
    EntertainmentWindow window;
    window.setModel(model);
    if (canOk)
        window.setCanStatus(true, QStringLiteral("%1  ●").arg(canIf));
    else
        window.setCanStatus(false, "stub  ○");

    // road_graph.json 자동 로드 (있으면 네비 모드, 없으면 위성 모드)
    QString graphPath = QCoreApplication::applicationDirPath() + "/road_graph.json";
    if (QFile::exists(graphPath))
        window.loadRoadGraph(graphPath);

    bool kiosk = args.contains("--fullscreen") ||
                 qEnvironmentVariable("ENTERTAINMENT_KIOSK") == "1";
    if (kiosk) window.showFullScreen();
    else        window.show();

    return app.exec();
}
