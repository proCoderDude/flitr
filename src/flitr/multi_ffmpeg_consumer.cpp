/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
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

#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

void MultiFFmpegConsumerThread::run() 
{
    size_t num_writers = Consumer_->ImagesPerSlot_;
    std::vector<Image**> imv;

    while (true) {
        // check if image available
        imv.clear();
        imv = Consumer_->reserveReadSlot();
        if (imv.size() >= num_writers) { // allow selection of some sources                       
            std::lock_guard<std::mutex> scopedLock(Consumer_->WritingMutex_);
            
            if (Consumer_->Writing_) {
                Consumer_->MultiWriteStats_->tick();

                {
                int i=0;
                #pragma omp parallel for
                for (i=0; i<(int)num_writers; i++) {
                    if (Consumer_->FFmpegWriters_[i])
                    {
                        Image* im = *(imv[i]);

                        Consumer_->FFmpegWriters_[i]->writeVideoFrame(im->data());
                        Consumer_->MetadataWriters_[i]->writeFrame(*im);
                    }
                }
                }

                Consumer_->MultiWriteStats_->tock();
            } else {
                // just discard
            }
            // indicate we are done with the image/s
            Consumer_->releaseReadSlot();
        } else {
            // wait a while for producers.
            FThread::microSleep(1000);
        }
        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

MultiFFmpegConsumer::MultiFFmpegConsumer(ImageProducer& producer,
                                         uint32_t images_per_slot) :
    ImageConsumer(producer),
    ImagesPerSlot_(images_per_slot),
    Codec_(FLITR_RAWVIDEO_CODEC),
    BitRate_(-1),
    Container_(FLITR_ANY_CONTAINER),
    Writing_(false)
{
    std::stringstream write_stats_name;
    write_stats_name << " MultiFFmpegConsumer::write";
    MultiWriteStats_ = std::shared_ptr<StatsCollector>(new StatsCollector(write_stats_name.str()));

    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(producer.getFormat(i));
    }
}

MultiFFmpegConsumer::~MultiFFmpegConsumer()
{
    closeFiles();

    Thread_->setExit();
    Thread_->join();
}

bool MultiFFmpegConsumer::setCodec(VideoCodec codec, int32_t bit_rate)
{
    Codec_=codec;
    BitRate_=bit_rate;

    return true;
}

bool MultiFFmpegConsumer::setContainer(VideoContainer container)
{
    Container_=container;

    return true;
}

bool MultiFFmpegConsumer::init()
{
    FFmpegWriters_.resize(ImagesPerSlot_);
    MetadataWriters_.resize(ImagesPerSlot_);

    Thread_ = new MultiFFmpegConsumerThread(this);
    Thread_->startThread();

    return true;
}

bool MultiFFmpegConsumer::openFiles(std::string basename, const uint32_t frame_rate)
{
    std::vector<std::string> postfixes;

    for (unsigned int i=0; i<ImagesPerSlot_; i++)
    {
        char c_count[16];
        sprintf(c_count, "%02d", i+1);
        postfixes.push_back(std::string(c_count));
    }

    return openFiles(basename,postfixes,frame_rate);
}

bool MultiFFmpegConsumer::openFiles(std::string basename, std::vector<std::string> basename_postfixes, const uint32_t frame_rate)
{
    if (basename_postfixes.size()==ImagesPerSlot_)
    {
        std::vector<std::string> filenames;

        for (unsigned int i=0; i<ImagesPerSlot_; i++)
        {
            filenames.push_back(basename + "_" + basename_postfixes[i]);
        }

        return openFiles(filenames,frame_rate);
    } else
    {
        return false;
    }
}

bool MultiFFmpegConsumer::openFiles(std::vector<std::string> filenames, const uint32_t frame_rate, std::vector<MetadataWriter*> metadata_writers)
{
    /* If the size is 0, the default value, then this function will create default
     * MetadataWriters. Otherwise the size must match the number of images in each slot. */
    if((metadata_writers.size() != 0 ) && (metadata_writers.size() != ImagesPerSlot_))
    {
        return false;
    }
    bool useDefaultMetadataWriter = (metadata_writers.size() == 0);

    if (filenames.size()==ImagesPerSlot_)
    {
        for (unsigned int i=0; i<ImagesPerSlot_; i++)
        {
            if (filenames[i]!="")
            {
                std::string video_filename(filenames[i] + ".avi");

                if (Container_==FLITR_MKV_CONTAINER)
                {
                    video_filename=std::string(filenames[i] + ".mkv");
                } else
                    if (Container_==FLITR_RTSP_CONTAINER)
                    {
                        video_filename=std::string(filenames[i]);
                    }

                std::string metadata_filename(filenames[i] + ".meta");

                FFmpegWriters_[i] = new FFmpegWriter(video_filename, ImageFormat_[i], frame_rate, Container_, Codec_, BitRate_);
                
                
                
                if(useDefaultMetadataWriter == true) {
                    MetadataWriters_[i] = new MetadataWriter(metadata_filename);
                } else {
                    MetadataWriters_[i] = metadata_writers[i];
                }
            } else
            {//If the filename is "" then the recording is disbaled.
                FFmpegWriters_[i] = 0;
                MetadataWriters_[i] = 0;
            }
        }
        return true;
    } else
    {
        return false;
    }
}


bool MultiFFmpegConsumer::startWriting()
{
    std::lock_guard<std::mutex> scopedLock(WritingMutex_);
    Writing_ = true;
    return true;
}

bool MultiFFmpegConsumer::stopWriting()
{
    std::lock_guard<std::mutex> scopedLock(WritingMutex_);
    Writing_ = false;
    return true;
}

uint32_t MultiFFmpegConsumer::closeFiles()
{
    stopWriting();
    
    uint32_t numFrames = 0;
    for (unsigned int i=0; i<ImagesPerSlot_; i++) {
        if (FFmpegWriters_[i] != 0) {
            uint32_t f = static_cast<uint32_t>(FFmpegWriters_[i]->getNumImages());
            if (f > numFrames) numFrames = f;
            delete FFmpegWriters_[i];
            FFmpegWriters_[i] = 0;
        }
        if (MetadataWriters_[i] != 0) {
            delete MetadataWriters_[i];
            MetadataWriters_[i] = 0;
        }
    }
    return numFrames;
}

uint32_t MultiFFmpegConsumer::getNumImages() const
{
    uint32_t numFrames = 0;
    for (unsigned int i=0; i<ImagesPerSlot_; i++) 
    {
        if (FFmpegWriters_[i] != 0) 
        {
            uint32_t f = static_cast<uint32_t>(FFmpegWriters_[i]->getNumImages());
            if (f > numFrames) numFrames = f;
        }
    }
    return numFrames;
}
