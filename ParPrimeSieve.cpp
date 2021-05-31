#include <iostream>
#include "mpi.h"
#include <string>
#include <cstdlib>
#include <chrono>
#include <math.h>

using namespace std;

int ROOT_RANK = 0;
int	MUL_TAG = 0;

long long unsigned int n, k = 1, root;
int pRank = -1, count = -1;
bool* bitArray;
bool* exchangeBitArray;
long int comTime0, comTimeReceived, comTimeTotal = 0, comCounter = 0;

int nextBase(long long unsigned int klocal, bool boolArray[]) {
	bool found = false;
	//szukanie nieskreslonego indeksu/liczby,
	//dopoki indeks/liczba <= n^0.5 AND wartosc komorki o indeksie klocal == false
	while(klocal <= root && !found) {
		++klocal;
		found = boolArray[klocal];
	}
	
	return klocal;
}


int main(int argc, char* argv[]){
	//inicjalizacja komunikacji w ramach MPI
	MPI_Init(&argc, &argv);
	
	//pobranie rangi oraz ilosci node'ow
	MPI_Comm_rank(MPI_COMM_WORLD, &pRank);
	MPI_Comm_size(MPI_COMM_WORLD, &count);
	
	//wykonywane przez pierwszy proces/zarzadce
	if(pRank == 0){
		string nStr(argv[argc - 1]);
		n = stoll(nStr);
		bitArray = new bool[n];
		exchangeBitArray = new bool[n];
		root = sqrt(n);
		k = 1;
		
		//informowanie o rozmiarze instancji problemu
		for(int it = 1; it < count; ++it){
			MPI_Send(&n, 1, MPI_UNSIGNED_LONG_LONG, it, MUL_TAG, MPI_COMM_WORLD);
		}
		
		//inicjalizacja tablicy wynikowej
		for(long long unsigned int it = 0; it < n; ++it){
			bitArray[it] = true;
		}
		//nie podlegaja rozpatrzeniu ale moga byc wypisane do pliku, nalezy nadpisac recznie
		bitArray[0] = false;
		bitArray[1] = false;
		
		//wysylanie znalezionych l. pierwszych node'om- sito eratostenesa do n^0.5
		long int time0 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
		while(k <= root) {
			for(int node = 1; k <= root && node < count; ++node){
				k = nextBase(k, bitArray);
				if(k <= root){
					MPI_Send(&k, 1, MPI_UNSIGNED_LONG_LONG, node, MUL_TAG, MPI_COMM_WORLD);
					for(long long unsigned int it = k + k; it <= root; it += k) {
						bitArray[it] = false;
					}
				}
			}
		}
		
		//zakonczenie wysylania l. pierwszych
		k = 0;
		for(int it = 1; it < count; ++it){
			MPI_Send(&k, 1, MPI_UNSIGNED_LONG_LONG, it, MUL_TAG, MPI_COMM_WORLD);
		}
		long int timeTotal = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - time0;
		cout << "sending time: " + to_string(timeTotal) + "ms\n";
		
		//czekanie, pobieranie informacji o zakonczeniu obliczen
		for(int it = 1; it < count; ++it){
			MPI_Recv(&k, 1, MPI_UNSIGNED_LONG_LONG, it, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		timeTotal = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - time0;
		cout << "timeTotal: " + to_string(timeTotal) + "ms\n";
		
		//pobieranie posrednich tablic wynikowych oraz redukcja do glownej tablicy
		for(int it = 1; it < count; ++it){
			MPI_Recv(exchangeBitArray, n, MPI_C_BOOL, it, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			cout << "received exchangeBitArray from node " + to_string(it) + ".\n";
			for(long long unsigned int itt = root + 1; itt < n; ++itt)
				bitArray[itt] = bitArray[itt] && exchangeBitArray[itt];
		}
		
		//wypisanie wynikow do pliku
		FILE* filePrimesPointer = fopen("outputPrimes", "w");
		for(long long unsigned int it = 0; it < n; ++it) {
			if(bitArray[it] == true)
				fprintf(filePrimesPointer, "%llu \n", it);
		}
		fflush(filePrimesPointer);
		fclose(filePrimesPointer);
		
	}else//wykonywane przez wszystkie procesy poza pierwszym/zarzadca
	if(pRank != 0){
		//odebranie informacji o rozmiarze instancji problemu
		MPI_Recv(&n, 1, MPI_UNSIGNED_LONG_LONG, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		root = sqrt(n);
		
		//inicjalizacja posredniej tablicy wynikowej; komorki o indeksach <= n^0.5 maja wartosc "false", reszta ma wartosc "true".
		//wynika to ze sposobu zapisu przekazanych node'owi l. pierwszych - komorki posredniej tablicy wynikowej o indeksach l. pierwszych
		//maja wartosc "true" oraz najwieksza otrzymana l. pierwsza <= n^0.5. Ulatwia to znalezienie ich metoda nextBase(...);
		//Pozostaje jedynie "skreslic" wielokrotnosci l. pierwszych wiekszych od n^0.5, jako ze nadzorca obliczyl l. pierwsze <= n^0.5
		exchangeBitArray = new bool[n];
		for(long long unsigned int it = 0; it <= root; ++it){
			exchangeBitArray[it] = false;
		}
		for(long long unsigned int it = root + 1; it < n; ++it){
			exchangeBitArray[it] = true;
		}
		
		
		//odbieranie l. pierwszych
		k = 1;
		while(k != 0){
			MPI_Recv(&k, 1, MPI_UNSIGNED_LONG_LONG, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			exchangeBitArray[k] = true;
		}
		
		//przekazane k == 0 oznacza koniec przekazywania l. pierwszych, nalezy skorygowac zaznaczona komorke
		exchangeBitArray[0] = false;
		
		//skreslanie
		long long unsigned int firstProductBiggerThanRoot, multiplicant;
		while(k <= root){
			k = nextBase(k, exchangeBitArray);
			if(k <= root){
				firstProductBiggerThanRoot = root - 1;
				multiplicant = 1;
				while(firstProductBiggerThanRoot <= root)
					firstProductBiggerThanRoot = k * ++multiplicant;
				for(long long unsigned int it = firstProductBiggerThanRoot; it < n; it += k) {
					exchangeBitArray[it] = false;
				}
			}
		}
		
		//wyslanie zmierzonego czasu oraz posredniej tablicy wynikowej
		MPI_Send(&k, 1, MPI_UNSIGNED_LONG_LONG, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD);
		MPI_Send(exchangeBitArray, n, MPI_C_BOOL, ROOT_RANK, MUL_TAG, MPI_COMM_WORLD);
	}
	
	delete[] bitArray;
	delete[] exchangeBitArray;
	
	//finalizacja komunikacji w ramach standardu MPI
	MPI_Finalize();
	
	return 0;
}