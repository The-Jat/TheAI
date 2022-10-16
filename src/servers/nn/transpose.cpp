vector <float> transpose (float *m, const int C, const int R) {
    
    /*  Returns a transpose matrix of input matrix.
        Inputs:
            m: vector, input matrix
            C: int, number of columns in the input matrix
            R: int, number of rows in the input matrix
        Output: vector, transpose matrix mT of input matrix m
    */
    
    vector <float> mT (C*R);
    
    for(int n = 0; n!=C*R; n++) {
        int i = n/C;
        int j = n%C;
        mT[n] = m[R*j + i];
    }
    
    return mT;
}
