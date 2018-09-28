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

#include <gpu_coverage/Config.h>
#include <gpu_coverage/Utilities.h>
#include <fstream>
#include <sstream>

namespace gpu_coverage {

#if __ANDROID__
AAssetManager *Config::apkAssetManager = NULL;
#endif

Config * Config::instance = NULL;

void Config::init(const int argc, const char * const argv[]) {
    std::string filename = std::string(DATADIR "/config/config.txt");
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            filename = std::string(DATADIR "/") + std::string(argv[i]);
            break;
        }
    }
    instance = new Config(filename);
}

Config::Config(const std::string& filename)
        : filename(filename) {
    params["file"] = new Param<std::string>("file", "Filename to the CAD model of the environment", "models/cupboard.dae");
    params["panoOutputFormat"] = new Param<PanoOutputValue>("panoOutputFormat",
            "Output format for panorama (IMAGE_STRIP, CUBE)", Config::IMAGE_STRIP_HORIZONTAL);
    params["tesselate"] = new Param<bool>("tesselate", "Apply tesselation shader", false);
    params["renderToCubemap"] = new Param<bool>("renderToCubemap", "Render to cubemap instead of texture array", true);
    params["panoCamera"] = new Param<std::string>("panoCamera", "Node where the panorama camera should be attached",
            "Camera_001");
    params["externalCamera"] = new Param<std::string>("externalCamera", "Node where the external camera is attached",
            "Camera");
    params["robotCamera"] = new Param<std::string>("robotCamera", "Node where the robot camera is attached",
            "RobotCamera");
    params["target"] = new Param<std::string>("target", "Name of the target node that should be covered", "target");
    params["floor"] = new Param<std::string>("floor", "Name of the floor node", "floor");
    params["floorProjection"] = new Param<std::string>("floorProjection", "Name of the floor projection node", "floorProjection");
    params["projectionPlane"] = new Param<std::string>("projectionPlane",
            "Name of the plane node onto which the costmap is projected", "Plane");
    params["costCameraHeightChange"] = new Param<float>("costCameraHeightChange",
            "Costs for changing the camera height above the ground", 0.1);
    params["costDistance"] = new Param<float>("costDistance", "Costs for the robot walking", 0.1);
    params["minCameraHeight"] = new Param<float>("minCameraHeight", "Minimum camera height above the ground", 0.5);
    params["maxCameraHeight"] = new Param<float>("minCameraHeight", "Minimum camera height above the ground", 0.6);
    params["gainFactor"] = new Param<float>("gainFactor",
            "Scaling factor for the information gain when evaluating pose", 1e-4);
    params["panoSemantic"] = new Param<bool>("panoSemantic", "Render panorama with semantic colors", true);
    load();
}

Config::~Config() {
    for (Params::iterator it = params.begin(); it != params.end(); ++it) {
        delete it->second;
    }
    params.clear();
}

void Config::save() const {
#if __ANDROID__
    return;
#endif
    std::ofstream ofs(filename.c_str());
    for (Params::const_iterator mapIt = params.begin(); mapIt != params.end(); ++mapIt) {
        ofs << mapIt->second->name << " " << (*mapIt->second) << std::endl;
    }
    ofs.close();
}

void Config::load() {
#if __ANDROID__
    if (!apkAssetManager) {
        logError("Asset manager not initialized");
    }
    AAsset* asset = AAssetManager_open(apkAssetManager, filename.c_str()[0] == '/' ? &filename.c_str()[1] : filename.c_str(), AASSET_MODE_UNKNOWN);
    if (!asset) {
        logError("Could not open config file %s", filename.c_str());
        return;
    }
    const char *data = (const char *) AAsset_getBuffer(asset);
    std::istringstream ifs(data);
#else
    std::ifstream ifs(filename.c_str());
    if (!ifs.good()) {
        logWarn("Config file %s not found, loading default parameters", filename.c_str());
    }
#endif
    while (ifs.good()) {
        std::string name;
        ifs >> name;
        if (name.empty()) {
            continue;
        }
        if (name[0] == '#') {
            std::string dummy;
            std::getline(ifs, dummy);
            continue;
        }
        Params::iterator mapIt = params.find(name);
        if (mapIt == params.end()) {
            logError("Warning: unknown parameter %s in config file", name.c_str());
            std::string dummy;
            std::getline(ifs, dummy);
        } else {
            ifs >> *mapIt->second;
        }
    }
#if __ANDROID__
    AAsset_close(asset);
#else
    ifs.close();
#endif
}

template<class T>
void Config::Param<T>::write(std::ostream& os) const {
    os << value;
}

template<class T>
void Config::Param<T>::read(std::istream& is) throw (std::invalid_argument) {
    is >> value;
}

template<>
void Config::Param<bool>::write(std::ostream& os) const {
    os << (value ? "true" : "false");
}

template<>
void Config::Param<bool>::read(std::istream& is) throw (std::invalid_argument) {
    std::string v;
    is >> v;
    if (v.compare("true") == 0) {
        value = true;
    } else if (v.compare("false") == 0) {
        value = false;
    } else {
        throw std::invalid_argument("invalid argument for boolean parameter " + name + ": must be true or false");
    }
}

template<>
void Config::Param<std::string>::read(std::istream& is) throw (std::invalid_argument) {
    // ignore 1 tab or n spaces
    char nextChar = is.peek();
    if (nextChar == '\t') {
        is.ignore(1, '\n');
    } else {
        while (nextChar == ' ') {
            is.ignore(1, '\n');
            nextChar = is.peek();
        }
    }
    std::getline(is, value);
    // strip spaces at end of line
    for (int i = value.size() - 1; i >= 0; --i) {
        if (value.at(i) != ' ') {
            value.erase(i + 1);
            break;
        }
    }
}

template<>
void Config::Param<Config::PanoOutputValue>::write(std::ostream& os) const {
    switch (value) {
    case IMAGE_STRIP_HORIZONTAL:
        os << "IMAGE_STRIP_HORIZONTAL";
        break;
    case IMAGE_STRIP_VERTICAL:
        os << "IMAGE_STRIP_VERTICAL";
        break;
    case EQUIRECTANGULAR:
        os << "EQUIRECTANGULAR";
        break;
    case CYLINDRICAL:
        os << "CYLINDRICAL";
        break;
    case CUBE:
        os << "CUBE";
        break;
    }
}

template<>
void Config::Param<Config::PanoOutputValue>::read(std::istream& is) throw (std::invalid_argument) {
    std::string v;
    is >> v;
    if (v.compare("IMAGE_STRIP_HORIZONTAL") == 0) {
        value = IMAGE_STRIP_HORIZONTAL;
    } else if (v.compare("IMAGE_STRIP_VERTICAL") == 0) {
        value = IMAGE_STRIP_VERTICAL;
    } else if (v.compare("EQUIRECTANGULAR") == 0) {
        value = EQUIRECTANGULAR;
    } else if (v.compare("CUBE") == 0) {
        value = CUBE;
    } else if (v.compare("CYLINDRICAL") == 0) {
        value = CYLINDRICAL;
    } else {
        throw std::invalid_argument(
                "invalid argument for panoOutputFormat parameter " + name + ": is " + v
                        + ", but must be one of:\n * IMAGE_STRIP_HORIZONTAL\n * IMAGE_STRIP_VERTICAL\n * CUBE\n * EQUIRECTANGULAR\n * CYLINDRICAL");
    }
}

std::ostream& operator<<(std::ostream& os, Config::AbstractParam& param) {
    param.write(os);
    return os;
}
std::istream& operator>>(std::istream& is, Config::AbstractParam& param) {
    param.read(is);
    return is;
}

} /* namespace gpu_coverage */
