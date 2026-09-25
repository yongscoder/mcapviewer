#include "McapImageProvider.h"
#include "McapController.h"

McapImageProvider::McapImageProvider(McapController* controller)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , controller_(controller)
{}

QImage McapImageProvider::requestImage(const QString& id, QSize* size, const QSize& /*requestedSize*/) {
    // id is a unique serial encoded as: serial * kMaxFrames + frameIndex
    // We recover frameIndex from controller.currentFrame() since id is only for cache-busting.
    bool ok = false;
    id.toLongLong(&ok);  // just validate it's numeric
    (void)ok;

    QImage img = controller_->getFrame(controller_->currentFrame());
    if (size) *size = img.size();
    return img;
}
