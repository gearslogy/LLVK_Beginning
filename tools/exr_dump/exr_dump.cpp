//
// Created by liuya on 12/11/2024.
//

#include "exr_dump.h"
#define TINYEXR_IMPLEMENTATION
#include <LLVK_Utils.hpp>
#include "libs/tinyexr/tinyexr.h"
#include "libs/tinyexr/deps/miniz/miniz.h"
#include "LLVK_Utils.hpp"
LLVK_NAMESPACE_BEGIN
bool GetEXRLayers(const char *filename)
{
    const char** layer_names = nullptr;
    int num_layers = 0;
    const char *err = nullptr;
    int ret = EXRLayers(filename, &layer_names, &num_layers, &err);

    if (err) {
        fprintf(stderr, "EXR error = %s\n", err);
    }

    if (ret != 0) {
        fprintf(stderr, "Load EXR err: %s\n", err);
        return false;
    }
    if (num_layers > 0)
    {
        fprintf(stdout, "EXR Contains %i Layers\n", num_layers);
        for (int i = 0; i < num_layers; ++i) {
            fprintf(stdout, "Layer %i : %s\n", i + 1, layer_names[i]);
        }
    }
    free(layer_names);
    return true;
}


void exr_dump::load(const char *filename) {
    auto getPixelType = [](const EXRChannelInfo &chInfo) {
        const auto pixelType = chInfo.pixel_type;
        std::string sPixelType;
        if (pixelType == TINYEXR_PIXELTYPE_HALF) sPixelType = "HALF";
        else if (pixelType == TINYEXR_PIXELTYPE_FLOAT) sPixelType = "FLOAT";
        else if (pixelType == TINYEXR_PIXELTYPE_UINT) sPixelType = "UINT";
        else sPixelType = "UNKNOWN ERROR";
        return sPixelType;
    };
    auto readHALFtoFloat= [](EXRHeader &header) {
        // Read HALF channel as FLOAT.
        for (int i = 0; i < header.num_channels; i++) {
            if (header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
                header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
            }
        }
    };

    EXRVersion exr_version;
    const char *err = nullptr;
    int ret = ParseEXRVersionFromFile(&exr_version, filename);
    if (ret != 0) {
        fprintf(stderr, "Invalid EXR file or version\n");
        return;
    }

    struct {
        EXRHeader **exr_headers; // list of EXRHeader pointers.
        int num_exr_headers;
    } multiPartHeader{};

    bool isMultiPart{true};
    ret = ParseEXRMultipartHeaderFromFile(&multiPartHeader.exr_headers, &multiPartHeader.num_exr_headers, &exr_version,
                                          filename, &err);
    if (ret != 0) {
        fprintf(stderr, "Not multi part exr: %s, roll back to single part exr reader: ", err);
        FreeEXRErrorMessage(err); // free's buffer for an error message
        isMultiPart = false;
    }

    if (not isMultiPart) {
        // 初始化并解析header
        EXRHeader header;
        InitEXRHeader(&header);
        ret = ParseEXRHeaderFromFile(&header, &exr_version, filename, &err);
        if (ret != 0) {
            fprintf(stderr, "Parse single part exr header failed: %s\n, quit loader", err);
            FreeEXRErrorMessage(err);
            return;
        }

        // 打印基本信息
        printf("EXR version: %d\n", exr_version.version);
        printf("Number of channels: %d\n", header.num_channels);

        // 遍历并打印所有通道的信息
        for (int i = 0; i < header.num_channels; i++) {
            const auto pixelType = getPixelType(header.channels[i]);
            printf("Channel[%d]: name = %s, pixel_type = %s, sampling = (%d, %d)\n",
                i,
                header.channels[i].name,
                pixelType.data(),
                header.channels[i].x_sampling,
                header.channels[i].y_sampling
            );
        }
        readHALFtoFloat(header);   // Read HALF channel as FLOAT.

        EXRImage exr_image;
        InitEXRImage(&exr_image);

        ret = LoadEXRImageFromFile(&exr_image, &header, filename, &err);
        if (ret != 0) {
            fprintf(stderr, "Load EXR err: %s\n", err);
            FreeEXRHeader(&header);
            FreeEXRErrorMessage(err); // free's buffer for an error message
            return ;
        }
        auto *imageDataA = reinterpret_cast<const float*>(exr_image.images[0]);
        auto *imageDataB = reinterpret_cast<const float*>(exr_image.images[1]);
        auto *imageDataG = reinterpret_cast<const float*>(exr_image.images[2]);
        auto *imageDataR = reinterpret_cast<const float*>(exr_image.images[3]);
        auto pixelDataCount = exr_image.width * exr_image.height;
        if (exr_image.num_channels==4) {
            std::cout <<  "first pixel value:" << imageDataR[0] << " " << imageDataG[0] << " "<< imageDataB[0] << " " << imageDataA[0] << std::endl;
            std::cout <<  "last pixel value:" << imageDataR[pixelDataCount-1] << " " << imageDataG[pixelDataCount-1] << " "<< imageDataB[pixelDataCount-1] << " " << imageDataA[pixelDataCount-1] << std::endl;
        }
        if (exr_image.num_channels==3) {
            //std::cout << imageData[0]  << std::endl; // A
        }

        FreeEXRImage(&exr_image);
        FreeEXRHeader(&header);
    }


    if (isMultiPart) {
        const auto numExrHeaders = multiPartHeader.num_exr_headers;
        printf("exr num parts: %d\n", numExrHeaders);
        for (int i = 0; i < numExrHeaders; ++i) {
            EXRHeader *exr_header = multiPartHeader.exr_headers[i];
            auto numChan = exr_header->num_channels;
            std::cout << exr_header->name << " chunk_count:" << exr_header->chunk_count << "  num_channels:" << numChan
                    << std::endl;

            for (int ch = 0; ch < numChan; ch++) {
                auto pixelType = getPixelType(exr_header->channels[ch]);
                printf("\t Channel[%d]: name = %s, pixel_type = %s, sampling = (%d, %d)\n",
                    ch,
                    exr_header->channels[ch].name,
                    pixelType.data(),
                    exr_header->channels[ch].x_sampling,
                    exr_header->channels[ch].y_sampling);

                // IF HEADER IS HALF!
                readHALFtoFloat(*exr_header);
            }
        }

        // Prepare array of EXRImage.
        std::vector<EXRImage> images(numExrHeaders);
        for (int i = 0; i < numExrHeaders; i++) {
            InitEXRImage(&images[i]);
        }
        ret = LoadEXRMultipartImageFromFile(&images.at(0), const_cast<const EXRHeader **>(multiPartHeader.exr_headers),
                                            numExrHeaders, filename, &err);
        if (ret != 0) {
            fprintf(stderr, "Parse EXR err: %s\n", err);
            FreeEXRErrorMessage(err); // free's buffer for an error message
            return;
        }
        printf("Loaded %d part images, now dump image info use ExrImage\n", numExrHeaders);

        // 4. Access image data
        // `exr_image.images` will be filled when EXR is scanline format.
        // `exr_image.tiled` will be filled when EXR is tiled format.
        for (const auto &&[index, image] : UT_Fn::enumerate( images) ) {
            const auto &exr_header = *multiPartHeader.exr_headers[index];
            std::cout << "\t"<< "image name:" << exr_header.name <<" image num tiles:"<< image.num_tiles << " width: "<< image.width << " height:"<< image.height << " num chans:"<< image.num_channels  << std::endl;


            if (image.num_channels==4) {
                auto *imageDataA = reinterpret_cast<const float*>(image.images[0]);
                auto *imageDataB = reinterpret_cast<const float*>(image.images[1]);
                auto *imageDataG = reinterpret_cast<const float*>(image.images[2]);
                auto *imageDataR = reinterpret_cast<const float*>(image.images[3]);
                auto pixelDataCount = image.width * image.height;
                std::cout <<  "\t\tfirst pixel value:" << imageDataR[0] << " " << imageDataG[0] << " "<< imageDataB[0] << " " << imageDataA[0] << std::endl;
                std::cout <<  "\t\tlast pixel value:" << imageDataR[pixelDataCount-1] << " " << imageDataG[pixelDataCount-1] << " "<< imageDataB[pixelDataCount-1] << " " << imageDataA[pixelDataCount-1] << std::endl;
            }
            if (image.num_channels==3) {
                //std::cout << imageData[0]  << std::endl; // A
            }

        }


        // 5. Free images
        for (int i =0; i < numExrHeaders; i++) {
            FreeEXRImage(&images.at(i));
        }
        // 6. Free headers.
        for (int i =0; i < numExrHeaders; i++) {
            FreeEXRHeader(multiPartHeader.exr_headers[i]);
            free(multiPartHeader.exr_headers[i]);
        }
        free(multiPartHeader.exr_headers);
    }

}

void exr_dump::directLoadRGBA(const char *filename) {
    std::cout << "directLoadRGBA:" << filename << std::endl;
    EXRVersion exr_version;
    const char *err = nullptr;
    int ret = ParseEXRVersionFromFile(&exr_version, filename);
    if (ret != 0) {
        fprintf(stderr, "Invalid EXR file or version\n");
        return;
    }

    // 初始化并解析header
    EXRHeader header;
    InitEXRHeader(&header);
    ret = ParseEXRHeaderFromFile(&header, &exr_version, filename, &err);
    if (ret != 0) {
        fprintf(stderr, "Parse single part exr header failed: %s\n, quit loader", err);
        FreeEXRErrorMessage(err);
        return;
    }
    if (header.num_channels != 4) {
        std::cout << "wrong number of channels != 4, not RGBA" << std::endl;
        return ;
    }

    // READ
    float* out; // width * height * RGBA
    int width;
    int height;
    ret = LoadEXR(&out, &width, &height, filename, &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            fprintf(stderr, "ERR : %s\n", err);
            FreeEXRErrorMessage(err); // release memory of error message.
        }
        FreeEXRHeader(&header);
        return;
    }


    auto lastRow = height - 1;
    auto lastCol = width - 1;
    auto lastPixelIndex = (lastRow * width + lastCol) * 4;

    std::cout <<  "first pixel value:" << out[0] << " " << out[1] << " "<< out[2] << " " << out[3] << std::endl;
    std::cout <<  "last pixel value:" << out[lastPixelIndex+ 0 ] << " " << out[lastPixelIndex + 1] << " "<< out[lastPixelIndex + 2] << " " << out[lastPixelIndex + 3] << std::endl;
    free(out);
    FreeEXRHeader(&header);
}


LLVK_NAMESPACE_END



int main(int argc, char *argv[]) {
    using namespace LLVK;
    if(argc < 2) {
        std::cout << "Error, Usage: " << argv[0] << " <exr_file>" << std::endl;
        return -1;
    }
    const char *file = argv[1];
    exr_dump dump;
    dump.load(file);
    //dump.directLoadRGBA(file);
    return 0;
}

