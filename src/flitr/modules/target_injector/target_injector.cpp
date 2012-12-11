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

#include <flitr/modules/target_injector/target_injector.h>

using namespace flitr;
using std::tr1::shared_ptr;

TargetInjector::TargetInjector(ImageProducer& producer,
                               uint32_t images_per_slot, uint32_t buffer_size) :
    ImageProcessor(producer, images_per_slot, buffer_size),
    targetBrightness_(255.0f)
{
}

TargetInjector::~TargetInjector()
{
}

bool TargetInjector::init()
{
    bool rValue=ImageProcessor::init();

    return rValue;
}

bool TargetInjector::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();

        //Start stats measurement event.
        ProcessorStats_->tick();

        //Update the synthetic targets.
        std::vector<SyntheticTarget>::iterator iter=targetVector_.begin();
        for (; iter!=targetVector_.end(); iter++)
        {
            iter->update();
        }

        unsigned int i=0;
        for (i=0; i<ImagesPerSlot_; i++)
        {
            Image const * const imRead = *(imvRead[i]);
            Image * const imWrite = *(imvWrite[i]);
            const ImageFormat imFormat=getFormat(i);
            const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();

            const uint32_t width=imFormat.getWidth();
            const uint32_t height=imFormat.getHeight();
            const uint32_t bytesPerPixel=imFormat.getBytesPerPixel();
            uint8_t const * const dataRead=imRead->data();
            uint8_t * const dataWrite=imWrite->data();

            uint32_t offset=0;

            //Copy the read/upstream image to the write/downstream image.
            // The read and write images have the same format.
            memcpy(dataWrite, dataRead, imFormat.getBytesPerImage());

            //Do image processing here...
            #pragma omp parallel for
            for (uint32_t y=0; y<height; y++)
            {
                for (uint32_t x=0; x<width; x++)
                {
                    float targetSupportDensity=0.0;

                    std::vector<SyntheticTarget>::iterator iter=targetVector_.begin();
                    for (; iter!=targetVector_.end(); iter++)
                    {
                        targetSupportDensity=1.0f - (1.0f-targetSupportDensity)*(1.0f-iter->getSupportDensity(x+0.5f, y+0.5f));
                    }

                    switch (pixelFormat)
                    {
                    case ImageFormat::FLITR_PIX_FMT_Y_8 :
                        dataWrite[offset]=(uint8_t)(dataRead[offset]*(1.0f-targetSupportDensity)+targetBrightness_*targetSupportDensity+0.5f);
                        break;
                    case ImageFormat::FLITR_PIX_FMT_RGB_8 :
                        dataWrite[offset]=(uint8_t)(dataRead[offset]*(1.0f-targetSupportDensity)+targetBrightness_*targetSupportDensity+0.5f);
                        dataWrite[offset+1]=(uint8_t)(dataRead[offset+1]*(1.0f-targetSupportDensity)+targetBrightness_*targetSupportDensity+0.5f);
                        dataWrite[offset+2]=(uint8_t)(dataRead[offset+2]*(1.0f-targetSupportDensity)+targetBrightness_*targetSupportDensity+0.5f);
                        break;
                    case ImageFormat::FLITR_PIX_FMT_Y_16 :
                        dataWrite[offset]=(uint8_t)(dataRead[offset]*(1.0f-targetSupportDensity)+targetBrightness_*targetSupportDensity+0.5f);
                        dataWrite[offset+1]=(uint8_t)(dataRead[offset+1]*(1.0f-targetSupportDensity)+targetBrightness_*targetSupportDensity+0.5f);
                        break;
                        //default:
                    }

                    offset+=bytesPerPixel;
                }
            }


        }

        //Stop stats measurement event.
        ProcessorStats_->tock();

        releaseWriteSlot();
        releaseReadSlot();

        return true;
    }

    return false;
}
