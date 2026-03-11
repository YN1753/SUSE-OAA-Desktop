#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "datamanager.h"
#include "uimanager.h"
#include "menumanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onTick();
    void handleSyncRequest();
    void handlePythonOutput();
    void startSyncProcess(QString u, QString p);

private:
    DataManager *dataManager;
    UIManager *uiManager;
    MenuManager *menuManager;
    QTimer *refreshTimer;

   int currentLayoutIndex = 0;
    bool m_isDragging = false;
    QPoint m_dragPosition;


    void handleQuickRefresh();

};

#endif // MAINWINDOW_H
