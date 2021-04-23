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
    bool isNew;

    ANCSNotification();
    bool show(bool feedback);
    bool refresh(bool feedback);
    bool hide();
    ~ANCSNotification();
private:
    uint notify(uint replaces, QString title, QString message, QString icon, bool feedback);
    bool shown;
    uint dbusNotificationId;

    QString decodeIcon();
};

#endif // ANCS_NOTIFICATION_H
