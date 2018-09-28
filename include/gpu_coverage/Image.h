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

#ifndef INCLUDE_ARTICULATION_IMAGE_H_
#define INCLUDE_ARTICULATION_IMAGE_H_

#include <assimp/scene.h>
#include <stdexcept>
#include <cstdlib>

#if HAS_OPENCV
#include <opencv2/core/core.hpp>
#else
// Define dummy classes for systems without OpenGL (e.g. Android)
///@cond WITH_DUMMY_OPENCV
#define CV_8UC3 16
#define CV_8UC4 24
#define GL_BGR GL_RGB
#define GL_BGRA GL_RGBA
namespace cv {
    struct Mat {
        int rows;
        int cols;
        int type;
        int nchannels;
        unsigned char *data;
        Mat() : rows(0), cols(0), type(0), data(NULL) {}
        Mat(const int rows, const int cols, const int type, const void * const data) : rows(rows), cols(cols), type(type), data(NULL), allocatedData(false) {
            switch (type) {
                case CV_8UC3: nchannels = 3; break;
                case CV_8UC4: nchannels = 4; break;
                default: throw std::invalid_argument("Unknown type");
            }
            this->data = (unsigned char *) data;
        }
        Mat(const int rows, const int cols, const int type) : rows(rows), cols(cols), type(type), data(NULL), allocatedData(true) {
            switch (type) {
                case CV_8UC3: nchannels = 3; break;
                case CV_8UC4: nchannels = 4; break;
                default: throw std::invalid_argument("Unknown type");
            }
            data = (unsigned char *) calloc(rows * cols * nchannels, sizeof(unsigned char));
        }
        Mat(const Mat& other) : data(NULL), allocatedData(true) {
            rows = other.rows;
            cols = other.cols;
            type = other.type;
            nchannels = other.nchannels;
            data = (unsigned char *) calloc(rows * cols * nchannels, sizeof(unsigned char));
            memcpy(data, other.data, rows * cols * nchannels);
        }
        ~Mat() {
            if (allocatedData) {
                //TODO free leads to SIGSEGV
                //free(data);
            }
            data = NULL;
        }
        inline const int& channels() const {
            return nchannels;
        }
    private:
        bool allocatedData;
    };
}
///@endcond
#endif
#include GL_INCLUDE

namespace gpu_coverage {

/**
 * @brief Represents a 2D image.
 */
class Image {
public:
    /**
     * @brief Constructor from aiTexture.
     * @param[in] texture The Assimp aiTexture from where to load this image.
     * @param[in] id Unique ID for this image.
     *
     * For creating an Image from a file instead of an Assimp texture, use the static get() method.
     */
    Image(const aiTexture * const texture, const size_t& id);

    /**
     * @Destructor.
     */
    virtual ~Image();

    /**
     * @brief Construct and return an Image from a file.
     * @param[in] path File path from where to load the image.
     * @return A new Image instance.
     *
     * The image will be held in a cache. If this method gets
     * called multiple times, the same instance will be returned
     * from the cache and the usage counter will be incremented.
     *
     * Call release() when the image is not needed anymore. If
     * release() has been called the same number of times than
     * the get() method, the Image will be deleted and removed
     * from the cache.
     */
    static const Image * get(const std::string& path);

    /**
     * @brief Release the image.
     * @param[in] image The image to release.
     *
     * Calling this method decreases an internal usage counter.
     * If release() has been called the same number of times
     * than the get() method, the image will be deleted and
     * removed from the cache.
     */
    static void release(const Image * image);

    /**
     * @brief Returns true if the image is in use.
     * @return True if this image is in use.
     * @sa release()
     */
    inline bool inUse() const {
        return usage > 0;
    }

    /**
     * @brief Bind this image to an OpenGL image unit.
     * @param[in] unit The ID of the image unit.
     */
    void bindToUnit(const GLenum unit) const;

    /**
     * @brief Returns this image as an OpenCV image.
     * @return OpenCV image.
     */
    const cv::Mat& image() const {
        return mat;
    }

protected:
    /**
     * @brief Protected constructor for loading image from file.
     * @param[in] path File path from where to load the image.
     *
     * Do not use this constructor, use the static get() method instead to allow for caching images.
     */
    Image(const std::string& path);

    /**
     * @brief Protected constructor for creating image from raw data.
     * @param[in] rows Height of the image.
     * @param[in] cols Width of the image.
     * @param[in] type OpenCV image type.
     * @param[in] data Raw data.
     */
    Image(const int rows, const int cols, const int type, const unsigned char * const data = NULL);

    const size_t id;                 ///< Unique ID of this image.
    cv::Mat mat;                     ///< OpenCV image.
    mutable size_t usage;            ///< Internal usage counter, see get(), release() and inUse().
    const std::string path;          ///< File path from where the image was loaded.
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_IMAGE_H_ */
