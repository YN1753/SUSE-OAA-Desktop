#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QTime>
#include <QDate>
#include <QString>
#include <QVector>
#include <QMap>
#include <windows.h>

// 提取数据结构
struct CourseInfo {
    QString name;
    QString location;
    QTime startTime;
    QTime endTime;
    int minutesUntilStart;
    bool isCurrent;
};

struct ScheduleData {
    int currentWeek;
    QString weekDayStr;
    QVector<CourseInfo> todayCourses;
    QVector<CourseInfo> tomorrowCourses;
};

class DataManager : public QObject {
    Q_OBJECT
public:
    explicit DataManager(QObject *parent = nullptr);

    // 获取系统硬件状态
    void getSystemMetrics(int &cpu, int &mem);

    // 解析课表数据
    ScheduleData parseSchedule(const QString &jsonFilePath, const QDate &semesterStartDate);

private:
    ULARGE_INTEGER lastIdleTime, lastKernelTime, lastUserTime;
    ULARGE_INTEGER fileTimeToUlarge(FILETIME ft);
    bool isWeekActive(QString zcd, int currentWeek);
};

#endif // DATAMANAGER_H
