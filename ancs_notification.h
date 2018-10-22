#ifndef ANCS_NOTIFICATION_H
#define ANCS_NOTIFICATION_H

#include <QtGlobal>
#include <QString>

class ANCSNotification
{
public:
    uint eventFlags;
    uint categoryId;
    QString title;
    QString message;

    ANCSNotification();
    void show();
private:
    QString decodeIcon();
};

#endif // ANCS_NOTIFICATION_H
