#include "ancs_notification.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusArgument>

#include "common.h"
#include "ancs_protocol_constants.h"

ANCSNotification::ANCSNotification() : eventFlags(0), categoryId(ANCS_CATEGORY_ID_OTHER),
    shown(false), dbusNotificationId(0) {}

QString ANCSNotification::decodeIcon()
{
    QString result;
    switch (categoryId) {

    case ANCS_CATEGORY_ID_INCOMING_CALL:
        return "ios-call-outline";

    case ANCS_CATEGORY_ID_MISSED_CALL:
        return "ios-phone-landscape";

    case ANCS_CATEGORY_ID_VOICEMAIL:
        return "ios-disc-outline";

    case ANCS_CATEGORY_ID_SOCIAL:
        return "ios-person-outline";

    case ANCS_CATEGORY_ID_SCHEDULE:
        return "ios-calendar-outline";

    case ANCS_CATEGORY_ID_EMAIL:
        return "ios-mail-outline";

    case ANCS_CATEGORY_ID_NEWS:
        return "ios-paper-outline";

    case ANCS_CATEGORY_ID_HEALTH_AND_FITNESS:
        return "ios-heart-outline";

    case ANCS_CATEGORY_ID_BUSINESS_AND_FINANCE:
        return "ios-briefcase";

    case ANCS_CATEGORY_ID_LOCATION:
        return "ios-compass-outline";

    case ANCS_CATEGORY_ID_ENTERTAINMENT:
        return "ios-film-outline";

    default:
        return "ios-notifications-outline";
    }
}

bool ANCSNotification::show()
{
    if (shown)
        return true;

    uint result = notify(0, title, message, decodeIcon());
    if (result) {
        shown = true;
        dbusNotificationId = result;
        return true;
    }
    return false;
}

bool ANCSNotification::hide()
{
    if (!shown)
        return true;
    QList<QVariant> argumentList;
    argumentList << dbusNotificationId;

    static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE,
                                    NOTIFICATIONS_MAIN_IFACE);
    QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "CloseNotification",
                                                        argumentList);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "CloseNotification D-Bus call returned error" << reply.errorMessage();
        return false;
    }
    shown = false;
    return true;
}

bool ANCSNotification::refresh()
{
    if (!shown) {
        return show();
    }
    uint result = notify(dbusNotificationId, title, message, decodeIcon());
    return result != 0;
}

uint ANCSNotification::notify(uint replaces, QString title, QString message, QString icon)
{
    QString appName = "";
    QString appIcon = decodeIcon();
    QVariantMap hints;
    hints.insert("x-nemo-preview-body", message);
    hints.insert("x-nemo-preview-summary", title);
    hints.insert("x-nemo-feedback", "notif_strong");
    hints.insert("urgency", 3);

    QList<QVariant> argumentList;
    argumentList << appName;
    argumentList << (uint) 0;
    argumentList << appIcon;
    argumentList << title;
    argumentList << message;
    argumentList << QStringList();
    argumentList << hints;
    argumentList << (int) 0;

    static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE,
                                    NOTIFICATIONS_MAIN_IFACE);
    QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Notify D-Bus call returned error:" << reply.errorMessage();
        return 0;
    }
    if (reply.arguments().size() < 1) {
        qWarning() << "Notify D-Bus call successful but no reply arguments";
        return 0;
    }
    return reply.arguments()[0].toUInt();
}

ANCSNotification::~ANCSNotification()
{
    hide();
}
