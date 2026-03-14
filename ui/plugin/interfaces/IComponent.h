#pragma once

/**
 * IComponent — logos-app UI plugin interface.
 *
 * Sourced from jimmy-claw/scala (interfaces/IComponent.h).
 * IID: "com.logos.component.IComponent"
 *
 * A plugin that implements this interface can be discovered and loaded by
 * logos-app from the plugins directory:
 *   ~/.local/share/Logos/LogosAppNix/plugins/<name>/lib<name>.so
 *
 * logos-app calls:
 *   1. createWidget(logosAPI) — create and return the embedded widget
 *   2. destroyWidget(widget)  — cleanup before unload
 *
 * @see https://github.com/jimmy-claw/scala/blob/main/interfaces/IComponent.h
 */

#include <QObject>
#include <QWidget>
#include <QtPlugin>

class LogosAPI;

class IComponent {
public:
    virtual ~IComponent() = default;
    virtual QWidget* createWidget(LogosAPI* logosAPI = nullptr) = 0;
    virtual void destroyWidget(QWidget* widget) = 0;
};

#define IComponent_iid "com.logos.component.IComponent"
Q_DECLARE_INTERFACE(IComponent, IComponent_iid)
