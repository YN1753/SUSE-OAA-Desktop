#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <QObject>
#include <QMenu>
#include <QPoint>

class MenuManager : public QObject {
    Q_OBJECT
public:
    explicit MenuManager(QWidget *parent = nullptr);
    void showMenu(const QPoint &pos);

signals:
    void requestSync();
    void requestStyleChange(int styleIndex);
    void requestLayoutChange(int layoutIndex);
    void  requestQuickSync();

private:
    QWidget *parentWidget;
};

#endif // MENUMANAGER_H
