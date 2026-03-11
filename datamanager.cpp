#include "datamanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <QDebug>
DataManager::DataManager(QObject *parent) : QObject(parent) {
    lastIdleTime.QuadPart = 0;
    lastKernelTime.QuadPart = 0;
    lastUserTime.QuadPart = 0;
}

ULARGE_INTEGER DataManager::fileTimeToUlarge(FILETIME ft) {
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return ul;
}

void DataManager::getSystemMetrics(int &cpu, int &mem) {
    MEMORYSTATUSEX memory;
    memory.dwLength = sizeof(memory);
    GlobalMemoryStatusEx(&memory);
    mem = memory.dwMemoryLoad;

    FILETIME idle, kernel, user;
    cpu = 0;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        ULARGE_INTEGER ci = fileTimeToUlarge(idle), ck = fileTimeToUlarge(kernel), cu = fileTimeToUlarge(user);
        if (lastIdleTime.QuadPart != 0) {
            ULONGLONG tot = (ck.QuadPart - lastKernelTime.QuadPart) + (cu.QuadPart - lastUserTime.QuadPart);
            if (tot > 0) cpu = (tot - (ci.QuadPart - lastIdleTime.QuadPart)) * 100 / tot;
        }
        lastIdleTime = ci; lastKernelTime = ck; lastUserTime = cu;
    }
}

bool DataManager::isWeekActive(QString zcd, int currentWeek) {
    if (zcd.isEmpty()) return false;
    QString cleanZcd = zcd;
    cleanZcd.remove("周").remove("单").remove("双").remove("(").remove(")").remove(" ");
    QStringList parts = cleanZcd.split(",");
    for (const QString& part : parts) {
        QString trimmedPart = part.trimmed();
        if (trimmedPart.isEmpty()) continue;
        if (trimmedPart.contains("-")) {
            QStringList range = trimmedPart.split("-");
            if (range.size() == 2) {
                bool ok1, ok2;
                int start = range[0].toInt(&ok1);
                int end = range[1].toInt(&ok2);
                if (ok1 && ok2 && currentWeek >= start && currentWeek <= end) return true;
            }
        } else {
            bool ok;
            int week = trimmedPart.toInt(&ok);
            if (ok && week == currentWeek) return true;
        }
    }
    return false;
}

ScheduleData DataManager::parseSchedule(const QString &jsonFilePath, const QDate &semesterStartDate) {
    ScheduleData data;
    QDate today = QDate::currentDate();
    int dayOfWeek = today.dayOfWeek();

    // 初始化日期字符串和周次
    const QString days[] = {"未知", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"};
    data.weekDayStr = days[dayOfWeek];
    data.currentWeek = (today < semesterStartDate) ? 0 : (semesterStartDate.daysTo(today) / 7) + 1;

    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return data;
    }

    QByteArray rawData = file.readAll();
    file.close();

    if (rawData.isEmpty()) {
        return data;
    }

    // 将可能为 GBK 编码的本地数据转换为 UTF-8
    QString jsonString = QString::fromLocal8Bit(rawData);
    QByteArray jsonData = jsonString.toUtf8();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (doc.isNull() || !doc.object().contains("kbList")) {
        return data;
    }

    QJsonArray kbList = doc.object()["kbList"].toArray();
    QTime currentTime = QTime::currentTime();
    int tomorrowWeek = (dayOfWeek == 7) ? data.currentWeek + 1 : data.currentWeek;

    // 节次时间映射表
    QMap<int, QPair<QTime, QTime>> timeMap;
    timeMap[1] = {QTime(8, 0), QTime(9, 35)};
    timeMap[3] = {QTime(10, 0), QTime(11, 35)};
    timeMap[5] = {QTime(14, 0), QTime(15, 35)};
    timeMap[7] = {QTime(16, 0), QTime(17, 35)};
    timeMap[9] = {QTime(19, 0), QTime(20, 35)};

    for (const QJsonValue &val : kbList) {
        QJsonObject course = val.toObject();
        int courseDay = course["xqj"].toString().toInt();
        QString jcStr = course["jc"].toString();
        if (jcStr.isEmpty() || jcStr.split("-").size() < 2) continue;
        int startNode = jcStr.split("-")[0].toInt();
        if (!timeMap.contains(startNode)) continue;

        CourseInfo info;
        info.name = course["kcmc"].toString();
        info.location = course.contains("cdmc") && !course["cdmc"].toString().isEmpty() ? course["cdmc"].toString() : (course.contains("jxcd") ? course["jxcd"].toString() : "未知地点");
        info.startTime = timeMap[startNode].first;
        info.endTime = timeMap[startNode].second;
        if (startNode == 9 && jcStr.split("-")[1].toInt() == 11) info.endTime = QTime(21, 25);

        // 判断是否为今日课程
        if (courseDay == dayOfWeek && isWeekActive(course["zcd"].toString(), data.currentWeek)) {
            info.minutesUntilStart = currentTime.secsTo(info.startTime) / 60;
            // 只有当前时间早于下课时间，才算入“今日剩余课程”
            if (currentTime.secsTo(info.endTime) > 0) {
                info.isCurrent = (info.minutesUntilStart <= 0);
                data.todayCourses.push_back(info);
            }
        }

        // 判断是否为明日课程
        if (courseDay == today.addDays(1).dayOfWeek() && isWeekActive(course["zcd"].toString(), tomorrowWeek)) {
            info.minutesUntilStart = 0;
            info.isCurrent = false;
            data.tomorrowCourses.push_back(info);
        }
    }

    auto sortFunc = [](const CourseInfo &a, const CourseInfo &b) { return a.startTime < b.startTime; };
    std::sort(data.todayCourses.begin(), data.todayCourses.end(), sortFunc);
    std::sort(data.tomorrowCourses.begin(), data.tomorrowCourses.end(), sortFunc);

    return data;
}
