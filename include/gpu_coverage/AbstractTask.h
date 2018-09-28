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

#ifndef INCLUDE_ARTICULATION_ABSTRACTTASK_H_
#define INCLUDE_ARTICULATION_ABSTRACTTASK_H_

#include <pthread.h>

namespace gpu_coverage {

/**
 * @brief Shared data for parallel tasks.
 *
 * This data structure contains barriers and mutexes for synchronizing parallel tasks.
 */
struct SharedData {
    pthread_barrier_t barrier;
    pthread_mutex_t mutex;
    size_t numThreads;
};

/**
 * @brief Abstract superclass for parallel tasks.
 *
 * This class is the abstract superclass for tasks can can be
 * executed in parallel.
 *
 * Subclasses must implement at least the following methods:
 * * run(): Method doing the main work of the task.
 * * finish(): Will be executed after all parallel tasks have finished.
 */
class AbstractTask {
public:
    /**
     * @brief Constructor.
     * @param[in] sharedData Task synchronization objects shared between all tasks.
     * @param[in] threadNr The number of the thread executing the task.
     *
     * Subclasses should set ready = true when the task is initialized and ready to be executed.
     */
    AbstractTask(SharedData * const sharedData, const size_t threadNr);

    /**
     * @brief Destructor.
     */
    virtual ~AbstractTask();
    /**
     * @brief Method doing the main work of the task.
     */
    virtual void run() = 0;

    /**
     * @brief Method called after all parallel tasks have finished their run() method.
     */
    virtual void finish();

    /**
     * Returns true if the task is ready to be executed.
     * @return True if task is ready.
     */
    inline bool isReady() const {
        return ready;
    }

    /**
     * Set random seed for reproducible results.
     * @param[in] seed Random seed.
     *
     * By default, a seed based on time and thread number is used.
     */
    inline void setSeed(unsigned int seed) {
        this->seed = seed;
    }

protected:
    const size_t threadNr;             ///< Thread number.
    bool ready;                        ///< Set to true when renderer is ready, see isReady().
    SharedData * const sharedData;     ///< Task synchronization objects.

    unsigned int seed;                 ///< Random seed.

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_ABSTRACTTASK_H_ */
