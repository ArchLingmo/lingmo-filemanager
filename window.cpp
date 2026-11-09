/*
 * Copyright (C) 2021 LingmoOS Team.
 *
 * Author:     Reion Wong <reion@lingmoos.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "window.h"
#include <QEvent>
#include <QDebug>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QCursor>
#include <QScreen>
#include <QTimer>
#include <QPixmapCache>

Window::Window(QObject *parent)
    : QQmlApplicationEngine(parent)
{
}

void Window::load(const QUrl &url)
{
    QQmlApplicationEngine::load(url);

    if (rootObjects().isEmpty()) {
        deleteLater();
        return;
    }

    QQuickWindow *w = qobject_cast<QQuickWindow *>(rootObjects().first());

    if (!w)
        return;
    w->installEventFilter(this);

    // A dialog (Empty Trash, Delete, …) opens over the file manager window it came from: with
    // no parent it opened at 0,0 of whatever screen, often another monitor or behind the window
    if (w->flags().testFlag(Qt::Dialog)) {
        QWindow *parent = QGuiApplication::focusWindow();
        if (!parent || parent == w) {
            for (QWindow *top : QGuiApplication::topLevelWindows()) {
                if (top != w && top->isVisible() && top->type() == Qt::Window && top->screen() == QGuiApplication::screenAt(QCursor::pos())) {
                    parent = top;
                    break;
                }
            }
        }
        QRect area;
        if (parent && parent != w) {
            area = parent->geometry();
        } else if (QScreen *screen = QGuiApplication::screenAt(QCursor::pos())) {
            area = screen->availableGeometry();
        }
        if (parent && parent != w)
            w->setTransientParent(parent);
        // the QML shows the window while loading and the frameless LingmoUI window settles its
        // geometry right after: place it once that is done
        QTimer::singleShot(0, w, [w, area] {
            if (area.isValid())
                w->setPosition(area.center() - QPoint(w->width() / 2, w->height() / 2));
            w->requestActivate();
        });
    }
}

bool Window::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::Close) {
        QPixmapCache::clear();
        clearComponentCache();
        deleteLater();
        e->accept();
    }

    return QObject::eventFilter(obj, e);
}
