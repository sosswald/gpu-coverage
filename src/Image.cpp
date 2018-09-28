/*
 * Copyright (c) 2018, Stefan Osswald
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <gpu_coverage/Image.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Utilities.h>
#if HAS_OPENCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include <stdexcept>
#include <iostream>
#include <map>
#include <sstream>

namespace gpu_coverage {

static std::map<std::string, Image*> CACHE;

Image::Image(const aiTexture * const texture, const size_t& id)
        : id(id), usage(0), path() {
    if (texture->mHeight == 0) {
        throw std::runtime_error("Cannot handle embedded compressed textures");
    }
    mat = cv::Mat(texture->mHeight, texture->mWidth, CV_8UC4);
    memcpy(mat.data, texture->pcData, texture->mHeight * texture->mWidth * 4);
}

Image::Image(const std::string& path)
        : id(0), usage(0), path(path) {
#if HAS_OPENCV
    cv::Mat orig = cv::imread(path.c_str(), cv::IMREAD_COLOR);
    cv::Mat flip(orig.rows, orig.cols, orig.type());
    cv::flip(orig, flip, 0);

    int code;
    switch(orig.channels()) {
    case 1:
        code = cv::COLOR_GRAY2RGBA;
        break;
    case 3:
        code = cv::COLOR_BGR2RGBA;
        break;
    case 4:
        code = cv::COLOR_BGRA2RGBA;
        break;
    default:
        throw std::runtime_error(std::string("Unkown color format for image from ") + path);
    }
    cv::cvtColor(flip, mat, code, 4);
    if (!mat.data) {
        throw std::runtime_error(std::string("Could not load image from ") + path);
    }
#else
#if __ANDROID__
    AAsset *asset = AAssetManager_open(Config::getAssetManager(), (std::string("models") + path).c_str(), AASSET_MODE_UNKNOWN);
    if (!asset) {
        logError("Could not open image file models%s", path.c_str());
        return;
    }
    size_t len = AAsset_getLength(asset);
    char *data = (char *) calloc(len, sizeof(char));
    AAsset_read(asset, data, len);
    AAsset_close(asset);
    std::istringstream ifs(data);
    std::string header;
    ifs >> header;
    bool ok = true;
    if (header != "P6") {
        logError("Unknown image format for file %s, only PPM P6 supported", path.c_str());
        ok = false;
    }
    if (ok) {
        size_t width, height, valueRange;
        ifs >> width >> height >> valueRange;
        if (valueRange != 255) {
            logError("Unexpected value range for P6 file %s", path.c_str());
        } else {
            const size_t start = static_cast<size_t>(ifs.tellg()) + 1;
            mat = cv::Mat(width, height, CV_8UC4);
            size_t i = 0;
            for (size_t y = 0; y < height; ++y) {
                for (size_t x = 0; x < width; ++x) {
                    const size_t outPixelIdx = ((height - y - 1) * width + x) * 4;
                    mat.data[outPixelIdx + 0] = data[start + i]; ++i;
                    mat.data[outPixelIdx + 1] = data[start + i]; ++i;
                    mat.data[outPixelIdx + 2] = data[start + i]; ++i;
                    mat.data[outPixelIdx + 3] = 0xFF;
                }
            }
        }
    }
    free(data);
#else
    unsigned char data[256 * 256 * 4];
    for (size_t x = 0; x < 256; ++x) {
        for (size_t y = 0; y < 256; ++y) {
            const bool c = ((x / 10) % 2) == ((y / 10) % 2);
            data[(y * 256 + x) * 4 + 0] = c ? 255 : 128;
            data[(y * 256 + x) * 4 + 1] = 128;
            data[(y * 256 + x) * 4 + 2] = 128;
            data[(y * 256 + x) * 4 + 3] = 255;
        }
    }
    mat = cv::Mat(256, 256, CV_8UC4, &data);
#endif
#endif
}

Image::Image(const int rows, const int cols, const int type, const unsigned char * const data)
        : id(0), mat(rows, cols, type), usage(0), path() {
    if (data) {
        memcpy(mat.data, data, rows * cols * 4);
    }
}

Image::~Image() {
}

const Image * Image::get(const std::string& path) {
    std::map<std::string, Image*>::const_iterator it = CACHE.find(path);
    if (it == CACHE.end()) {
        Image * texture = new Image(path);
        CACHE[path] = texture;
        ++texture->usage;
        return texture;
    } else {
        ++it->second->usage;
        return it->second;
    }
}

void Image::release(const Image * image) {
    if (!image) {
        return;
    }
    if (image->usage > 0) {
        --image->usage;
        if (image->usage == 0) {
            std::map<std::string, Image*>::iterator it = CACHE.find(image->path);
            if (it != CACHE.end()) {
                CACHE.erase(it);
            }
            delete image;
        }
    }
}

} /* namespace gpu_coverage */
