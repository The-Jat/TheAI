/*
 * 
 */
#ifndef _NN_H
#define _NN_H


#include <util/DoublyLinkedList.h>
#ifdef __cplusplus
extern "C" {
#endif

const char* NN_PORT = "neural listener";

typedef struct {
	area_id areaID;
	char* address;
	long unsigned int size;
} kile;

typedef enum{

	NN_INIT = 0,
} nn_msg;

class Msg : public DoublyLinkedListLinkImpl<Msg> {
public:
	Msg(){}
	//int str;


	/*Data(String input){
		str = input;
	}*/
	
	/*void set(int input){
		str = input;
	}*/
	
	/*string Display(){
		return str;
	}*/
	
	//int& Data(){ return str;}
	
	void SetCode(nn_msg code){
		fCode = code;
	}
	
	nn_msg Code() const{
		return fCode;
	}
	
	kile &Data(){
		return fData;
	}
	
private:
	kile fData;
	nn_msg fCode;

};





#ifdef __cplusplus
}
#endif

#endif /* _NN_H */
