#pragma once

/**
 * IComponent — logos-app UI plugin interface.
 *
 * Canonical source: logos-co/logos-app (app/interfaces/IComponent.h)
 * IID: "com.logos.component.IComponent"
 *
 * logos-app loads UI plugins via QPluginLoader from the plugins directory:
 *   Non-portable: ~/.local/share/LogosAppNix/plugins/<name>/<name>.so
 *   Portable:     ~/.local/share/LogosApp/plugins/<name>/<name>.so
 *
 * logos-app calls:
 *   1. createWidget(logosAPI) — create and return the embedded widget
 *   2. destroyWidget(widget)  — cleanup before unload
 *
 * logosAPI provides:
 *   - getClient(moduleName)   → LogosAPIClient* (remote method calls)
 *   - getProvider()           → LogosAPIProvider* (expose methods to others)
 *   - getTokenManager()       → TokenManager* (auth tokens)
 *
 * @see https://github.com/logos-co/logos-app/blob/master/app/interfaces/IComponent.h
 * @see https://github.com/logos-co/logos-cpp-sdk
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
