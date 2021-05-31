#include <iostream>
#include <math.h>
#include <chrono>

using namespace std;

long long unsigned int n, k = 1, root, primeCount = 0;

int nextBase(long long unsigned int klocal, bool boolArray[]) {
	bool found = false;
	while(klocal < n && !found) {
		++klocal;
		found = boolArray[klocal];
	}

	return klocal;
}

int main(int argc, char* argv[])
{
	string nStr(argv[argc - 1]);
	n = stoll(nStr);
	
	root = sqrt(n);
	
    bool* bitArray = new bool[n];
    for(long long unsigned int it = 0; it < n; ++it){
        bitArray[it] = true;
    }
    bitArray[0] = false;
    bitArray[1] = false;

	long int time0 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    while(k <= root){
        k = nextBase(k, bitArray);
        if(k <= root){
            for(long long unsigned int it = k + k; it < n; it += k) {
					bitArray[it] = false;
            }
        }
    }
	long int timeTotal = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - time0;
	
	cout << "computation time: " + to_string(timeTotal) << endl;
	
    for(long long unsigned int it = 0; it < n; ++it){
        if(bitArray[it])
            ++primeCount;
    }
	cout << to_string(primeCount) << endl;
	
	FILE* filePrimesPointer = fopen("outputPrimes", "w");
	for(long long unsigned int it = 0; it < n; ++it) {
		if(bitArray[it] == true)
			fprintf(filePrimesPointer, "%llu \n", it);
	}
	fflush(filePrimesPointer);
	fclose(filePrimesPointer); 
}
