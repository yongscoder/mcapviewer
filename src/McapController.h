#pragma once
#include <QObject>
#include <QImage>
#include <QString>
#include <QStringList>
#include <vector>
#include <cstdint>

struct FrameData {
    uint64_t logTime;
    std::vector<uint8_t> msgData;
};

class McapController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList topics      READ topics         NOTIFY topicsChanged)
    Q_PROPERTY(int         frameCount  READ frameCount     NOTIFY frameCountChanged)
    Q_PROPERTY(int         currentFrame READ currentFrame  WRITE setCurrentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(QString     selectedTopic READ selectedTopic WRITE setSelectedTopic NOTIFY selectedTopicChanged)
    Q_PROPERTY(QString     frameSource READ frameSource    NOTIFY frameSourceChanged)
    Q_PROPERTY(bool        loading     READ loading        NOTIFY loadingChanged)
    Q_PROPERTY(QString     statusText  READ statusText     NOTIFY statusTextChanged)

public:
    explicit McapController(QObject* parent = nullptr);

    Q_INVOKABLE void openFile(const QString& urlOrPath);

    QStringList topics()       const { return topics_; }
    int         frameCount()   const { return (int)frames_.size(); }
    int         currentFrame() const { return currentFrame_; }
    QString     selectedTopic() const { return selectedTopic_; }
    QString     frameSource()  const { return frameSource_; }
    bool        loading()      const { return loading_; }
    QString     statusText()   const { return statusText_; }

    void setCurrentFrame(int frame);
    void setSelectedTopic(const QString& topic);

    // Called by McapImageProvider
    QImage getFrame(int index);

signals:
    void topicsChanged();
    void frameCountChanged();
    void currentFrameChanged();
    void selectedTopicChanged();
    void frameSourceChanged();
    void loadingChanged();
    void statusTextChanged();

private:
    void loadTopic();
    QImage decodeFrame(const FrameData& fd) const;
    QImage rawToQImage(uint32_t w, uint32_t h, const std::string& enc,
                       const uint8_t* data, size_t len) const;
    void setStatus(const QString& s);

    QString     filePath_;
    QStringList topics_;
    QString     selectedTopic_;
    int         currentFrame_ = 0;
    QString     frameSource_;
    bool        loading_ = false;
    QString     statusText_;

    std::vector<FrameData> frames_;
    std::string schemaName_;
    int         frameSerial_ = 0; // makes each source URL unique
};
