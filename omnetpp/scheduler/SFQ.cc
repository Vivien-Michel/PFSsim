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

#include "scheduler/SFQ.h"

FILE * SFQ::schfp = NULL;

SFQ::SFQ(int id, int deg, int totalc):IQueue(id, deg) {
	vtime = 0;
	totalClients = totalc;
	for(int i = 0; i < MAX_APP; i ++)
		maxftags[i] = 0;
	for(int wi = 0; wi < MAX_APP; wi ++){
		IQueue::weight[wi] = 1000000;
	}

#ifdef SCH_PRINT
	if(id == 0){
		char schfn[30] = "schedprint/queue";
		schfp = fopen(schfn, "w+");
		if(schfp == NULL){
			fprintf(stderr,"[ERROR] SFQ: can't open schedprint/queue file.\n");
		}
	}
#endif
}

void SFQ::pushWaitQ(bPacket * pkt){
	struct Job * job = (struct Job *)malloc(sizeof(struct Job));
	int app = pkt->getApp();
	job->pkt = pkt;
	if(waitQ[app].empty()){ // No back-logged jobs.
		job->stag = (maxftags[app] > vtime) ? maxftags[app] : vtime;
		job->ftag = job->stag + job->pkt->getSize() / IQueue::weight[app];
	}
/*
	// stag(k) = max{ftag(k-1), vtime}
	job->stag = maxftags[app];
	if(vtime > job->stag)
		job->stag = vtime;

	job->ftag = job->stag + job->pkt->getSize() / IQueue::weight[app];

	maxftags[app] = job->ftag;
*/
	waitQ[app].push_back(job);
#ifdef SCH_PRINT
	printNJ(job);
#endif
}

bPacket * SFQ::dispatchNext(){
	if((signed int)(osQ.size()) >= IQueue::degree) // If outstanding queue is bigger than the degree, stop dispatching more jobs.
		return NULL;

	// Get the job with the lowest start tag.
	double mintag = 1000000000;
	int minindex = -1;

	// Random, for fairness.
	int start = (float)rand() / RAND_MAX * totalClients;
	bool firstround = true;
	for(int i = start; ; i ++){
		if(i == totalClients)
			i = 0;
		if(i == start){
			if(firstround == true)
				firstround = false;
			else
				break;
		}
		if(waitQ[i].empty()) // No job in queue
			continue;
		// Only compare to the front (The earliest job).
		if(minindex == -1 || mintag > waitQ[i].front()->stag){ // Select the smallest stag.
			minindex = i;
			mintag = waitQ[i].front()->stag;
		}
	}
	if(minindex == -1){ // No job to schedule.
		return NULL;
	}

	Job * job = waitQ[minindex].front();
	waitQ[minindex].pop_front();
	pushOsQ(job);

	// Update vtime
	vtime = job->stag;

	// Set the tags for the next one in wait queue.
	if(waitQ[minindex].empty()){
		maxftags[minindex] = job->ftag;
	}else{ // Set the tags of the next one in the wait queue.
		Job * next = waitQ[minindex].front();
		next->stag = job->ftag; // vtime should be equal to job->stag, which is smaller than job->ftag.
		next->ftag = next->stag + next->pkt->getSize() / IQueue::weight[minindex];
	}
	bPacket * pkt = job->pkt;

#ifdef SCH_PRINT
	printDP(job);
#endif

	return pkt;
}

void SFQ::pushOsQ(Job * job){
	/*
	if(osQ.empty()){
		osQ.push_back(job);
		return;
	}
	typename list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( (*iter)->stag > job->stag ){
			osQ.insert(iter, job);
			return;
		}
	}
	*/
	osQ.push_back(job);
}

bPacket * SFQ::popOsQ(long id){
	bPacket * pkt = NULL;
	if(osQ.empty()){
		fprintf(stderr, "[ERROR] SFQ: Didn't find the job %ld in OsQ, OsQ is empty.\n", id);
		fflush(stderr);
		return NULL;
	}

	list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( (*iter)->pkt->getID() == id ){
			pkt = (*iter)->pkt;
			osQ.erase(iter);
#ifdef SCH_PRINT
	printFIN(*iter);
#endif
			free(*iter);
			break;
		}
	}
	if(pkt == NULL){
		fprintf(stderr, "[ERROR] SFQ: Didn't find the job %ld in OsQ!\n", id);
		fflush(stderr);
		return NULL;
	}

	return pkt;
}

bPacket * SFQ::queryJob(long id){
	bPacket * pkt = NULL;
	if(osQ.empty())
		return NULL;

	list<Job*>::iterator iter;
	for(iter = osQ.begin(); iter != osQ.end(); iter ++){
		if( (*iter)->pkt->getID() == id ){
			pkt = (*iter)->pkt;
			break;
		}
	}
	if(pkt == NULL){
		fprintf(stderr, "[ERROR] SFQ: Didn't find the job %ld in OsQ!\n", id);
		fflush(stderr);
		return NULL;
	}
	return pkt;
}


sPacket * SFQ::propagateSPacket(){
	fprintf(stderr, "[ERROR] SFQ: calling propagateSPacket method in SFQ is prohibited.\n");
	fflush(stderr);
	return NULL;
}

void SFQ::receiveSPacket(sPacket *){
	fprintf(stderr, "[ERROR] SFQ: calling receiveSPacket method in SFQ is prohibited.\n");
	fflush(stderr);
}


void SFQ::printNJ(Job * job){
#ifdef SCH_PRINT
	fprintf(schfp, "%.5lf\t[%d]\tNJ #%d {%d;%d #%d}\n",
			SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), waitQ[0].size(), waitQ[1].size(), osQ.size());
#endif
}

void SFQ::printDP(Job * job){
#ifdef SCH_PRINT
	fprintf(schfp, "%.5lf\t[%d]\tDP #%d [%.2lf %.2lf] {%d;%d #%d}\n",
			SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), job->stag, job->ftag, waitQ[0].size(), waitQ[1].size(), osQ.size());
#endif
}
void SFQ::printFIN(Job * job){
#ifdef SCH_PRINT
	fprintf(schfp, "%.5lf\t[%d]\tFIN #%d {%d;%d #%d}\n",
			SIMTIME_DBL(simTime()), myID, job->pkt->getApp(), waitQ[0].size(), waitQ[1].size(), osQ.size());
#endif
}
