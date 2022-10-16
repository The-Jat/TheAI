#ifndef __NN_HPP_
#define __NN_HPP_


#include <iostream>
#include <vector>
#include <math.h>

using std::vector;
using std::cout;
using std::endl;

extern vector<float> X ;

extern vector<float> y ;

extern vector<float> W;

vector <float> sigmoid_d (const vector <float>& m1);

vector <float> sigmoid (const vector <float>& m1);

vector <float> operator+(const vector <float>& m1, const vector <float>& m2);

vector <float> operator-(const vector <float>& m1, const vector <float>& m2);

vector <float> operator*(const vector <float>& m1, const vector <float>& m2);

vector <float> transpose (float *m, const int C, const int R);
vector <float> dot (const vector <float>& m1, const vector <float>& m2, const int m1_rows, const int m1_columns, const int m2_columns);

void print ( const vector <float>& m, int n_rows, int n_columns );

int main2(/*int argc, const char * argv[]*/);


#endif
