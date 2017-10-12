#ifndef FFMPEG_H
#define FFMPEG_H

#include <QObject>

#include <QProcess>

#include "ffmpegcodec.h"

class FFmpeg : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief FFmpeg Constructs the FFmpeg manager
     * @param path The path to the FFmpeg binary executable
     * @param parent The parent QObject
     */
    explicit FFmpeg(QString path, QObject *parent = nullptr);
    /**
     * @brief getEncoders Gets the list of encoders supported the current version of FFmpeg
     * @return The codec list
     */
    QList<FFmpegCodec> getEncoders();
    /**
     * @brief getVideoEncoders Gets the list of video encoders supported by the current version of FFmpeg
     * @return The video codec list
     */
    QList<FFmpegCodec> getVideoEncoders();
    /**
     * @brief getVideoEncoders Gets the list of audio encoders supported by the current version of FFmpeg
     * @return The audio codec list
     */
    QList<FFmpegCodec> getAudioEncoders();

signals:

public slots:

private:
    /**
     * @brief ffmpeg The process used to handle the binary
     */
    QProcess *ffmpeg;
    /**
     * @brief videoEncoders The list of the encoders supported by the current version of FFmpeg
     */
    QList<FFmpegCodec> videoEncoders;
    /**
     * @brief audioEncoders The list of the encoders supported by the current version of FFmpeg
     */
    QList<FFmpegCodec> audioEncoders;
    /**
     * @brief ffmpeg_gotCodecs Parses the codec list
     * @param output The output of the FFmpeg process with the codecs list
     */
    void gotCodecs(QString output);
};

#endif // FFMPEG_H