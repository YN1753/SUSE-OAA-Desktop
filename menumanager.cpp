#include "menumanager.h"
#include <QApplication>

MenuManager::MenuManager(QWidget *parent) : QObject(parent), parentWidget(parent) {}

void MenuManager::showMenu(const QPoint &pos) {
    QMenu menu(parentWidget);
    menu.setStyleSheet("QMenu { background-color: #282c34; color: #fff; border: 1px solid #444; } QMenu::item:selected { background-color: #FFA500; color: #000; }");

    QAction *syncAction = menu.addAction("📅 同步教务课表");
    menu.addSeparator();

    QMenu *layoutMenu = menu.addMenu("↔️ 切换布局");
    QAction *layoutExp = layoutMenu->addAction("经典竖版 (Expanded)");
    QAction *layoutPill = layoutMenu->addAction("微型灵动岛 (Pill)");
    QAction *layoutDash = layoutMenu->addAction("大屏控制中心 (Dashboard)");

    QMenu *styleMenu = menu.addMenu("🎨 切换风格");
    QAction *styleMin = styleMenu->addAction("现代极简风 (毛玻璃)");
    QAction *styleCyb = styleMenu->addAction("科技赛博朋克 (霓虹)");
    QAction *styleCle = styleMenu->addAction("清新实用风 (高对比)");
    QAction *quickRefreshAction = menu.addAction("🚀 快速刷新课表");

    connect(quickRefreshAction, &QAction::triggered, [this]() {
        // 逻辑：读取 config.json 中的账号密码
        // 然后调用 MainWindow 里的 handleSyncRequest，但不弹出输入框
        emit requestQuickSync(); // 需要在 signals 中定义此信号
    });

    menu.addSeparator();
    QAction *exitAction = menu.addAction("❌ 退出程序");

    // 信号连接
    connect(syncAction, &QAction::triggered, this, &MenuManager::requestSync);
    connect(layoutExp, &QAction::triggered, this, [this](){ emit requestLayoutChange(0); });
    connect(layoutPill, &QAction::triggered, this, [this](){ emit requestLayoutChange(1); });
    connect(layoutDash, &QAction::triggered, this, [this](){ emit requestLayoutChange(2); });

    connect(styleMin, &QAction::triggered, this, [this](){ emit requestStyleChange(0); });
    connect(styleCyb, &QAction::triggered, this, [this](){ emit requestStyleChange(1); });
    connect(styleCle, &QAction::triggered, this, [this](){ emit requestStyleChange(2); });

    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    menu.exec(pos);
}
