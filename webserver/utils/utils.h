#pragma once

struct TimeNow {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    long micros;
};

TimeNow getTimeNow();