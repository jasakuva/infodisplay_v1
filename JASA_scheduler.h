#ifndef JASA_scheduler_h
#define JASA_scheduler_h
class JASA_Scheduler {
private:
	int lastMillisec;
	int intervallMillisec;
	
public:
	JASA_Scheduler(int intervallMillisec);
	bool doTask();
	
};
#endif