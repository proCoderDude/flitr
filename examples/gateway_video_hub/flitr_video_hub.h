/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2016 CSIR
 *
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef FLITR_VIDEO_HUB_H
#define FLITR_VIDEO_HUB_H 1

#include <flitr/image_format.h>
#include <flitr/image_producer.h>

#include <memory>
#include <map>
#include <string>

namespace flitr
{
    //!Structure to hold the image format of a video hub image.
    struct VideoHubImageFormat
    {
        int32_t _width;//!< Width of the image in pixels.
        int32_t _height;//!< Height of the image in pixels.
        ImageFormat::PixelFormat _pixelFormat;
        
        VideoHubImageFormat() :
        _width(-1),
        _height(-1),
        _pixelFormat(ImageFormat::FLITR_PIX_FMT_ANY)
        {}
    };
    
    
    //!Wrapper class around FLITr functionality.
    class VideoHub
    {
    public:
        VideoHub();
        ~VideoHub();
        
        bool init();
        
        
        //=== Producers
        /** Create and image producer to play back a video file.
         \param name The name of the new processor/producer.
         \param fileName The name of the producer to attach to.
         */
        bool createVideoFileProducer(const std::string &name, const std::string &filename);

        /** Create and image producer to play an RTSP stream.
         \param name The name of the new processor/producer.
         \param fileName The name of the producer to attach to.
         */
        bool createRTSPProducer(std::string &name, const std::string &url);
        
        /** Create and image producer to open and get images from a v4l device.
         \param name The name of the new processor/producer.
         \param fileName The name of the producer to attach to.
         */
        bool createV4LProducer(std::string &name, const std::string &device);
        
        
        //=== Image processors
        /** Create and image processor that stabilises the image stream.
         \param name The name of the new processor/producer.
         \param producerName The name of the producer to attach to.
         */
        bool createImageStabProcess(const std::string &name, const std::string &producerName);
        
        /** Create and image processor that detects motion in the image stream.
         \param name The name of the new processor/producer.
         \param producerName The name of the producer to attach to.
         \oaram showOverlays If true the areas of motion are indicated by a bounding box.
         \oaram produceOnlyMotionImages If true then images are only produced when there is motion.
         */
        bool createMotionDetectProcess(const std::string &name, const std::string &producerName,
                                       const bool showOverlays, const bool produceOnlyMotionImages);
        
        
        //=== Consumers
        /** Create and image consumer that writes the image stream to a video file.
         \param name The name of the new consumer.
         \param producerName The name of the producer to attach to.
         \param fileName The name of the video file.
         \param frameRate The expected video frame rate used for later playback.
         */
        bool createVideoFileConsumer(const std::string &name, const std::string &producerName,
                                     const std::string &fileName, const double frameRate);
        
        /** Create and image consumer that streams the video using RTSP.
         \param name The name of the new consumer.
         \param producerName The name of the producer to attach to.
         \param port The port number where the RTSP stream is served.
         */
        bool createRTSPConsumer(const std::string &name, const std::string &producerName,
                                const int port);
        
        /** Create and image consumer that streams the video using WebRTC.
         \param name The name of the new consumer.
         \param producerName The name of the producer to attach to.
         \param streamName Temp placeholder for WebRTC destination endpoint.
         */
        bool createWebRTCConsumer(const std::string &name, const std::string &producerName,
                                  const std::string &streamName);
        
        /** Create an image consumer that copies the raw data to a user allocated 'buffer'.
         \param name The name of the new consumer.
         \param producerName The name of the producer to attach to.
         \param buffer The user allocated buffer big enough to hold an image from producer producerName.
         \sa getImageFormat
         */
        bool createImageBufferConsumer(const std::string &name, const std::string &producerName,
                                       uint8_t * const buffer);
        
        //=== Misc.
        //!Trigger all producers that require an external trigger.
        void trigger();
        
        //!Return a pointer to specific producer for use with an external consumer.
        std::shared_ptr<flitr::ImageProducer> getProducer(const std::string &producerName);
        
        //!Get the image format of the first image in the slot of a specific producer.
        VideoHubImageFormat getImageFormat(const std::string &producer) const;
        
    private:
        //!Vector of producers and processors that consumers may attach to.
        std::map<std::string, std::shared_ptr<flitr::ImageProducer>> _producerMap;
        
        //!Vector of consumers.
        std::map<std::string, std::shared_ptr<flitr::ImageConsumer>> _consumerMap;
    };
    
}
#endif //FLITR_VIDEO_HUB_H