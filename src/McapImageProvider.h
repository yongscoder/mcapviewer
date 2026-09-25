#pragma once
#include <QQuickImageProvider>

class McapController;

class McapImageProvider : public QQuickImageProvider {
public:
    explicit McapImageProvider(McapController* controller);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    McapController* controller_;
};
