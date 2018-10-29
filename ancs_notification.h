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
    bool show();
    bool refresh();
    bool hide();
    ~ANCSNotification();
private:
    uint notify(uint replaces, QString title, QString message, QString icon);
    bool shown;
    uint dbusNotificationId;

    QString decodeIcon();
};

#endif // ANCS_NOTIFICATION_H
