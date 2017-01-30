/*
 * Copyright (c) 2016 Chris Iatrou <Chris_Paul.Iatrou@tu-dresden.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef RUN_TIME_VALUE_GENERATOR_H
#define RUN_TIME_VALUE_GENERATOR_H

#include "ipc_managed_object.h"
#include "ChimeraTK/ControlSystemAdapter/DeviceSynchronizationUtility.h"

using namespace ChimeraTK;

class runtime_value_generator : public ipc_managed_object {
private:   
	boost::shared_ptr<DevicePVManager> devManager;
	boost::shared_ptr<DeviceSynchronizationUtility> syncDevUtility;
    
public:
	runtime_value_generator(boost::shared_ptr<DevicePVManager> devManager, boost::shared_ptr<DeviceSynchronizationUtility> syncDevUtility);
	~runtime_value_generator();
	void workerThread();
	static void generateValues(boost::shared_ptr<DevicePVManager> devManager, boost::shared_ptr<DeviceSynchronizationUtility> syncDevUtility);
    
};

#endif // RUN_TIME_VALUE_GENERATOR_H
