#include <QmlNet/qml/QCoreApplication.h>
#include <QmlNet/qml/NetVariantList.h>
#include <QGuiApplication>
#include <QApplication>

GuiThreadContextTriggerCallback::GuiThreadContextTriggerCallback() :
    callback(nullptr) {
}

void GuiThreadContextTriggerCallback::trigger() {
    if (callback) {
       callback();
    }
}

extern "C" {

Q_DECL_EXPORT QGuiApplicationContainer* qapp_fromExisting(QCoreApplication* rawPointer)
{
    QGuiApplicationContainer* result = new QGuiApplicationContainer();
    result->ownsApp = false;
    result->app = rawPointer;

    result->callback = QSharedPointer<GuiThreadContextTriggerCallback>(new GuiThreadContextTriggerCallback());

    return result;
}

Q_DECL_EXPORT QGuiApplicationContainer* qapp_create(NetVariantListContainer* argsContainer, int flags, int type)
{
    QGuiApplicationContainer* result = new QGuiApplicationContainer();

    // Build our args
    if(argsContainer != nullptr) {
        QSharedPointer<NetVariantList> args = argsContainer->list;
        for(int x = 0; x < args->count(); x++) {
            QByteArray arg = args->get(x)->getString().toLatin1();
            result->args.append(arg);
            char* cstr = new char [size_t(arg.size())+1];
            memcpy(cstr, arg.data(), size_t(arg.size())+1);
            result->argsPointer.push_back(cstr);
        }
        result->argCount = result->args.size();
    } else {
        result->argCount = 0;
    }

    result->ownsApp = true;

    switch(type)
    {
    case 0:
        result->app = new QCoreApplication(result->argCount, &result->argsPointer[0], flags);
        break;
    case 1:
        result->app = new QGuiApplication(result->argCount, &result->argsPointer[0], flags);
        break;
    case 2:
        result->app = new QApplication(result->argCount, &result->argsPointer[0], flags);
        break;
    default:
        qCritical("invalid app type %d", type);
        delete result;
        return nullptr;
    }

    result->callback = QSharedPointer<GuiThreadContextTriggerCallback>(new GuiThreadContextTriggerCallback());

    return result;
}

Q_DECL_EXPORT void qapp_destroy(QGuiApplicationContainer* container)
{
    for (auto i : container->argsPointer) {
        delete[] i;
    }
    container->callback.clear();
    if(container->ownsApp) {
        delete container->app;
    }
    delete container;
}

Q_DECL_EXPORT int qapp_getType(QGuiApplicationContainer* container, QCoreApplication* rawPointer)
{
    if (!container && !rawPointer) {
        qCritical("invalid container and/or rawPointer");
        return -1;
    }
    if (container && rawPointer) {
        qCritical("invalid container and/or rawPointer");
        return -1;
    }
    if(!rawPointer && container) {
        rawPointer = container->app;
    }
    if (qobject_cast<QApplication*>(rawPointer) != nullptr){
        return 2;
    }
    if (qobject_cast<QGuiApplication*>(rawPointer) != nullptr){
        return 1;
    }
    // QCoreApplication
    return 0;
}

Q_DECL_EXPORT int qapp_exec()
{
    return QGuiApplication::exec();
}

Q_DECL_EXPORT void qapp_addTriggerCallback(QGuiApplicationContainer* container, guiThreadTriggerCb callback)
{
    container->callback->callback = callback;
}

Q_DECL_EXPORT void qapp_requestTrigger(QGuiApplicationContainer* container)
{
    QMetaObject::invokeMethod(container->callback.data(), "trigger", Qt::QueuedConnection);
}

Q_DECL_EXPORT void qapp_exit(int returnCode)
{
    QGuiApplication::exit(returnCode);
}

Q_DECL_EXPORT QCoreApplication* qapp_internalPointer(QGuiApplicationContainer* container)
{
    return container->app;
}

}