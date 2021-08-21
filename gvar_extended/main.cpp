#include "plugin.h"
#include "logger.h"
#include "settings.h"
#include "modules/goes/gvar/module_gvar_image_decoder.h"

class GVARExtended : public satdump::Plugin
{
private:
    static std::string misc_preview_text;

    static void satdumpStartedHandler(const satdump::SatDumpStartedEvent &)
    {
        if (global_cfg.contains("gvar_extended"))
        {
            if (global_cfg["gvar_extended"].contains("preview_misc_text"))
                misc_preview_text = global_cfg["gvar_extended"]["preview_misc_text"].get<std::string>();
            else
                misc_preview_text = "SatDump | GVAR";
        }
    }

    static void gvarSaveChannelImagesHandler(const goes::gvar::events::GVARSaveChannelImagesEvent &evt)
    {
        logger->info("Preview... preview.png");
        cimg_library::CImg<unsigned char> preview(1300, 948, 1, 1);
        cimg_library::CImg<unsigned char> previewImage;

        // Preview generation
        {
            cimg_library::CImg<unsigned char> channel1_8bit(1040, 948, 1, 1);
            cimg_library::CImg<unsigned char> channel2_8bit(260, 237, 1, 1);
            cimg_library::CImg<unsigned char> channel3_8bit(260, 237, 1, 1);
            cimg_library::CImg<unsigned char> channel4_8bit(260, 237, 1, 1);
            cimg_library::CImg<unsigned char> channel5_8bit(260, 237, 1, 1);

            cimg_library::CImg<unsigned short> resized5 = evt.images.image5;
            resized5.resize(1040, 948);

            for (int i = 0; i < (int)channel1_8bit.size(); i++)
            {
                channel1_8bit[i] = resized5[i] / 256;
            }

            cimg_library::CImg<unsigned short> buff2 = evt.images.image1,
                                               buff3 = evt.images.image2,
                                               buff4 = evt.images.image3,
                                               buff5 = evt.images.image4;

            buff2.resize(260, 237);
            buff3.resize(260, 237);
            buff4.resize(260, 237);
            buff5.resize(260, 237);

            for (int i = 0; i < (int)channel2_8bit.size(); i++)
            {
                channel2_8bit[i] = buff2[i] / 256;
                channel3_8bit[i] = buff3[i] / 256;
                channel4_8bit[i] = buff4[i] / 256;
                channel5_8bit[i] = buff5[i] / 256;
            }

            channel1_8bit.blur_median(2);
            channel2_8bit.blur_median(2);
            channel3_8bit.blur_median(2);
            channel4_8bit.blur_median(2);
            channel5_8bit.blur_median(2);

            preview.draw_image(channel1_8bit);
            preview.draw_image(1040, 0, channel2_8bit);
            preview.draw_image(1040, 237, channel3_8bit);
            preview.draw_image(1040, 474, channel4_8bit);
            preview.draw_image(1040, 711, channel5_8bit);
        }

        // Overlay the preview
        {
            std::string sat_name = evt.images.sat_number == 13 ? "EWS-G1 / GOES-13" : ("GOES-" + std::to_string(evt.images.sat_number));
            std::string date_time = (evt.timeReadable->tm_mday > 9 ? std::to_string(evt.timeReadable->tm_mday) : "0" + std::to_string(evt.timeReadable->tm_mday)) + '/' +
                                    (evt.timeReadable->tm_mon + 1 > 9 ? std::to_string(evt.timeReadable->tm_mon + 1) : "0" + std::to_string(evt.timeReadable->tm_mon + 1)) + '/' +
                                    std::to_string(evt.timeReadable->tm_year + 1900) + ' ' +
                                    (evt.timeReadable->tm_hour > 9 ? std::to_string(evt.timeReadable->tm_hour) : "0" + std::to_string(evt.timeReadable->tm_hour)) + ':' +
                                    (evt.timeReadable->tm_min > 9 ? std::to_string(evt.timeReadable->tm_min) : "0" + std::to_string(evt.timeReadable->tm_min)) + " UTC";

            int offsetX, offsetY, bar_height;
            unsigned int text_size;

            //set ratios for calculating bar size
            float bar_ratio = 0.02;
            float text_ratio = 0.015;
            float offsetXratio = 0.005;
            float offsetYratio = 0.0025;

            cimg_library::CImg<unsigned char> imgtext, imgtext1, imgtext2;

            bar_height = preview.width() * bar_ratio;
            text_size = preview.width() * text_ratio;
            offsetX = preview.width() * offsetXratio;
            offsetY = preview.width() * offsetYratio;

            previewImage = cimg_library::CImg<unsigned char>(preview.width(), preview.height() + 2 * bar_height, 1, 1, 0);

            unsigned char color = 255;

            previewImage.fill(0);
            imgtext.draw_text(0, 0, sat_name.c_str(), &color, 0, 1, cimg_library::CImgList<unsigned char>::font(text_size, false));
            imgtext1.draw_text(0, 0, date_time.c_str(), &color, 0, 1, cimg_library::CImgList<unsigned char>::font(text_size, false));
            imgtext2.draw_text(0, 0, misc_preview_text.c_str(), &color, 0, 1, cimg_library::CImgList<unsigned char>::font(text_size, false));

            previewImage.draw_image(offsetX, offsetY, 0, 0, imgtext);
            previewImage.draw_image(previewImage.width() - imgtext1.width() - offsetX, offsetY, 0, 0, imgtext1);
            previewImage.draw_image(offsetX, bar_height + preview.height() + offsetY, 0, 0, imgtext2);
            previewImage.draw_image(0, bar_height, preview);
        }

        previewImage.save_png(std::string(evt.directory + "/preview.png").c_str());
    }

    static void gvarSaveFalceColorHandler(const goes::gvar::events::GVARSaveFCImageEvent &evt)
    {
        if (evt.sat_number == 13)
        {
            logger->info("Europe crop... europe.png");
            cimg_library::CImg<unsigned char> crop = evt.false_color_image;
            if (crop.width() == 20836)
                crop.crop(3198, 240, 3198 + 5928, 240 + 4120);
            else
                crop.crop(1348, 240, 1348 + 5928, 240 + 4120);
            crop.save_png(std::string(evt.directory + "/europe.png").c_str());
        }
    }

public:
    std::string getID()
    {
        return "gvar_extended";
    }

    void init()
    {
        satdump::eventBus->register_handler<satdump::SatDumpStartedEvent>(satdumpStartedHandler);
        satdump::eventBus->register_handler<goes::gvar::events::GVARSaveChannelImagesEvent>(gvarSaveChannelImagesHandler);
        satdump::eventBus->register_handler<goes::gvar::events::GVARSaveFCImageEvent>(gvarSaveFalceColorHandler);
    }
};

std::string GVARExtended::misc_preview_text = "SatDump | GVAR";

PLUGIN_LOADER(GVARExtended)