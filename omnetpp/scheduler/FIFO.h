//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef FIFO_H_
#define FIFO_H_

#include "IQueue.h"

#include "General.h"
using namespace std;

class FIFO : public IQueue{
private:
	list<bPacket *> waitQ;
	list<bPacket *> osQ;
public:
	FIFO(int, int);
	void pushWaitQ(bPacket *);
	bPacket * dispatchNext();
	bPacket * popOsQ(long id);
	bPacket * popOsQ();
	bPacket * queryJob(long id);
	sPacket * propagateSPacket();
	void receiveSPacket(sPacket *);
};

#endif /* FIFO_H_ */
