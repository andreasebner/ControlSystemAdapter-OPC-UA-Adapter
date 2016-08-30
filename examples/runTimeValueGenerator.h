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

#include "ipc_task.h"
#include "ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h"

using namespace ChimeraTK;

typedef boost::shared_ptr<ControlSystemPVManager> shCSysPVManager;

class runTimeValueGenerator : public ipc_managed_object {
private:   
	shCSysPVManager csManager;
	
    void runTimeValueGenerator_constructserver(shCSysPVManager csManager);
    
public:
    runTimeValueGenerator(shCSysPVManager csManager);
	runTimeValueGenerator();
    ~runTimeValueGenerator();
	void workerThread();
	static void generateValues(shCSysPVManager csManager);
    
};

#endif // RUN_TIME_VALUE_GENERATOR_H
