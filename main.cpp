// g++ --std=c++11 main.cpp -o main -lutil -L/usr/local/lib -lboost_iostreams -lboost_system -lboost_filesystem

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm> 
#include <math.h>
#include <limits>
#include <cmath>
#include <cstdio>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

// Warn about use of deprecated functions.
#define GNUPLOT_DEPRECATE_WARN
#include "gnuplot-iostream.h"

using namespace std;

#define EXP 2.7182
#define TEMPERATURE 10

random_device rd;
mt19937 gen(rd());

class Guest
{
public:
	int index;
	string name;
		int age; // 0 = 0-12 | 1 = 13-20 | 2 = 21-35 | 3 = 36 - 55 | 4= 56 - inf
		int instructionDegree; // 0 = primaria | 1 = secundaria | 2 = instituto | 3 = universidad | 4 = postgrado
		int familiarDegree;
		int partnerIndex;

	public:
		Guest(int index, string name, int age, string instructionDegree, int familiarDegree, int partnerIndex)
		{
			this->index = index;
			this->name = name;
			this->age = transformAge(age);
			this->instructionDegree = transformInstructionDegree(instructionDegree);
			this->familiarDegree = familiarDegree;
			this->partnerIndex = partnerIndex;
		}

		int transformAge(int age)
		{
			if(age <= 12)
				return 0;
			else if(age <= 20)
				return 1;
			else if(age <= 35)
				return 2;
			else if(age <= 55)
				return 3;
			//else
			return 4;
		}

		int transformInstructionDegree(string & instructionDegree)
		{
			if(instructionDegree == "primaria")
				return 0;
			else if(instructionDegree == "secundaria")
				return 1;
			else if(instructionDegree == "instituto")
				return 2;
			else if(instructionDegree == "universidad")
				return  3;
			//else if(instructionDegree == "postgrado")
			return  4;
		}
	};

	class WeddingSeats
	{
	private:
		int numberOfTables;	
		int numberOfSeatsPerTable;
		vector<Guest> guestsList;	
		vector<int> mutationValues;
		vector<float> mutationWeights;
		piecewise_constant_distribution<> * mutationDistribution;
		uniform_int_distribution<> * tableDistribution;
		string guestsFile;
		vector<int> * tables;
		vector<pair<double, double> > plotDataA;
		vector<pair<double, double> > plotDataB;
		vector<pair<double, double> > plotDataAverage;
		int iterationCounter;

	public:
		WeddingSeats(string guestsFile, int numberOfTables, int numberOfSeatsPerTable)
		{		
			this->tables = new vector<int> [2];
			this->numberOfTables = numberOfTables;
			this->numberOfSeatsPerTable = numberOfSeatsPerTable;
			this->guestsFile = guestsFile;
			this->iterationCounter = 0;

			if(loadGuests())
			{
				cout << "Error : can't load all the guests." << endl;
				return;
			}

			generatePopulation();
			generateMutationValues();
			printPopulation();

			int i = 1000;
			int indexTableReproduction;
			while(--i)
			{
				int indexTableReproduction = probability();
				reproduce(indexTableReproduction);
				mutate(!indexTableReproduction);
				printPopulation();			
			}
			printChart();
			printChartAverage();
		}

		~WeddingSeats()
		{
			delete [] tables;
			delete tableDistribution;
			delete mutationDistribution;
		}

		void generateMutationValues()
		{
			for(int i=0 ; i<numberOfSeatsPerTable ; ++i)
			{
				mutationValues.push_back(i);
				mutationValues.push_back(i);

				mutationWeights.push_back(1.0/(numberOfSeatsPerTable-i+3));
				mutationWeights.push_back(0);
			}
			mutationWeights.pop_back();

			tableDistribution = new uniform_int_distribution<>(0, numberOfTables-1);
		}

		void printChart()
		{
			Gnuplot gp;

			gp << "set title \n";
			gp << "set ylabel 'values'\n";
			gp << "set xlabel 'iterations'\n";
			gp << "plot '-' with lines title 'chromosome1' , '-' with lines title 'chromosome 2'\n";
			gp.send1d(plotDataA);
			gp.send1d(plotDataB);

		}

		void printChartAverage()
		{
			Gnuplot gp;

			gp << "set title \n";
			gp << "set ylabel 'values'\n";
			gp << "set xlabel 'iterations'\n";
			gp << "plot '-' with lines title 'average'\n";
			gp.send1d(plotDataAverage);			

		}

		void printPopulation()
		{
			for(int i=0 ; i<2 ; ++i)
			{
				cout << "Population number " << i+1 << endl;
				for(int j=0 ; j<this->numberOfTables ; ++j)
				{
					cout << "- Table number " << j+1 << " : ";
					for(int k=0 ; k<this->numberOfSeatsPerTable ; ++k)
						cout << this->tables[i][j*this->numberOfSeatsPerTable+k] << " ";
					cout << endl;
				}
				cout << endl;
			}
			this->mutationDistribution = new piecewise_constant_distribution<>(this->mutationValues.begin(), this->mutationValues.end(), this->mutationWeights.begin());
		}

		bool loadGuests()
		{
			fstream data;
			data.open(guestsFile, fstream::in);

			string name, instructionDegree;
			int index, age, familiarDegree, partnerIndex;

			while(data)
			{			
				data >> index >> name >> age >> instructionDegree >> familiarDegree >> partnerIndex;
				guestsList.push_back(Guest(index, name, age, instructionDegree, familiarDegree, partnerIndex));
			}
			data.close();

			return guestsList.size() == (numberOfSeatsPerTable * numberOfTables);
		}

		void generatePopulation()
		{
			vector<int> temp;
			for(int i=0 ; i<numberOfSeatsPerTable*numberOfTables ; ++i)
			{
				temp.push_back(i+1);
			}
			random_shuffle(temp.begin(), temp.end());
			this->tables[0] = temp;
			random_shuffle(temp.begin(), temp.end());
			this->tables[1] = temp;

			for(int i=0 ; i<numberOfTables ; ++i)
			{
				sortTable(0, i);
				sortTable(1, i);
			}
		}

		int probability()
		{
			float sum = 0.0;
			float valueTableA = 0.0;
			float valueTableB = 0.0;

			for(int i=0 ; i<this->numberOfTables ; ++i)
			{
				vector<int> distancesA = evaluateTable(0, i);
				vector<int> distancesB = evaluateTable(1, i);
				for(int j=0 ; j<this->numberOfSeatsPerTable ; ++j)
				{
					valueTableA = distancesA[j];
					valueTableB = distancesB[j];
				}
			}			

			valueTableA = sqrt(valueTableA);
			valueTableB = sqrt(valueTableB);

			plotDataA.push_back(make_pair(iterationCounter, valueTableA));
			plotDataB.push_back(make_pair(iterationCounter, valueTableB));
			plotDataAverage.push_back(make_pair(iterationCounter++, (valueTableB+valueTableA)/2));

			sum = pow(EXP, valueTableA/TEMPERATURE) + pow(EXP, valueTableB/TEMPERATURE);
			vector<int> values = { 0,0,1,1 };
			vector<float> weights =  {  pow(EXP, 1.0*valueTableA/TEMPERATURE)/sum,  0, pow(EXP, 1.0*valueTableB/TEMPERATURE)/sum};

			piecewise_constant_distribution<> d(values.begin(), values.end(), weights.begin());

			return d(gen);
		}

		vector<int> evaluateTable(int indexTable, int table)
		{
			vector<int> distanceValues(numberOfSeatsPerTable);

			for(int i=0 ; i<numberOfSeatsPerTable ; ++i)
			{
				int sum = 0;
				Guest * tmp = & this->guestsList[this->tables[indexTable][numberOfSeatsPerTable*table+i]];
				for(int j=0 ; j<numberOfSeatsPerTable ; ++j)
				{
					Guest * tmp2 = & this->guestsList[this->tables[indexTable][numberOfSeatsPerTable*table+j]];
					sum += (tmp->age - tmp2->age) * (tmp->age - tmp2->age);
					sum += (tmp->instructionDegree - tmp2->instructionDegree) * (tmp->instructionDegree - tmp2->instructionDegree);
					sum += (tmp->familiarDegree - tmp2->familiarDegree) * (tmp->familiarDegree - tmp2->familiarDegree);
				}
				distanceValues[i] = sum;
			}

			return distanceValues;
		}

		void sortTable(int indexTable, int table)
		{
			vector<int> distanceValues = evaluateTable(indexTable, table);

			for(int i=0 ; i<numberOfSeatsPerTable ; ++i)
			{
				vector<int>::iterator it = max_element(distanceValues.begin(), distanceValues.end());
				int index = distance(distanceValues.begin(), it);
				swap(this->tables[indexTable][numberOfSeatsPerTable*table+i], this->tables[indexTable][numberOfSeatsPerTable*table+index]);
				distanceValues[index] = -1;
			}

		}

		void mutate(int indexTable)
		{		
			int mutationPosition = (*mutationDistribution)(gen);		

			int tableA = (*tableDistribution)(gen);
			int tableB;
			do
			{
				tableB = (*tableDistribution)(gen);
			}while(tableA == tableB);

			swap(this->tables[indexTable][numberOfSeatsPerTable*tableA+mutationPosition], this->tables[indexTable][numberOfSeatsPerTable*tableB+mutationPosition]);

			sortTable(indexTable, tableA);
			sortTable(indexTable, tableB);
		}

		void reproduce(int indexTable)
		{
			int reproductionPosition = (*mutationDistribution)(gen);

			vector<int> higherValues(this->numberOfTables*this->numberOfSeatsPerTable, -1);
			vector<int> temp;
			vector<int> lowerValues;

			for(int i=0, j; i<this->numberOfTables ; ++i)
			{
				for (j = 0; j <= reproductionPosition; ++j)
				{
					higherValues[i*this->numberOfSeatsPerTable+j] = this->tables[indexTable][i*this->numberOfSeatsPerTable+j];
					temp.push_back(this->tables[!indexTable][i*this->numberOfSeatsPerTable+j]);
				}
				for( ; j<this->numberOfSeatsPerTable ; ++j)
				{
					lowerValues.push_back(this->tables[!indexTable][i*this->numberOfSeatsPerTable+j]);
				}
			}

			random_shuffle(lowerValues.begin(), lowerValues.end());

			for(int i=0 ; i<temp.size() ; ++i)
			{
				lowerValues.push_back(temp[i]);
			}

			vector<int>::iterator it = find(higherValues.begin(), higherValues.end(), -1);
			vector<int>::iterator it2;
			for(int j=0; j<lowerValues.size() and it != higherValues.end(); ++j)
			{
				it2 = find(higherValues.begin(), higherValues.end(), lowerValues[j]);
				if(it2 == higherValues.end())
				{
					(*it) = lowerValues[j];
					it = find(it, higherValues.end(), -1);
				}
			}
			this->tables[indexTable] = higherValues;

			for(int i=0 ; i<numberOfTables ; ++i)
			{
				sortTable(indexTable, i);
			}
		}
	};

	int main(int argc, char const *argv[])
	{
		WeddingSeats myWedding("guests.csv", 4, 8);
		return 0;
	}