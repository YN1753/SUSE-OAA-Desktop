#include "mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QMouseEvent>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    // 初始化各个模块
    dataManager = new DataManager(this);
    menuManager = new MenuManager(this);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    uiManager = new UIManager(central);
    uiManager->setupUI();

    // 绑定菜单信号
    connect(menuManager, &MenuManager::requestSync, this, &MainWindow::handleSyncRequest);
//    connect(menuManager, &MenuManager::requestLayoutToggle, this, [this]() {
//        isPillMode = !isPillMode;
//        uiManager->switchLayout(isPillMode ? UILayoutMode::Pill : UILayoutMode::Expanded);
//    });
    connect(menuManager, &MenuManager::requestStyleChange, this, [this](int id) {
        uiManager->switchStyle(static_cast<UIStyle>(id));
        this->onTick();
    });
    connect(menuManager, &MenuManager::requestLayoutChange, this, [this](int id) {
            currentLayoutIndex = id;
            UILayoutMode targetMode = UILayoutMode::Expanded;
            if (id == 1) targetMode = UILayoutMode::Pill;
            else if (id == 2) targetMode = UILayoutMode::Dashboard;

            uiManager->switchLayout(targetMode);
        });
    //
    connect(menuManager, &MenuManager::requestQuickSync, this, [this]() {
        QString u, p; bool s;
        uiManager->loadConfig(u, p, s);
        if (u.isEmpty() || p.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先登录并保存账号密码！");
            return;
        }
        // 调用原有的 QProcess 同步逻辑
        startSyncProcess(u, p);
    });

    // 启动定时器
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::onTick);
    refreshTimer->start(1000);

    onTick(); // 立即执行一次
}

MainWindow::~MainWindow() {}

void MainWindow::onTick() {
    int cpu = 0, mem = 0;
    dataManager->getSystemMetrics(cpu, mem);

    // 假设开学日期为 2026年3月9日，你可以后期把它写进配置文件
    ScheduleData schedule = dataManager->parseSchedule("schedule.json", QDate(2026, 3, 9));

    uiManager->updateContent(schedule, cpu, mem);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *e) {
    menuManager->showMenu(e->globalPos());
}

void MainWindow::handlePythonOutput() {
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;

    QByteArray res = proc->readAllStandardOutput();
    if (!res.isEmpty() && !res.contains("FAILED") && !res.contains("Error")) {
        QFile f("schedule.json");
        if (f.open(QIODevice::WriteOnly)) {
            f.write(res);
            f.close();
            QMessageBox::information(this, "成功", "课表同步成功！");
            onTick();
        }
    } else {
        QMessageBox::warning(this, "失败", "登录失败或网络超时");
    }
    proc->deleteLater();
}

// 拖拽窗口逻辑
void MainWindow::mousePressEvent(QMouseEvent *e) {
    if(e->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = e->globalPos() - frameGeometry().topLeft();
    }
}
void MainWindow::mouseMoveEvent(QMouseEvent *e) {
    if(m_isDragging) move(e->globalPos() - m_dragPosition);
}
void MainWindow::mouseReleaseEvent(QMouseEvent *) {
    m_isDragging = false;
}

void MainWindow::handleSyncRequest() {
    uiManager->setupLoginUI();
}

//快速刷新逻辑（不弹窗，直接用存好的密码跑）
void MainWindow::handleQuickRefresh() {
    QString u, p; bool s;
    uiManager->loadConfig(u, p, s);
    if (u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, "提示", "尚未保存账号密码，请先登录！");
        uiManager->setupLoginUI();
    } else {
        startSyncProcess(u, p);
    }
}

// 3. 核心同步进程
void MainWindow::startSyncProcess(QString u, QString p) {
    QProcess *proc = new QProcess(this);
    QString exePath = QCoreApplication::applicationDirPath() + "/main.exe";

    // 连接结果处理
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handlePythonOutput);

    proc->start(exePath, QStringList() << u << p);
}
