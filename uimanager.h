#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QProgressBar> // 新增：进度条
#include <QCheckBox>    // 新增：复选框
#include <QListWidget> // 🌟 新增：动态列表
#include <QLineEdit>
#include "datamanager.h"
#include <qpushbutton.h>

enum class UIStyle { Minimalist, Cyberpunk, Clean };
enum class UILayoutMode { Expanded, Pill, Dashboard }; // 新增：Dashboard模式

class UIManager : public QObject {
    Q_OBJECT
public:
    explicit UIManager(QWidget *parentWidget);

    void setupUI();
    void switchStyle(UIStyle style);
    void switchLayout(UILayoutMode mode);
    void updateContent(const ScheduleData &data, int cpu, int mem);
    void setupLoginUI();
    void saveConfig(QString user, QString pass, bool save);
    void loadConfig(QString &user, QString &pass, bool &save);

private:
    QWidget *parent;
    UIStyle currentStyle = UIStyle::Clean;
    UILayoutMode currentMode = UILayoutMode::Expanded;

    // 展开模式的控件
    QWidget *expandedContainer;
    QLabel *lblDateInfo;
    QLabel *lblTitle;
    QLabel *lblCountdown;
    QScrollArea *scrollArea;
    QWidget *courseListWidget;
    QVBoxLayout *courseListLayout;
    QLabel *lblSysInfo;

    // 灵动岛模式的控件
    QWidget *pillContainer;
    QLabel *pillTimeLabel;
    QLabel *pillCourseLabel;

    //大屏控制中心 (Dashboard) 模式的控件
    QWidget *dashboardContainer;
    QLabel *dashTimeLabel;
    QLabel *dashDateLabel;
    QWidget *dashCourseWidget;
    QVBoxLayout *dashCourseLayout;

    QListWidget *todoListWidget;
    QLineEdit *todoInput;

    QLabel *dashCpuLabel;
    QProgressBar *dashCpuBar;
    QLabel *dashMemLabel;
    QProgressBar *dashMemBar;

    void loadTodos();
    void saveTodos();
    void addTodoItem(const QString &text, bool isDone);

    QWidget *loginWindow = nullptr;
    QLineEdit *editUser;
    QLineEdit *editPass;
    QCheckBox *chkSavePass;



    // 辅助函数：修改为可指定向哪个布局添加元素
    void addCourseItemToLayout(QVBoxLayout *layout, const QString &text, const QString &color, bool centerAlign = false, Qt::Alignment alignment = Qt::AlignLeft);
    void clearLayout(QVBoxLayout *layout);
};

#endif // UIMANAGER_H
