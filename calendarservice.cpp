/*
 * Copyright (C) 2016 - Florent Revest <revestflo@gmail.com>
 *               2015 - Chris Adams <chris.adams@jollamobile.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDBusInterface>
#include <QDBusMessage>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QBuffer>
#include <QDataStream>
#include <QTextStream>
#include <QtDebug>

#include <memorycalendar.h>
#include <extendedcalendar.h>
#include <extendedstorage.h>
#include <icalformat.h>
#include <vcalformat.h>
#include <incidence.h>
#include <event.h>
#include <todo.h>
#include <journal.h>
#include <attendee.h>
#include <kdatetime.h>

#include "calendarservice.h"
#include "characteristic.h"
#include "common.h"

#ifdef CALENDARSERVICE_DEBUG
#define LOG_DEBUG(msg) qDebug() << msg
#else
#define LOG_DEBUG(msg)
#endif

#define COPY_IF_NOT_EQUAL(dest, src, get, set) \
{ \
    if (dest->get != src->get) { \
        dest->set(src->get); \
    } \
}

template <typename T>
bool pointerDataEqual(const QVector<QSharedPointer<T> > &vectorA, const QVector<QSharedPointer<T> > &vectorB)
{
    if (vectorA.count() != vectorB.count()) {
        return false;
    }
    for (int i=0; i<vectorA.count(); i++) {
        if (vectorA[i].data() != vectorB[i].data()) {
            return false;
        }
    }
    return true;
}

void copyIncidenceProperties(KCalCore::Incidence::Ptr dest, const KCalCore::Incidence::Ptr &src)
{
    if (!dest || !src) {
        qWarning() << "Invalid parameters!";
        return;
    }
    if (dest->type() != src->type()) {
        qWarning() << "incidences do not have same type!";
        return;
    }

    KDateTime origCreated = dest->created();
    KDateTime origLastModified = dest->lastModified();

    // Copy recurrence information if required.
    if (*(dest->recurrence()) != *(src->recurrence())) {
        dest->recurrence()->clear();

        KCalCore::Recurrence *dr = dest->recurrence();
        KCalCore::Recurrence *sr = src->recurrence();

        // recurrence rules and dates
        KCalCore::RecurrenceRule::List srRRules = sr->rRules();
        for (QList<KCalCore::RecurrenceRule*>::const_iterator it = srRRules.constBegin(), end = srRRules.constEnd(); it != end; ++it) {
            KCalCore::RecurrenceRule *r = new KCalCore::RecurrenceRule(*(*it));
            dr->addRRule(r);
        }
        dr->setRDates(sr->rDates());
        dr->setRDateTimes(sr->rDateTimes());

        // exception rules and dates
        KCalCore::RecurrenceRule::List srExRules = sr->exRules();
        for (QList<KCalCore::RecurrenceRule*>::const_iterator it = srExRules.constBegin(), end = srExRules.constEnd(); it != end; ++it) {
            KCalCore::RecurrenceRule *r = new KCalCore::RecurrenceRule(*(*it));
            dr->addExRule(r);
        }
        dr->setExDates(sr->exDates());
        dr->setExDateTimes(sr->exDateTimes());
    }

    // copy the duration before the dtEnd as calling setDuration() changes the dtEnd
    COPY_IF_NOT_EQUAL(dest, src, duration(), setDuration);

    if (dest->type() == KCalCore::IncidenceBase::TypeEvent && src->type() == KCalCore::IncidenceBase::TypeEvent) {
        KCalCore::Event::Ptr destEvent = dest.staticCast<KCalCore::Event>();
        KCalCore::Event::Ptr srcEvent = src.staticCast<KCalCore::Event>();
        COPY_IF_NOT_EQUAL(destEvent, srcEvent, dtEnd(), setDtEnd);
        COPY_IF_NOT_EQUAL(destEvent, srcEvent, transparency(), setTransparency);
    }

    if (dest->type() == KCalCore::IncidenceBase::TypeTodo && src->type() == KCalCore::IncidenceBase::TypeTodo) {
        KCalCore::Todo::Ptr destTodo = dest.staticCast<KCalCore::Todo>();
        KCalCore::Todo::Ptr srcTodo = src.staticCast<KCalCore::Todo>();
        COPY_IF_NOT_EQUAL(destTodo, srcTodo, completed(), setCompleted);
        COPY_IF_NOT_EQUAL(destTodo, srcTodo, dtRecurrence(), setDtRecurrence);
        COPY_IF_NOT_EQUAL(destTodo, srcTodo, percentComplete(), setPercentComplete);
    }

    // dtStart and dtEnd changes allDay value, so must set those before copying allDay value
    COPY_IF_NOT_EQUAL(dest, src, dtStart(), setDtStart);
    COPY_IF_NOT_EQUAL(dest, src, allDay(), setAllDay);

    COPY_IF_NOT_EQUAL(dest, src, hasDuration(), setHasDuration);
    COPY_IF_NOT_EQUAL(dest, src, organizer(), setOrganizer);
    COPY_IF_NOT_EQUAL(dest, src, isReadOnly(), setReadOnly);

    if (!pointerDataEqual(src->attendees(), dest->attendees())) {
        dest->clearAttendees();
        Q_FOREACH (const KCalCore::Attendee::Ptr &attendee, src->attendees()) {
            dest->addAttendee(attendee);
        }
    }

    if (src->comments() != dest->comments()) {
        dest->clearComments();
        Q_FOREACH (const QString &comment, src->comments()) {
            dest->addComment(comment);
        }
    }
    if (src->contacts() != dest->contacts()) {
        dest->clearContacts();
        Q_FOREACH (const QString &contact, src->contacts()) {
            dest->addContact(contact);
        }
    }

    COPY_IF_NOT_EQUAL(dest, src, altDescription(), setAltDescription);
    COPY_IF_NOT_EQUAL(dest, src, categories(), setCategories);
    COPY_IF_NOT_EQUAL(dest, src, customStatus(), setCustomStatus);
    COPY_IF_NOT_EQUAL(dest, src, description(), setDescription);
    COPY_IF_NOT_EQUAL(dest, src, geoLatitude(), setGeoLatitude);
    COPY_IF_NOT_EQUAL(dest, src, geoLongitude(), setGeoLongitude);
    COPY_IF_NOT_EQUAL(dest, src, hasGeo(), setHasGeo);
    COPY_IF_NOT_EQUAL(dest, src, location(), setLocation);
    COPY_IF_NOT_EQUAL(dest, src, resources(), setResources);
    COPY_IF_NOT_EQUAL(dest, src, secrecy(), setSecrecy);
    COPY_IF_NOT_EQUAL(dest, src, status(), setStatus);
    COPY_IF_NOT_EQUAL(dest, src, summary(), setSummary);
    COPY_IF_NOT_EQUAL(dest, src, revision(), setRevision);

    if (!pointerDataEqual(src->alarms(), dest->alarms())) {
        dest->clearAlarms();
        Q_FOREACH (const KCalCore::Alarm::Ptr &alarm, src->alarms()) {
            dest->addAlarm(alarm);
        }
    }

    if (!pointerDataEqual(src->attachments(), dest->attachments())) {
        dest->clearAttachments();
        Q_FOREACH (const KCalCore::Attachment::Ptr &attachment, src->attachments()) {
            dest->addAttachment(attachment);
        }
    }

    // Don't change created and lastModified properties as that affects mkcal
    // calculations for when the incidence was added and modified in the db.
    if (origCreated != dest->created()) {
        dest->setCreated(origCreated);
    }
    if (origLastModified != dest->lastModified()) {
        dest->setLastModified(origLastModified);
    }
}

void prepareImportedIncidence(KCalCore::Incidence::Ptr incidence)
{
    if (incidence->type() != KCalCore::IncidenceBase::TypeEvent) {
        qWarning() << "unable to handle imported non-event incidence; skipping";
        return;
    }
    KCalCore::Event::Ptr event = incidence.staticCast<KCalCore::Event>();

    if (event->allDay()) {
        KDateTime dtStart = event->dtStart();
        KDateTime dtEnd = event->dtEnd();

        // calendar processing requires all-day events to have a dtEnd
        if (!dtEnd.isValid()) {
            LOG_DEBUG("Adding DTEND to" << incidence->uid() << "as" << dtStart.toString());
            event->setDtEnd(dtStart);
        }

        // setting dtStart/End changes the allDay value, so ensure it is still set to true
        event->setAllDay(true);
    }
}

bool updateIncidence(mKCal::ExtendedCalendar::Ptr calendar, mKCal::Notebook::Ptr notebook, KCalCore::Incidence::Ptr incidence, bool *criticalError)
{
    if (incidence.isNull()) {
        return false;
    }

    KCalCore::Incidence::Ptr storedIncidence;
    switch (incidence->type()) {
    case KCalCore::IncidenceBase::TypeEvent:
        storedIncidence = calendar->event(incidence->uid(), incidence->hasRecurrenceId() ? incidence->recurrenceId() : KDateTime());
        break;
    case KCalCore::IncidenceBase::TypeTodo:
        storedIncidence = calendar->todo(incidence->uid());
        break;
    case KCalCore::IncidenceBase::TypeJournal:
        storedIncidence = calendar->journal(incidence->uid());
        break;
    case KCalCore::IncidenceBase::TypeFreeBusy:
    case KCalCore::IncidenceBase::TypeUnknown:
        qWarning() << "Unsupported incidence type:" << incidence->type();
        return false;
    }
    if (storedIncidence) {
        if (incidence->status() == KCalCore::Incidence::StatusCanceled
                || incidence->customStatus().compare(QStringLiteral("CANCELLED"), Qt::CaseInsensitive) == 0) {
            LOG_DEBUG("Deleting cancelled event:" << storedIncidence->uid() << storedIncidence->recurrenceId().toString());
            if (!calendar->deleteIncidence(storedIncidence)) {
                qWarning() << "Error removing cancelled occurrence:" << storedIncidence->uid() << storedIncidence->recurrenceId().toString();
                return false;
            }
        } else {
            LOG_DEBUG("Updating existing event:" << storedIncidence->uid() << storedIncidence->recurrenceId().toString());
            storedIncidence->startUpdates();
            prepareImportedIncidence(incidence);
            copyIncidenceProperties(storedIncidence, incidence);

            // if this incidence is a recurring incidence, we should get all persistent occurrences
            // and add them back as EXDATEs.  This is because mkcal expects that dissociated
            // single instances will correspond to an EXDATE, but most sync servers do not (and
            // so will not include the RECURRENCE-ID values as EXDATEs of the parent).
            if (storedIncidence->recurs()) {
                KCalCore::Incidence::List instances = calendar->instances(incidence);
                Q_FOREACH (KCalCore::Incidence::Ptr instance, instances) {
                    if (instance->hasRecurrenceId()) {
                        storedIncidence->recurrence()->addExDateTime(instance->recurrenceId());
                    }
                }
            }
            storedIncidence->endUpdates();
        }
    } else {
        // the new incidence will be either a new persistent occurrence, or a new base-series (or new non-recurring).
        LOG_DEBUG("Have new incidence:" << incidence->uid() << incidence->recurrenceId().toString());
        KCalCore::Incidence::Ptr occurrence;
        if (incidence->hasRecurrenceId()) {
            // no dissociated occurrence exists already (ie, it's not an update), so create a new one.
            // need to detach, and then copy the properties into the detached occurrence.
            KCalCore::Incidence::Ptr recurringIncidence = calendar->event(incidence->uid(), KDateTime());
            if (recurringIncidence.isNull()) {
                qWarning() << "error: parent recurring incidence could not be retrieved:" << incidence->uid();
                return false;
            }
            occurrence = calendar->dissociateSingleOccurrence(recurringIncidence, incidence->recurrenceId(), incidence->recurrenceId().timeSpec());
            if (occurrence.isNull()) {
                qWarning() << "error: could not dissociate occurrence from recurring event:" << incidence->uid() << incidence->recurrenceId().toString();
                return false;
            }

            prepareImportedIncidence(incidence);
            copyIncidenceProperties(occurrence, incidence);
            if (!calendar->addEvent(occurrence.staticCast<KCalCore::Event>(), notebook->uid())) {
                qWarning() << "error: could not add dissociated occurrence to calendar";
                return false;
            }
            LOG_DEBUG("Added new occurrence incidence:" << occurrence->uid() << occurrence->recurrenceId().toString());
        } else {
            // just a new event without needing detach.
            prepareImportedIncidence(incidence);
            bool added = false;
            switch (incidence->type()) {
            case KCalCore::IncidenceBase::TypeEvent:
                added = calendar->addEvent(incidence.staticCast<KCalCore::Event>(), notebook->uid());
                break;
            case KCalCore::IncidenceBase::TypeTodo:
                added = calendar->addTodo(incidence.staticCast<KCalCore::Todo>(), notebook->uid());
                break;
            case KCalCore::IncidenceBase::TypeJournal:
                added = calendar->addJournal(incidence.staticCast<KCalCore::Journal>(), notebook->uid());
                break;
            case KCalCore::IncidenceBase::TypeFreeBusy:
            case KCalCore::IncidenceBase::TypeUnknown:
                qWarning() << "Unsupported incidence type:" << incidence->type();
                return false;
            }
            if (added) {
                LOG_DEBUG("Added new incidence:" << incidence->uid() << incidence->recurrenceId().toString());
            } else {
                qWarning() << "Unable to add incidence" << incidence->uid() << incidence->recurrenceId().toString() << "to notebook" << notebook->uid();
                *criticalError = true;
                return false;
            }
        }
    }
    return true;
}

bool importIcsData(const QString &icsData)
{
    bool destructiveImport = true;
    KCalCore::ICalFormat iCalFormat;
    KCalCore::MemoryCalendar::Ptr cal(new KCalCore::MemoryCalendar(KDateTime::UTC));
    if (!iCalFormat.fromString(cal, icsData)) {
        qWarning() << "unable to parse iCal data, trying as vCal";
        KCalCore::VCalFormat vCalFormat;
        if (!vCalFormat.fromString(cal, icsData)) {
            qWarning() << "unable to parse vCal data";
            return false;
        }
    }

    // Reorganize the list of imported incidences into lists of incidences segregated by UID.
    QHash<QString, KCalCore::Incidence::List> uidIncidences;
    KCalCore::Incidence::List importedIncidences = cal->incidences();
    Q_FOREACH (KCalCore::Incidence::Ptr imported, importedIncidences) {
        prepareImportedIncidence(imported);
        uidIncidences[imported->uid()] << imported;
    }

    // Now save the imported incidences into the calendar.
    // Note that the import may specify updates to existing events, so
    // we will need to compare the imported incidences with the
    // existing incidences, by UID.
    mKCal::ExtendedCalendar::Ptr calendar = mKCal::ExtendedCalendar::Ptr(new mKCal::ExtendedCalendar(KDateTime::Spec::UTC()));
    mKCal::ExtendedStorage::Ptr storage = mKCal::ExtendedCalendar::defaultStorage(calendar);
    storage->open();
    storage->load();
    mKCal::Notebook::Ptr notebook;
    mKCal::Notebook::List notebooks = storage->notebooks();
    Q_FOREACH (const mKCal::Notebook::Ptr nb, notebooks) {
        if (nb->isMaster() && !nb->isShared() && nb->pluginName().isEmpty()) {
            notebook = nb;
        }
    }
    if (!notebook) {
        qWarning() << "No default notebook exists";
        storage->close();
        return false;
    }
    KCalCore::Incidence::List notebookIncidences;
    storage->loadNotebookIncidences(notebook->uid());
    storage->allIncidences(&notebookIncidences, notebook->uid());

    if (destructiveImport) {
        // Any incidences which don't exist in the import list should be deleted.
        Q_FOREACH (KCalCore::Incidence::Ptr possiblyDoomed, notebookIncidences) {
            if (!uidIncidences.contains(possiblyDoomed->uid())) {
                // no incidence or series with this UID exists in the import list.
                LOG_DEBUG("Removing rolled-back incidence:" << possiblyDoomed->uid() << possiblyDoomed->recurrenceId().toString());
                if (!calendar->deleteIncidence(possiblyDoomed)) {
                    qWarning() << "Error removing rolled-back incidence:" << possiblyDoomed->uid() << possiblyDoomed->recurrenceId().toString();
                    storage->close();
                    return false;
                }
            } // no need to remove rolled-back persistent occurrences here; we do that later.
        }
    }

    Q_FOREACH (const QString &uid, uidIncidences.keys()) {
        // deal with every incidence or series from the import list.
        KCalCore::Incidence::List incidences(uidIncidences[uid]);
        // find the recurring incidence (parent) in the import list, and save it.
        // alternatively, it may be a non-recurring base incidence.
        bool criticalError = false;
        int parentIndex = -1;
        for (int i = 0; i < incidences.size(); ++i) {
            if (!incidences[i]->hasRecurrenceId()) {
                parentIndex = i;
                break;
            }
        }

        if (parentIndex == -1) {
            LOG_DEBUG("No parent or base incidence in incidence list, performing direct updates to persistent occurrences");
            for (int i = 0; i < incidences.size(); ++i) {
                KCalCore::Incidence::Ptr importInstance = incidences[i];
                updateIncidence(calendar, notebook, importInstance, &criticalError);
                if (criticalError) {
                    qWarning() << "Error saving updated persistent occurrence:" << importInstance->uid() << importInstance->recurrenceId().toString();
                    storage->close();
                    return false;
                }
            }
        } else {
            // if there was a parent / base incidence, then we need to compare local/import lists.
            // load the local (persistent) occurrences of the series.  Later we will update or remove them as required.
            KCalCore::Incidence::Ptr localBaseIncidence = calendar->incidence(uid, KDateTime());
            KCalCore::Incidence::List localInstances;
            if (!localBaseIncidence.isNull() && localBaseIncidence->recurs()) {
                localInstances = calendar->instances(localBaseIncidence);
            }

            // first save the added/updated base incidence
            LOG_DEBUG("Saving the added/updated base incidence before saving persistent exceptions:" << incidences[parentIndex]->uid());
            KCalCore::Incidence::Ptr updatedBaseIncidence = incidences[parentIndex];
            updateIncidence(calendar, notebook, updatedBaseIncidence, &criticalError); // update the base incidence first.
            if (criticalError) {
                qWarning() << "Error saving base incidence:" << updatedBaseIncidence->uid();
                storage->close();
                return false;
            }

            // update persistent exceptions which are in the import list.
            QList<KDateTime> importRecurrenceIds;
            for (int i = 0; i < incidences.size(); ++i) {
                if (i == parentIndex) {
                    continue; // already handled this one.
                }

                LOG_DEBUG("Now saving a persistent exception:" << incidences[i]->recurrenceId().toString());
                KCalCore::Incidence::Ptr importInstance = incidences[i];
                importRecurrenceIds.append(importInstance->recurrenceId());
                updateIncidence(calendar, notebook, importInstance, &criticalError);
                if (criticalError) {
                    qWarning() << "Error saving updated persistent occurrence:" << importInstance->uid() << importInstance->recurrenceId().toString();
                    storage->close();
                    return false;
                }
            }

            if (destructiveImport) {
                // remove persistent exceptions which are not in the import list.
                for (int i = 0; i < localInstances.size(); ++i) {
                    KCalCore::Incidence::Ptr localInstance = localInstances[i];
                    if (!importRecurrenceIds.contains(localInstance->recurrenceId())) {
                        LOG_DEBUG("Removing rolled-back persistent occurrence:" << localInstance->uid() << localInstance->recurrenceId().toString());
                        if (!calendar->deleteIncidence(localInstance)) {
                            qWarning() << "Error removing rolled-back persistent occurrence:" << localInstance->uid() << localInstance->recurrenceId().toString();
                            storage->close();
                            return false;
                        }
                    }
                }
            }
        }
    }

    storage->save();
    storage->close();
    return true;
}

void CalendarWriteChrc::WriteValue(QByteArray value, QVariantMap)
{
    QString icsData = value;
    // TODO: Accumulate ics data over several messages, until the file is complete

    importIcsData(icsData);
}

CalendarService::CalendarService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, CALENDAR_UUID, parent)
{
    CalendarWriteChrc *writeChrc = new CalendarWriteChrc(bus, 0, this);
    addCharacteristic(writeChrc);
}
