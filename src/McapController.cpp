#include "McapController.h"
#include "ProtoDecoder.h"
#include "RosDecoder.h"

#include <QUrl>
#include <QDebug>
#include <unordered_map>
#include <unordered_set>

#include "mcap/reader.hpp"

static const int kMaxFrames = 500;

static bool isImageSchema(const std::string& name) {
    static const std::unordered_set<std::string> kImageSchemas = {
        "foxglove.RawImage",
        "foxglove.CompressedImage",
        "sensor_msgs/Image",
        "sensor_msgs/CompressedImage",
    };
    return kImageSchemas.count(name) > 0;
}

McapController::McapController(QObject* parent) : QObject(parent) {}

void McapController::openFile(const QString& urlOrPath) {
    QUrl url(urlOrPath);
    filePath_ = url.isLocalFile() ? url.toLocalFile() : urlOrPath;

    topics_.clear();
    frames_.clear();
    selectedTopic_.clear();
    schemaName_.clear();
    currentFrame_ = 0;
    frameSource_.clear();

    emit topicsChanged();
    emit frameCountChanged();
    emit selectedTopicChanged();
    emit frameSourceChanged();

    setStatus("Opening " + filePath_ + " ...");

    mcap::McapReader reader;
    const auto openStatus = reader.open(filePath_.toStdString());
    if (!openStatus.ok()) {
        setStatus("Error: " + QString::fromStdString(openStatus.message));
        return;
    }

    const auto summaryStatus = reader.readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);
    if (!summaryStatus.ok()) {
        qWarning() << "readSummary warning:" << QString::fromStdString(summaryStatus.message);
    }

    // schema id → name
    std::unordered_map<uint16_t, std::string> schemaNames;
    for (const auto& [id, schema] : reader.schemas()) {
        schemaNames[id] = schema->name;
    }

    QStringList imgTopics;
    for (const auto& [id, channel] : reader.channels()) {
        auto it = schemaNames.find(channel->schemaId);
        if (it != schemaNames.end() && isImageSchema(it->second)) {
            imgTopics << QString::fromStdString(channel->topic);
        }
    }
    reader.close();

    imgTopics.sort();
    topics_ = imgTopics;
    emit topicsChanged();

    if (topics_.isEmpty()) {
        setStatus("No image topics found in file.");
        return;
    }

    setStatus(QString("%1 image topic(s) found.").arg(topics_.size()));
    setSelectedTopic(topics_.first());
}

void McapController::setSelectedTopic(const QString& topic) {
    if (selectedTopic_ == topic && !frames_.empty()) return;
    selectedTopic_ = topic;
    currentFrame_ = 0;
    frames_.clear();
    schemaName_.clear();
    emit selectedTopicChanged();
    emit frameCountChanged();

    if (!topic.isEmpty())
        loadTopic();
}

void McapController::loadTopic() {
    loading_ = true;
    emit loadingChanged();
    setStatus("Loading topic: " + selectedTopic_);

    mcap::McapReader reader;
    const auto openStatus = reader.open(filePath_.toStdString());
    if (!openStatus.ok()) {
        setStatus("Error: " + QString::fromStdString(openStatus.message));
        loading_ = false;
        emit loadingChanged();
        return;
    }
    (void)reader.readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);

    // Find schema name for selected topic
    std::unordered_map<uint16_t, std::string> schemaNames;
    for (const auto& [id, schema] : reader.schemas()) {
        schemaNames[id] = schema->name;
    }
    for (const auto& [id, channel] : reader.channels()) {
        if (channel->topic == selectedTopic_.toStdString()) {
            auto it = schemaNames.find(channel->schemaId);
            if (it != schemaNames.end()) schemaName_ = it->second;
        }
    }

    frames_.clear();
    const std::string targetTopic = selectedTopic_.toStdString();

    auto messages = reader.readMessages();
    for (auto it = messages.begin(); it != messages.end(); ++it) {
        const mcap::MessageView& view = *it;
        if (view.channel->topic != targetTopic) continue;

        FrameData fd;
        fd.logTime = view.message.logTime;
        const auto* dataPtr = reinterpret_cast<const uint8_t*>(view.message.data);
        fd.msgData.assign(dataPtr, dataPtr + view.message.dataSize);
        frames_.push_back(std::move(fd));

        if ((int)frames_.size() >= kMaxFrames) {
            setStatus(QString("Loaded first %1 frames (capped). Schema: %2")
                      .arg(kMaxFrames)
                      .arg(QString::fromStdString(schemaName_)));
            break;
        }
    }
    reader.close();

    loading_ = false;
    emit loadingChanged();
    emit frameCountChanged();

    if (frames_.empty()) {
        setStatus("No messages found for topic: " + selectedTopic_);
        return;
    }

    if (statusText_.startsWith("Loaded first")) {
        /* already set above */
    } else {
        setStatus(QString("%1 frames, schema: %2")
                  .arg(frames_.size())
                  .arg(QString::fromStdString(schemaName_)));
    }

    currentFrame_ = 0;
    emit currentFrameChanged();
    ++frameSerial_;
    frameSource_ = QString("image://mcap/%1").arg(frameSerial_ * kMaxFrames + currentFrame_);
    emit frameSourceChanged();
}

void McapController::setCurrentFrame(int frame) {
    if (frames_.empty()) return;
    frame = qBound(0, frame, (int)frames_.size() - 1);
    if (frame == currentFrame_ && !frameSource_.isEmpty()) return;
    currentFrame_ = frame;
    emit currentFrameChanged();
    ++frameSerial_;
    frameSource_ = QString("image://mcap/%1").arg(frameSerial_ * kMaxFrames + currentFrame_);
    emit frameSourceChanged();
}

QImage McapController::getFrame(int index) {
    if (index < 0 || index >= (int)frames_.size()) return QImage();
    return decodeFrame(frames_[index]);
}

QImage McapController::decodeFrame(const FrameData& fd) const {
    const uint8_t* buf = fd.msgData.data();
    size_t         len = fd.msgData.size();

    if (schemaName_ == "foxglove.RawImage") {
        proto::RawImage ri;
        if (proto::parseRawImage(buf, len, ri))
            return rawToQImage(ri.width, ri.height, ri.encoding, ri.data, ri.dataLen);
    } else if (schemaName_ == "foxglove.CompressedImage") {
        proto::CompressedImage ci;
        if (proto::parseCompressedImage(buf, len, ci)) {
            QImage img;
            img.loadFromData(ci.data, (int)ci.dataLen);
            return img;
        }
    } else if (schemaName_ == "sensor_msgs/Image") {
        ros1::Image ri;
        if (ros1::parseImage(buf, len, ri))
            return rawToQImage(ri.width, ri.height, ri.encoding, ri.data, ri.dataLen);
    } else if (schemaName_ == "sensor_msgs/CompressedImage") {
        ros1::CompressedImage ci;
        if (ros1::parseCompressedImage(buf, len, ci)) {
            QImage img;
            img.loadFromData(ci.data, (int)ci.dataLen);
            return img;
        }
    }

    qWarning() << "Failed to decode frame, schema:" << QString::fromStdString(schemaName_);
    return QImage();
}

QImage McapController::rawToQImage(uint32_t w, uint32_t h, const std::string& enc,
                                    const uint8_t* data, size_t len) const {
    if (!data || w == 0 || h == 0) return QImage();

    if (enc == "rgb8" || enc == "RGB8") {
        if (len < (size_t)w * h * 3) return QImage();
        return QImage(data, w, h, w * 3, QImage::Format_RGB888).copy();
    }
    if (enc == "bgr8" || enc == "BGR8") {
        if (len < (size_t)w * h * 3) return QImage();
        QImage img(w, h, QImage::Format_RGB888);
        for (uint32_t y = 0; y < h; ++y) {
            const uint8_t* src = data + y * w * 3;
            uint8_t*       dst = img.scanLine(y);
            for (uint32_t x = 0; x < w; ++x) {
                dst[x*3+0] = src[x*3+2];
                dst[x*3+1] = src[x*3+1];
                dst[x*3+2] = src[x*3+0];
            }
        }
        return img;
    }
    if (enc == "mono8" || enc == "8UC1") {
        if (len < (size_t)w * h) return QImage();
        return QImage(data, w, h, w, QImage::Format_Grayscale8).copy();
    }
    if (enc == "16UC1") {
        // Normalize 16-bit to 8-bit grayscale
        if (len < (size_t)w * h * 2) return QImage();
        QImage img(w, h, QImage::Format_Grayscale8);
        const uint16_t* src16 = reinterpret_cast<const uint16_t*>(data);
        uint16_t vmin = 65535, vmax = 0;
        for (size_t i = 0; i < (size_t)w * h; ++i) {
            vmin = std::min(vmin, src16[i]);
            vmax = std::max(vmax, src16[i]);
        }
        float scale = (vmax > vmin) ? 255.f / (vmax - vmin) : 0.f;
        for (uint32_t y = 0; y < h; ++y) {
            uint8_t* dst = img.scanLine(y);
            for (uint32_t x = 0; x < w; ++x)
                dst[x] = (uint8_t)((src16[y * w + x] - vmin) * scale);
        }
        return img;
    }
    if (enc == "rgba8" || enc == "RGBA8") {
        if (len < (size_t)w * h * 4) return QImage();
        return QImage(data, w, h, w * 4, QImage::Format_RGBA8888).copy();
    }
    if (enc == "bgra8" || enc == "BGRA8") {
        if (len < (size_t)w * h * 4) return QImage();
        QImage img(w, h, QImage::Format_RGBA8888);
        for (uint32_t y = 0; y < h; ++y) {
            const uint8_t* src = data + y * w * 4;
            uint8_t*       dst = img.scanLine(y);
            for (uint32_t x = 0; x < w; ++x) {
                dst[x*4+0] = src[x*4+2];
                dst[x*4+1] = src[x*4+1];
                dst[x*4+2] = src[x*4+0];
                dst[x*4+3] = src[x*4+3];
            }
        }
        return img;
    }

    qWarning() << "Unsupported encoding:" << QString::fromStdString(enc);
    return QImage();
}

void McapController::setStatus(const QString& s) {
    statusText_ = s;
    emit statusTextChanged();
}
