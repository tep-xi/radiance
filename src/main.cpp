#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QThread>
#include "EffectNode.h"
#include "GraphicalDisplay.h"
#include "ImageNode.h"
#include "Lux.h"
#include "Midi.h"
#include "Model.h"
#include "NodeRegistry.h"
#include "Output.h"
#include "QQuickVideoNodeRender.h"
#include "VideoNode.h"
#include "View.h"
#include "main.h"

QSharedPointer<RenderContext> renderContext;
QSharedPointer<OpenGLWorkerContext> openGLWorkerContext;
QSharedPointer<QSettings> settings;
QSharedPointer<QSettings> outputSettings;
QSharedPointer<UISettings> uiSettings;
QSharedPointer<Audio> audio;
QSharedPointer<OutputManager> outputManager;
QSharedPointer<NodeRegistry> nodeRegistry;
QSharedPointer<Timebase> timebase;

QObject *uiSettingsProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(uiSettings.data(), QQmlEngine::CppOwnership);
    return uiSettings.data();
}

QObject *audioProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(audio.data(), QQmlEngine::CppOwnership);
    return audio.data();
}

QObject *outputManagerProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(outputManager.data(), QQmlEngine::CppOwnership);
    return outputManager.data();
}

QObject *nodeRegistryProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(nodeRegistry.data(), QQmlEngine::CppOwnership);
    return nodeRegistry.data();
}

QObject *renderContextProvider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    QQmlEngine::setObjectOwnership(renderContext.data(), QQmlEngine::CppOwnership);
    return renderContext.data();
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);

    QThread::currentThread()->setObjectName("mainThread");

    openGLWorkerContext = QSharedPointer<OpenGLWorkerContext>(new OpenGLWorkerContext());
    openGLWorkerContext->setObjectName("openGLWorkerContext");
    openGLWorkerContext->thread()->start();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, openGLWorkerContext->thread(), &QThread::quit);

    settings = QSharedPointer<QSettings>(new QSettings());
    outputSettings = QSharedPointer<QSettings>(new QSettings(QSettings::IniFormat, QSettings::UserScope, "Radiance", "Radiance Output"));
    uiSettings = QSharedPointer<UISettings>(new UISettings());
    timebase = QSharedPointer<Timebase>(new Timebase());
    audio = QSharedPointer<Audio>(new Audio());
    outputManager = QSharedPointer<OutputManager>(new OutputManager(outputSettings.data()));
    nodeRegistry = QSharedPointer<NodeRegistry>(new NodeRegistry());
    renderContext = QSharedPointer<RenderContext>(new RenderContext());

    qmlRegisterUncreatableType<VideoNode>("radiance", 1, 0, "VideoNode", "VideoNode is abstract and cannot be instantiated");
    qmlRegisterType<Model>("radiance", 1, 0, "Model");
    qmlRegisterType<EffectNode>("radiance", 1, 0, "EffectNode");
    qmlRegisterType<ImageNode>("radiance", 1, 0, "ImageNode");
    qmlRegisterType<View>("radiance", 1, 0, "View");

    qmlRegisterType<QQuickVideoNodeRender>("radiance", 1, 0, "VideoNodeRender");
    qmlRegisterType<MidiDevice>("radiance", 1, 0, "MidiDevice");
    qmlRegisterType<GraphicalDisplay>("radiance", 1, 0, "GraphicalDisplay");

    qmlRegisterSingletonType<UISettings>("radiance", 1, 0, "UISettings", uiSettingsProvider);
    qmlRegisterSingletonType<Audio>("radiance", 1, 0, "Audio", audioProvider);
    qmlRegisterSingletonType<NodeRegistry>("radiance", 1, 0, "NodeRegistry", nodeRegistryProvider);
    qmlRegisterSingletonType<RenderContext>("radiance", 1, 0, "RenderContext", renderContextProvider);

    qmlRegisterType<LuxBus>("radiance", 1, 0, "LuxBus");
    qmlRegisterType<LuxDevice>("radiance", 1, 0, "LuxDevice");
    qmlRegisterSingletonType<OutputManager>("radiance", 1, 0, "OutputManager", outputManagerProvider);

    QQmlApplicationEngine engine(QUrl("../resources/qml/application.qml"));
    if(engine.rootObjects().isEmpty()) {
        qFatal("Failed to load main QML application");
        return 1;
    }

    return app.exec();
}
