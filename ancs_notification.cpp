#include "ancs_notification.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusArgument>

#include "common.h"
#include "ancs_protocol_constants.h"

ANCSNotification::ANCSNotification() : eventFlags(0), categoryId(ANCS_CATEGORY_ID_OTHER) {}

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

void ANCSNotification::show()
{
    QString appName = "";
    QString appIcon = decodeIcon();
    QVariantMap hints;
    hints.insert("x-nemo-preview-body", message);
    hints.insert("x-nemo-preview-summary", title);
    hints.insert("x-nemo-feedback", "notif_strong");
    hints.insert("urgency", 3);
    hints.insert("transient", true);

    QList<QVariant> argumentList;
    argumentList << appName;
    argumentList << (uint) 0;
    argumentList << appIcon;
    argumentList << title;
    argumentList << message;
    argumentList << QStringList();
    argumentList << hints;
    argumentList << (int) 5;

    static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE,
                                    NOTIFICATIONS_MAIN_IFACE);
    notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
}
