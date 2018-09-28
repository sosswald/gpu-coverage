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

#ifndef INCLUDE_ARTICULATION_CONFIG_H_
#define INCLUDE_ARTICULATION_CONFIG_H_

#include <string>
#include <iostream>
#include <stdexcept>
#include <map>
#include <vector>
#if __ANDROID__
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

namespace gpu_coverage {

/**
 * @brief Singleton class for storing configuration.
 */
class Config {
public:
    /**
     * @brief Possible values for the panoOutputFormat parameter.
     */
    enum PanoOutputValue {
        IMAGE_STRIP_HORIZONTAL,//!< Six cube map sides aligned horizontally (right-left-top-bottom-back-front)
        IMAGE_STRIP_VERTICAL,  //!< Six cube map sides aligned vertically (right-left-top-bottom-back-front)
        CUBE,                  //!< Cube sides folded onto a 4x3 grid
        EQUIRECTANGULAR,       //!< Equirectangular projection image with aspect ratio 2:1
        CYLINDRICAL            //!< Cylindrical projection image with aspect ratio 2:1
    };

protected:
    /**
     * @brief Protected constructor, loads configuration from file.
     * @param[in] filename Name of the file for loading the configuration.
     */
    Config(const std::string& filename);
    /**
     * @brief Destructor.
     */
    virtual ~Config();

    const std::string filename; ///< The file name for loading and storing the configuration data.

    /**
     * @brief Abstract class representing a configuration parameter.
     */
    struct AbstractParam {
        const std::string name;            ///< The name of the descriptor.
        const std::string description;     ///< A human-readable description of the parameter.

        /**
         * @brief Stream operator for writing this parameter to a config data stream.
         * @param[in] os Output stream.
         * @param[in] param The parameter to write.
         * @return The output stream for chaining.
         */
        friend std::ostream& operator<<(std::ostream& os, AbstractParam& param);

        /**
         * @brief Stream operator for reading this parameter from a config data stream.
         * @param[in] is Input stream.
         * @param[in] param The parameter to read.
         * @return The input stream for chaining.
         */
        friend std::istream& operator>>(std::istream& is, AbstractParam& param);

        /**
         * @brief Write this parameter to a configuration file output stream.
         * @param[in] os The output stream to write to.
         */
        virtual void write(std::ostream& os) const = 0;

        /**
         * @brief Read this parameter from a configuration file input stream.
         * @param[in] is The input stream to read from.
         */
        virtual void read(std::istream& is) = 0;
        /**
         * @brief Constructor.
         * @param[in] name The name of the parameter.
         * @param[in] description A human-readable description of the parameter.
         */
        AbstractParam(const std::string& name, const std::string& description)
                : name(name), description(description) {
        }
        /**
         * @brief Destructor.
         */
        virtual ~AbstractParam() {
        }
    };
    friend std::ostream& operator<<(std::ostream& os, AbstractParam& param);
    friend std::istream& operator>>(std::istream& is, AbstractParam& param);

    /**
     * @brief Configuration parameter.
     * @tparam The type of the configuration value.
     */
    template<class T>
    struct Param: public AbstractParam {
        T dflt;       ///< Default value of the parameter.
        T value;      ///< Current value of the parameter.
         /**
          * @brief Constructor.
          * @param[in] name The name of the parameter.
          * @param[in] description A human-readable description of the parameter.
          * @param[in] dflt The default value of the parameter.
          */
        Param(const std::string& name, const std::string& description, const T& dflt)
                : AbstractParam(name, description), dflt(dflt), value(dflt) {
        }

        /**
         * @brief Destructor.
         */
        virtual ~Param() {
        }

        /**
         * @brief Write this parameter to a configuration file output stream.
         * @param[in] os The output stream to write to.
         */
        virtual void write(std::ostream& os) const;

        /**
         * @brief Read this parameter from a configuration file input stream.
         * @param[in] is The input stream to read from.
         */
        virtual void read(std::istream& is) throw (std::invalid_argument);
    };

    typedef std::map<std::string, AbstractParam *> Params;   ///< Parameter map type, maps names to parameter structure.
    Params params;                                           ///< The parameters.
    static Config *instance;                                 ///< The singleton instance.

#if __ANDROID__
    static AAssetManager *apkAssetManager;                   ///< Asset manager for loading and storing files to disk, see setAssetManager() and getAssetManager().
#endif

public:
#if __ANDROID__
    /**
     * @brief Sets the Android asset manager.
     * @param[in] manager The Android asset manager.
     */
    static void setAssetManager(AAssetManager *manager) {
        apkAssetManager = manager;
    }
    /**
     * @brief Returns the Android asset manager.
     * @return The Adroid asset manager.
     */
    static AAssetManager * getAssetManager() {
        return apkAssetManager;
    }
#endif

    /**
     * Returns the singleton Config instance.
     * @return The Config instance.
     *
     * Warning: This method does not create an instance. Call init() first to
     * create the instance and load the configuration data.
     */
    static inline Config &getInstance() {
        return *instance;
    }

    /**
     * Create a Config instance and load the config data from a file.
     * @param[in] argc Command line argument count.
     * @param[in] argv Command line arguments.
     */
    static void init(const int argc, const char * const argv[]);

    /**
     * @brief Save the configuration to disk.
     */
    void save() const;

    /**
     * @brief Load the configuration from disk.
     */
    void load();

    /**
     * @brief Get a configuration parameter.
     * @param[in] name The name of the configuration parameter.
     * @return The current value of the configuration parameter.
     * @exception std::invalid_argument Parameter name is unknown or the parameter type mismatches the template parameter.
     *
     * If the configuration parameter has not been set, the method returns its default value.
     */
    template<class T>
    T getParam(const std::string& name) const {
        Params::const_iterator paramIt = params.find(name);
        if (paramIt == params.end()) {
            throw std::invalid_argument(std::string("Parameter ") + name + " not found");
        }
        const Param<T> * const bp = dynamic_cast<Param<T>*>(paramIt->second);
        if (!bp) {
            throw std::invalid_argument(std::string("Parameter ") + name + " is not of requested type");
        }
        return bp->value;
    }
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_CONFIG_H_ */
