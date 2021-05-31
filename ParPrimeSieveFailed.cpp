#include <iostream>
#include "mpi.h"
#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <math.h>

//MPI_Bcast(&val, 1, MPI_INT, rank, MPI_COMM_WORLD);
using namespace std;

int ROOT_RANK = 0;
int CHECK_TAG = 1;
int	MUL_TAG = 0;

bool goodBase;
int n, k = 1, kCheck = 0, root, pRank = -1, count = -1;
bool* bitArray;
bool* exchangeBitArray;
long int comTime0, comTimeReceived, comTimeTotal = 0, comCounter = 0;

int nextBase(int klocal) {
	bool found = false;
	while(!found) {
		found = true;
		//odpytywanie node'ow czy proponowana l. pierwsza jest u nich skreslona
		for(int it = 1; it < count; ++it){
			kCheck = klocal;
			
			MPI_Send(&kCheck, 1, MPI_INT, it, CHECK_TAG, MPI_COMM_WORLD);
			comTime0 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
			MPI_Recv(&kCheck, 1, MPI_INT, it, CHECK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			comTimeReceived = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - comTime0;
			comTimeTotal += comTimeReceived;
			++comCounter;
			if(kCheck == 0){
				found = false;
				++klocal;
				break;
			}
		}
	}
	
	return klocal;
}

void checkLoopMethod(){
	while(true){
		MPI_Recv(&kCheck, 1, MPI_INT, ROOT_RANK, CHECK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(kCheck == 0)
			break;
		
		if(kCheck % k == 0 || !exchangeBitArray[kCheck]){
			kCheck = 0;
		}else {
			kCheck = 1;
		}
		MPI_Send(&kCheck, 1, MPI_INT, ROOT_RANK, CHECK_TAG, MPI_COMM_WORLD);
	}
}

int main(int argc, char* argv[]){
	int* response;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, response);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &pRank);
	MPI_Comm_size(MPI_COMM_WORLD, &count);
	
	if(pRank == 0){
		string nStr(argv[argc - 1]);
		n = stoi(nStr);
		bitArray = new bool[n];
		exchangeBitArray = new bool[n];
		root = sqrt(n);
		k = 1;
		
		//inicjalizacja
		for(int it = 0; it < n; ++it){
			bitArray[it] = true;
		}
		bitArray[0] = false;
		bitArray[1] = false;
		
		//powiadamianie o rozmiarze instancji problemu
		for(int it = 1; it < count; ++it){
			MPI_Send(&n, 1, MPI_INT, it, MUL_TAG, MPI_COMM_WORLD);
		}
		
		//znajdowanie i wysylanie liczb pierwszych
		long int time0 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
		while(k <= root){
			for(int it = 1; it < count; ++it){
				k = nextBase(++k);
				
				//jezeli znaleziona l. pierwsza <= n^0.5 to wyslij 
				if(k <= root)
					MPI_Send(&k, 1, MPI_INT, it, MUL_TAG, MPI_COMM_WORLD);
				else
					break;
			}
		}
		long int timeTotal = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - time0;
		cout << "time: " + to_string(timeTotal) + "ms" << endl;
		cout << "time spent on communication(recv): " + to_string(comTimeTotal) + "ms" << endl;
		cout << "times recv was called): " + to_string(comCounter)<< endl;
		
		//wysylanie zer
		k = 0;
		kCheck = 0;
		for(int it = 1; it < count; ++it){
			//zakonczenie watku sprawdzajacego
			MPI_Send(&kCheck, 1, MPI_INT, it, CHECK_TAG, MPI_COMM_WORLD);
			//zakonczenie mnozenia
			MPI_Send(&k, 1, MPI_INT, it, MUL_TAG, MPI_COMM_WORLD);
			//odbior posredniej tablicy wynikowej
			MPI_Recv(exchangeBitArray, n, MPI_C_BOOL, it, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//redukcja do tablicy glownej
			for(int itt = 0; itt < n; ++itt)
				bitArray[itt] = bitArray[itt] && exchangeBitArray[itt];
		}
		
		//zapis do pliku
		FILE* filePrimesPointer = fopen("outputPrimes", "w");
		for(int it = 0; it < n; ++it) {
			if(bitArray[it] == true)
				fprintf(filePrimesPointer, "%i \n", it);
		}
		fflush(filePrimesPointer);
		fclose(filePrimesPointer);
			
	}else
	if(pRank != 0){
		//odebranie rozmiaru instancji problemu
		MPI_Recv(&n, 1, MPI_INT, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		root = sqrt(n);
		
		//inicjalizacja posredniej tablicy wynikowej
		exchangeBitArray = new bool[n];
		for(int it = 0; it < n; ++it){
			exchangeBitArray[it] = true;
		}
		
		//inicjalizacja watku odpowiadajacego na zapytania o poprawnosc znalezionych liczb pierwszych
		k = n + 1;
		thread checkThread(checkLoopMethod);
		
		do{
			//odebranie kolejnego k(l. pierwsza) do skreslania
			MPI_Recv(&k, 1, MPI_INT, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			if(k > 1) {
				for(int it = k + k; it < n; it += k){
					exchangeBitArray[it] = false;
				}
			}
		}while(k);
		
		checkThread.join();
		
		//wyslanie posredniej talbicy wynikowej do nadzorcy
		MPI_Send(exchangeBitArray, n, MPI_C_BOOL, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD);
	}
	
	delete[] bitArray;
	delete[] exchangeBitArray;
		
	
	MPI_Finalize();
	
	return 0;
}