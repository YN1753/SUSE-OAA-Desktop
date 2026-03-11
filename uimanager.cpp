#include "uimanager.h"
#include <QScrollBar>
#include <QMenu>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
UIManager::UIManager(QWidget *parentWidget) : QObject(parentWidget), parent(parentWidget) {}

void UIManager::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(parent);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ==========================================
    // 1. 构造展开模式 (Expanded)
    // ==========================================
    expandedContainer = new QWidget(parent);
    expandedContainer->setObjectName("expandedContainer");
    expandedContainer->setMinimumWidth(300);

    QVBoxLayout *expLayout = new QVBoxLayout(expandedContainer);
    expLayout->setContentsMargins(20, 20, 20, 20);
    expLayout->setSpacing(12);

    lblDateInfo = new QLabel(expandedContainer);
    lblTitle = new QLabel(expandedContainer);
    lblCountdown = new QLabel(expandedContainer);

    scrollArea = new QScrollArea(expandedContainer);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: transparent; border: none;");
    scrollArea->setMaximumHeight(220);

    scrollArea->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { background: transparent; width: 6px; border-radius: 3px; } "
        "QScrollBar::handle:vertical { background: rgba(255,255,255,0.3); border-radius: 3px; min-height: 20px; } "
        "QScrollBar::handle:vertical:hover { background: rgba(255,255,255,0.5); } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    );

    courseListWidget = new QWidget();
    courseListLayout = new QVBoxLayout(courseListWidget);
    courseListLayout->setContentsMargins(0, 0, 0, 0);
    courseListLayout->setSpacing(8);
    scrollArea->setWidget(courseListWidget);

    lblSysInfo = new QLabel(expandedContainer);

    expLayout->addWidget(lblDateInfo);
    expLayout->addWidget(lblTitle);
    expLayout->addWidget(lblCountdown);
    expLayout->addWidget(scrollArea);
    expLayout->addWidget(lblSysInfo);

    // ==========================================
    // 2. 构造灵动岛模式 (Pill)
    // ==========================================
    pillContainer = new QWidget(parent);
    pillContainer->setObjectName("pillContainer");
    QHBoxLayout *pillLayout = new QHBoxLayout(pillContainer);
    pillLayout->setContentsMargins(15, 10, 15, 10);

    pillTimeLabel = new QLabel(pillContainer);
    pillCourseLabel = new QLabel(pillContainer);

    pillLayout->addWidget(pillTimeLabel);
    pillLayout->addWidget(pillCourseLabel);

    // ==========================================
    // 3. 👇 构造大屏控制中心 (Dashboard)
    // ==========================================
    dashboardContainer = new QWidget(parent);
    dashboardContainer->setObjectName("dashboardContainer");
    dashboardContainer->setMinimumWidth(650); // 更宽的空间

    QHBoxLayout *dashMainLayout = new QHBoxLayout(dashboardContainer);
    dashMainLayout->setContentsMargins(20, 20, 20, 20);
    dashMainLayout->setSpacing(25);

    // --- 左栏：时间与课程 ---
    QVBoxLayout *leftCol = new QVBoxLayout();
    dashTimeLabel = new QLabel(dashboardContainer);
    dashDateLabel = new QLabel(dashboardContainer);
    dashCourseWidget = new QWidget();
    dashCourseLayout = new QVBoxLayout(dashCourseWidget);
    dashCourseLayout->setContentsMargins(0,0,0,0);
    dashCourseLayout->setSpacing(8);
    leftCol->addWidget(dashTimeLabel);
    leftCol->addWidget(dashDateLabel);
    leftCol->addWidget(dashCourseWidget, 1);

    // --- 中栏：待办事项 ---
    QVBoxLayout *midCol = new QVBoxLayout();
        QLabel *todoTitle = new QLabel("📋 待办事项", dashboardContainer);
        todoTitle->setStyleSheet("color: #FFA500; font-size: 15px; font-weight: bold; background: transparent; margin-bottom: 5px;");

        todoListWidget = new QListWidget(dashboardContainer);
        todoListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        todoListWidget->setContextMenuPolicy(Qt::CustomContextMenu); // 允许右键菜单

        todoInput = new QLineEdit(dashboardContainer);
        todoInput->setAttribute(Qt::WA_InputMethodEnabled, true);
        todoInput->setPlaceholderText("输入新任务，按回车添加...");

        midCol->addWidget(todoTitle);
        midCol->addWidget(todoListWidget);
        midCol->addWidget(todoInput);

        // 🌟 交互逻辑：回车添加任务
        connect(todoInput, &QLineEdit::returnPressed, [this]() {
            QString text = todoInput->text().trimmed();
            if (!text.isEmpty()) {
                addTodoItem(text, false);
                todoInput->clear();
                saveTodos(); // 保存到文件
            }
        });

        // 🌟 交互逻辑：勾选/取消勾选时自动保存
        connect(todoListWidget, &QListWidget::itemChanged, this, &UIManager::saveTodos);

        // 🌟 交互逻辑：右键菜单删除任务
        connect(todoListWidget, &QListWidget::customContextMenuRequested, [this](const QPoint &pos) {
            QListWidgetItem *item = todoListWidget->itemAt(pos);
            if (item) {
                QMenu menu;
                QAction *delAct = menu.addAction("🗑️ 删除任务");
                menu.setStyleSheet("QMenu { background-color: #282c34; color: #fff; border: 1px solid #444; } QMenu::item:selected { background-color: #FF4B4B; color: #fff; }");
                if (menu.exec(todoListWidget->mapToGlobal(pos)) == delAct) {
                    delete todoListWidget->takeItem(todoListWidget->row(item));
                    saveTodos();
                }
            }
        });

        loadTodos(); // 界面加载时读取本地记录

    // --- 右栏：硬件监控 ---
    QVBoxLayout *rightCol = new QVBoxLayout();
    QLabel *hwTitle = new QLabel("🖥️ 硬件监控", dashboardContainer);
    hwTitle->setStyleSheet("color: #00FFFF; font-size: 15px; font-weight: bold; background: transparent; margin-bottom: 5px;");

    dashCpuLabel = new QLabel("CPU 使用率: --%", dashboardContainer);
    dashCpuBar = new QProgressBar(dashboardContainer);
    dashCpuBar->setFixedHeight(12);
    dashCpuBar->setTextVisible(false);

    dashMemLabel = new QLabel("内存占用: --%", dashboardContainer);
    dashMemBar = new QProgressBar(dashboardContainer);
    dashMemBar->setFixedHeight(12);
    dashMemBar->setTextVisible(false);

    rightCol->addWidget(hwTitle);
    rightCol->addWidget(dashCpuLabel);
    rightCol->addWidget(dashCpuBar);
    rightCol->addSpacing(15);
    rightCol->addWidget(dashMemLabel);
    rightCol->addWidget(dashMemBar);
    rightCol->addStretch();

    // 组合三栏
    dashMainLayout->addLayout(leftCol, 4);
    dashMainLayout->addLayout(midCol, 3);
    dashMainLayout->addLayout(rightCol, 3);

    // ==========================================
    // 添加到主窗口并初始化
    mainLayout->addWidget(expandedContainer);
    mainLayout->addWidget(pillContainer);
    mainLayout->addWidget(dashboardContainer);

    switchStyle(UIStyle::Clean);
    switchLayout(UILayoutMode::Expanded);
}

void UIManager::switchLayout(UILayoutMode mode) {
    currentMode = mode;
    expandedContainer->hide();
    pillContainer->hide();
    dashboardContainer->hide();

    if (mode == UILayoutMode::Expanded) expandedContainer->show();
    else if (mode == UILayoutMode::Pill) pillContainer->show();
    else if (mode == UILayoutMode::Dashboard) dashboardContainer->show();

    parent->window()->adjustSize();
}

void UIManager::switchStyle(UIStyle style) {
    currentStyle = style;
    QString mainBg, pillBg, sysColor, titleColor, timeColor;

    switch (style) {
        case UIStyle::Minimalist:
            mainBg = "background-color: rgba(240, 240, 240, 220); border-radius: 15px;";
            pillBg = "background-color: rgba(240, 240, 240, 230); border-radius: 20px;";
            sysColor = "#666666"; titleColor = "#333333"; timeColor = "#FF4B4B";
            break;
        case UIStyle::Cyberpunk:
            mainBg = "background-color: rgba(10, 10, 10, 230); border: 1px solid #00FFFF; border-radius: 10px;";
            pillBg = "background-color: rgba(10, 10, 10, 240); border: 1px solid #FF00FF; border-radius: 20px;";
            sysColor = "#00FFFF"; titleColor = "#FFFFFF"; timeColor = "#FF00FF";
            break;
        case UIStyle::Clean:
        default:
            mainBg = "background-color: rgba(43, 45, 48, 220); border-radius: 15px;";
            pillBg = "background-color: rgba(43, 45, 48, 240); border: 1px solid #FFA500; border-radius: 20px;";
            sysColor = "#00FFFF"; titleColor = "#FFFFFF"; timeColor = "#FFA500";
            break;
    }

    expandedContainer->setStyleSheet(QString("#expandedContainer { %1 }").arg(mainBg));
    pillContainer->setStyleSheet(QString("#pillContainer { %1 }").arg(pillBg));
    dashboardContainer->setStyleSheet(QString("#dashboardContainer { %1 }").arg(mainBg));

    lblSysInfo->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; background: transparent; border-top: 1px solid rgba(255,255,255,20); padding-top: 10px; margin-top: 5px;").arg(sysColor));

    dashTimeLabel->setStyleSheet(QString("color: %1; font-size: 46px; font-weight: bold; background: transparent; font-family: 'Consolas', monospace;").arg(timeColor));
    dashDateLabel->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold; background: transparent; margin-bottom: 15px;").arg(titleColor));

    QString listStyle = QString(
            "QListWidget { background: transparent; border: none; outline: none; }"
            "QListWidget::item { color: %1; font-size: 14px; padding: 4px; font-family: 'Microsoft YaHei', sans-serif; }"
            "QListWidget::item:hover { background: rgba(255,255,255,0.1); border-radius: 4px; }"
            "QListWidget::indicator { width: 16px; height: 16px; border-radius: 4px; border: 1px solid %2; background: rgba(0,0,0,0.2); }"
            "QListWidget::indicator:checked { background-color: %2; }"
        ).arg(titleColor, timeColor);
        todoListWidget->setStyleSheet(listStyle);

        // 🌟 给底部的输入框配色
        QString inputStyle = QString(
            "QLineEdit { background: rgba(0,0,0,0.2); border: 1px solid rgba(255,255,255,0.2); border-radius: 5px; color: %1; padding: 6px; font-family: 'Microsoft YaHei', sans-serif; }"
            "QLineEdit:focus { border: 1px solid %2; }"
        ).arg(titleColor, timeColor);
        todoInput->setStyleSheet(inputStyle);

    dashCpuLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold; background: transparent; font-family: 'Microsoft YaHei', sans-serif;").arg(sysColor));
    dashMemLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold; background: transparent; font-family: 'Microsoft YaHei', sans-serif;").arg(sysColor));

    QString barStyle = QString(
        "QProgressBar { border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; background: rgba(0,0,0,0.2); }"
        "QProgressBar::chunk { background-color: %1; border-radius: 5px; }"
    ).arg(timeColor);
    dashCpuBar->setStyleSheet(barStyle);
    dashMemBar->setStyleSheet(barStyle);
}

void UIManager::clearLayout(QVBoxLayout *layout) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
}

void UIManager::addCourseItemToLayout(QVBoxLayout *layout, const QString &text, const QString &color, bool centerAlign, Qt::Alignment alignment) {
    QLabel *label = new QLabel(text);
    label->setStyleSheet(QString("color: %1; font-size: 14px; background: transparent; font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;").arg(color));
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    if (centerAlign) label->setAlignment(alignment | Qt::AlignHCenter);
    else label->setAlignment(alignment);
    layout->addWidget(label);
}



void UIManager::updateContent(const ScheduleData &data, int cpu, int mem) {
    QTime currentTime = QTime::currentTime();
    QString timeStr = currentTime.toString("hh:mm:ss");
    QString shortTimeStr = currentTime.toString("hh:mm");
    QDate today = QDate::currentDate();
    QDate tomorrowDate = today.addDays(1);

    // 🌟 静态变量：用于记录状态，判定是否需要“重画”布局
    static int lastMinute = -1;
    static UIStyle lastStyle = (UIStyle)-1; // 初始化为一个不存在的值

    // 🚥 【主题颜色矩阵】
    QString cActive, cWait, cDim, cLine, cTitle, cSys;
    if (currentStyle == UIStyle::Minimalist) {
        cActive = "#FF4B4B"; cWait = "#333333"; cDim = "#888888"; cLine = "rgba(0,0,0,0.1)";
        cTitle = "#333333"; cSys = "#666666";
    } else if (currentStyle == UIStyle::Cyberpunk) {
        cActive = "#FF00FF"; cWait = "#00FFFF"; cDim = "#00AAAA"; cLine = "rgba(0,255,255,0.4)";
        cTitle = "#FFFFFF"; cSys = "#00FFFF";
    } else {
        cActive = "#00FF00"; cWait = "#FFA500"; cDim = "#aaaaaa"; cLine = "rgba(255,165,0,0.4)";
        cTitle = "#FFFFFF"; cSys = "#00FFFF";
    }

    // ==========================================
    // A. 实时更新部分（不触发布局重排，不打断输入）
    // ==========================================

    // 1. 更新标签文本
    lblSysInfo->setText(QString("🖥️ CPU: %1% | 💾 MEM: %2%").arg(cpu).arg(mem));
    dashCpuLabel->setText(QString("CPU 使用率: %1%").arg(cpu));
    dashMemLabel->setText(QString("内存占用: %1%").arg(mem));
    dashTimeLabel->setText(timeStr);
    pillTimeLabel->setText(shortTimeStr);

    // 2. 更新进度条数值
    dashCpuBar->setValue(cpu);
    dashMemBar->setValue(mem);

    // 3. 更新倒计时数值与实时颜色
    QString countdownText = timeStr;
    QString countdownColor = cDim;
    if (!data.todayCourses.isEmpty()) {
        CourseInfo nextCourse = data.todayCourses.first();
        int remainingSecs = nextCourse.isCurrent ? currentTime.secsTo(nextCourse.endTime) : currentTime.secsTo(nextCourse.startTime);
        int h = remainingSecs / 3600; int m = (remainingSecs % 3600) / 60; int s = remainingSecs % 60;
        countdownText = QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        countdownColor = nextCourse.isCurrent ? cActive : cWait;
    }
    lblCountdown->setText(countdownText);
    lblCountdown->setStyleSheet(QString("color: %1; font-size: 38px; font-weight: bold; background: transparent; font-family: 'Consolas', monospace;").arg(countdownColor));

    // 🌟【针对灵动岛的实时颜色修复】
    pillTimeLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold; background: transparent;").arg(countdownColor));

    // ==========================================
    // B. 刷新判定：分钟变化 或 风格变化 时，执行重排重画
    // ==========================================
    if (currentTime.minute() != lastMinute || currentStyle != lastStyle) {
        lastMinute = currentTime.minute();
        lastStyle = currentStyle; // 更新记录风格

        // 只有在这里才执行 clearLayout 和 setStyleSheet 重设
        clearLayout(courseListLayout);
        clearLayout(dashCourseLayout);

        // 重新应用标题和日期样式（防止看不清）
        lblDateInfo->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; background: transparent;").arg(cDim));
        lblTitle->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold; background: transparent; font-family: 'Microsoft YaHei', sans-serif;").arg(cTitle));
        dashDateLabel->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold; background: transparent; margin-bottom: 15px;").arg(cTitle));
        pillCourseLabel->setStyleSheet(QString("color: %1; font-size: 14px; margin-left: 10px; background: transparent;").arg(cTitle));

        if (data.currentWeek == 0) lblDateInfo->setText(QString("尚未开学 (%1)").arg(data.weekDayStr));
        else lblDateInfo->setText(QString("第 %1 周  %2").arg(data.currentWeek).arg(data.weekDayStr));
        dashDateLabel->setText(QString("%1  %2").arg(today.toString("yyyy年M月d日")).arg(data.weekDayStr));

        QString titleText = "▶ 今日无课";
        QString pillCourseText = "🏖️ 自由时间";

        auto addCourseText = [&](const QString &text, const QString &color, bool centerAlign = false) {
            addCourseItemToLayout(courseListLayout, text, color, centerAlign);
            addCourseItemToLayout(dashCourseLayout, text, color, centerAlign);
        };
        auto addLine = [&](const QString &colorStr) {
            QLabel *l1 = new QLabel(); l1->setFixedHeight(1); l1->setStyleSheet(QString("background-color: %1; margin: 8px 0;").arg(colorStr)); courseListLayout->addWidget(l1);
            QLabel *l2 = new QLabel(); l2->setFixedHeight(1); l2->setStyleSheet(QString("background-color: %1; margin: 8px 0;").arg(colorStr)); dashCourseLayout->addWidget(l2);
        };
        auto addTitle = [&](const QString &text, const QString &color) {
            QLabel *l1 = new QLabel(text); l1->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; margin-bottom: 5px; background: transparent;").arg(color)); courseListLayout->addWidget(l1);
            QLabel *l2 = new QLabel(text); l2->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; margin-bottom: 5px; background: transparent;").arg(color)); dashCourseLayout->addWidget(l2);
        };

        if (!data.todayCourses.isEmpty()) {
            CourseInfo nextCourse = data.todayCourses.first();
            titleText = nextCourse.isCurrent ? QString("▶ %1进行中").arg(nextCourse.name) : QString("▶ 距离%1还有：").arg(nextCourse.name);
            pillCourseText = nextCourse.isCurrent ? QString("🔴 %1").arg(nextCourse.name) : QString("🔵 下一节: %1").arg(nextCourse.name);

            QString locStr = QString(" %1").arg(nextCourse.location);
            QLabel *loc1 = new QLabel(locStr); loc1->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold; background: transparent;").arg(cActive)); loc1->setWordWrap(true); courseListLayout->addWidget(loc1);
            QLabel *loc2 = new QLabel(locStr); loc2->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold; background: transparent;").arg(cActive)); loc2->setWordWrap(true); dashCourseLayout->addWidget(loc2);

            addLine(cLine);
            addTitle("📅 今日剩余课程", cWait);
            for (int i = 0; i < data.todayCourses.size(); ++i) {
                const auto &c = data.todayCourses[i];
                QString textColor = c.isCurrent ? cActive : "#e0e0e0";
                QString suffix = c.isCurrent ? QString(" <span style='color:%1; font-size:12px;'>(剩%2分)</span>").arg(cActive).arg(currentTime.secsTo(c.endTime)/60) : "";
                QString text = QString("%1 - %2 <span style='opacity:0.8'>(%3)</span>%4").arg(c.startTime.toString("hh:mm")).arg(c.name).arg(c.location).arg(suffix);
                addCourseText(text, textColor);
            }
        }

        if (!data.tomorrowCourses.isEmpty()) {
            if (data.todayCourses.isEmpty()) {
                titleText = QString("🌙 明日预告 (%1)").arg(tomorrowDate.toString("M月d日"));
                pillCourseText = QString("🌙 明日首节: %1").arg(data.tomorrowCourses.first().name);
                addTitle(QString("📅 %1 课程表").arg(tomorrowDate.toString("M月d日")), cWait);
            } else {
                addLine(cLine);
                addTitle(QString("🌙 明日课程 (%1)").arg(tomorrowDate.toString("M月d日")), cWait);
            }
            for (const auto &c : data.tomorrowCourses) {
                QString text = QString("%1 - %2 <span style='opacity:0.6'>(%3)</span>").arg(c.startTime.toString("hh:mm")).arg(c.name).arg(c.location);
                addCourseText(text, cDim);
            }
        } else if (data.todayCourses.isEmpty()) {
            pillCourseText = "🎉 连续休息";
            addCourseText("🎉 连续休息！<br>今天和明天都没有课程", cDim, true);
        }

        courseListLayout->addStretch();
        dashCourseLayout->addStretch();

        lblTitle->setText(titleText);
        pillCourseLabel->setText(pillCourseText);

        // 🌟 更新 Dashboard 静态样式
        if (!todoInput->hasFocus()) {
            // 这里可以放置待办事项相关的 setStyleSheet 逻辑
        }
    }
}
// 🌟 核心：添加待办条目
void UIManager::addTodoItem(const QString &text, bool isDone) {
    todoListWidget->blockSignals(true); // 暂时屏蔽信号，防止初始化时触发重复保存

    QListWidgetItem *item = new QListWidgetItem(text, todoListWidget);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setCheckState(isDone ? Qt::Checked : Qt::Unchecked);

    todoListWidget->blockSignals(false);
}

// 🌟 读取本地 JSON
void UIManager::loadTodos() {
    todoListWidget->clear();
    QFile file("todos.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray arr = doc.array();
        for (const QJsonValue &v : arr) {
            QJsonObject obj = v.toObject();
            addTodoItem(obj["text"].toString(), obj["done"].toBool());
        }
        file.close();
    }
}

// 🌟 保存到本地 JSON
void UIManager::saveTodos() {
    QJsonArray arr;
    for (int i = 0; i < todoListWidget->count(); ++i) {
        QListWidgetItem *item = todoListWidget->item(i);
        QJsonObject obj;
        obj["text"] = item->text();
        obj["done"] = (item->checkState() == Qt::Checked);
        arr.append(obj);
    }
    QFile file("todos.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
}
void UIManager::setupLoginUI() {
    if (!loginWindow) {
        loginWindow = new QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        loginWindow->setAttribute(Qt::WA_TranslucentBackground);
        loginWindow->setFixedSize(320, 380);

        QVBoxLayout *layout = new QVBoxLayout(loginWindow);
        QWidget *bg = new QWidget(loginWindow);
        bg->setObjectName("loginBg");

        // 🌟 核心样式：圆角、深色背景、橙色边框
        bg->setStyleSheet("#loginBg { background-color: #2b2d30; border-radius: 20px; border: 2px solid #FFA500; }");

        QVBoxLayout *vbox = new QVBoxLayout(bg);
        vbox->setContentsMargins(30, 40, 30, 40);
        vbox->setSpacing(15);

        QLabel *title = new QLabel("教务系统登录");
        title->setStyleSheet("color: white; font-size: 22px; font-weight: bold; background: transparent;");
        title->setAlignment(Qt::AlignCenter);

        editUser = new QLineEdit();
        editUser->setPlaceholderText("请输入学号");
        editPass = new QLineEdit();
        editPass->setPlaceholderText("请输入密码");
        editPass->setEchoMode(QLineEdit::Password);

        QString editStyle = "QLineEdit { background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.2); border-radius: 10px; color: white; padding: 12px; font-size: 14px; } "
                            "QLineEdit:focus { border: 1px solid #FFA500; }";
        editUser->setStyleSheet(editStyle);
        editPass->setStyleSheet(editStyle);

        chkSavePass = new QCheckBox("记住密码");
        chkSavePass->setStyleSheet("QCheckBox { color: #aaaaaa; font-size: 13px; background: transparent; } QCheckBox::indicator { width: 16px; height: 16px; }");

        QPushButton *btnLogin = new QPushButton("登 录");
        btnLogin->setCursor(Qt::PointingHandCursor);
        btnLogin->setStyleSheet("QPushButton { background: #FFA500; color: #1e1e1e; border-radius: 10px; font-size: 16px; font-weight: bold; padding: 12px; } "
                                "QPushButton:hover { background: #FFB732; } "
                                "QPushButton:pressed { background: #E69500; }");

        QPushButton *btnClose = new QPushButton("✕");
        btnClose->setFixedSize(30, 30);
        btnClose->setStyleSheet("QPushButton { color: #888; border: none; font-size: 18px; background: transparent; } QPushButton:hover { color: white; }");

        // 布局组装
        vbox->addWidget(title);
        vbox->addSpacing(10);
        vbox->addWidget(editUser);
        vbox->addWidget(editPass);
        vbox->addWidget(chkSavePass);
        vbox->addSpacing(15);
        vbox->addWidget(btnLogin);

        layout->addWidget(bg);

        // 关闭逻辑
        connect(btnClose, &QPushButton::clicked, loginWindow, &QWidget::hide); // 🌟 修改为 clicked

        // 登录逻辑
        connect(btnLogin, &QPushButton::clicked, [this]() {
            saveConfig(editUser->text(), editPass->text(), chkSavePass->isChecked());
            loginWindow->hide();
            // 🌟 技巧：通过父窗口调用同步函数
            QMetaObject::invokeMethod(parent->window(), "startSyncProcess",
                                    Q_ARG(QString, editUser->text()),
                                    Q_ARG(QString, editPass->text()));
        });
    }

    // 填充历史数据
    QString u, p; bool s;
    loadConfig(u, p, s);
    editUser->setText(u);
    chkSavePass->setChecked(s);
    if(s) editPass->setText(p);

    loginWindow->show();
    loginWindow->raise();
    loginWindow->activateWindow();
}


void UIManager::saveConfig(QString user, QString pass, bool save) {
    QJsonObject obj;
    obj["user"] = user;
    obj["pass"] = save ? pass : ""; // 如果不记住密码则存空
    obj["save"] = save;

    QFile file("config.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

void UIManager::loadConfig(QString &user, QString &pass, bool &save) {
    QFile file("config.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        user = obj["user"].toString();
        pass = obj["pass"].toString();
        save = obj["save"].toBool();
        file.close();
    }
}
