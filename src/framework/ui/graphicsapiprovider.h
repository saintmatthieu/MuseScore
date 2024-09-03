/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <functional>
#include <QString>
#include <QTimer>
#include <QQuickPaintedItem>

#include "global/types/version.h"

namespace muse::ui {
class GraphicsProblemsDetectorLogDest;
class GraphicsTestObject;
class GraphicsApiProvider
{
public:

    GraphicsApiProvider(const Version& appVersion);
    ~GraphicsApiProvider();

    //! NOTE Same as QSGRendererInterface::Api
    enum Api {
        Default,
        Software,
        OpenVG,
        OpenGL,
        Direct3D11,
        Vulkan,
        Metal,
        Null
    };

    enum class Status {
        NeedCheck = 1, // switched to this api
        Checking,      // try checking
        Checked        // all good
    };

    static void setGraphicsApi(Api api);
    static Api graphicsApi();
    static QString graphicsApiName();

    static Api apiByName(const QString& name);
    static QString apiName(Api api);

    Api requiredGraphicsApi();
    void setGraphicsApiStatus(Api api, Status status);

    Api nextGraphicsApi(Api current) const;
    Api switchToNextGraphicsApi(Api current);

    using OnResult = std::function<void (bool res)>;
    void listen(const OnResult& f);
    void destroy();

    static GraphicsTestObject* graphicsTestObject;

private:

    QString dataFilePath() const;

    struct Data {
        QString appVersion;
        QString apiName;
        Status status = Status::NeedCheck;

        bool isValid() const { return !apiName.isEmpty(); }
    };

    Data readData() const;
    void writeData(const Data& d);

    Version m_appVersion;
    GraphicsProblemsDetectorLogDest* m_logDest = nullptr;
    OnResult m_onResult;
    QTimer m_timer;
};

class GraphicsTestObject : public QQuickPaintedItem
{
public:
    GraphicsTestObject();
    ~GraphicsTestObject();

    bool painted = false;

    void paint(QPainter*) override;
};
}
