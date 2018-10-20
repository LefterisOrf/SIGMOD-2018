#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include "funcs.h"

// Takes a table and converts it to an array of tuples for faster join access 
relation* ToRow(int** OriginalArray, int RowToJoin, relation* NewRel)
{
	for (int i = 0; i < NewRel->num_tuples; ++i)
	{
		NewRel->tuples[i].Value = OriginalArray[i][RowToJoin];
		NewRel->tuples[i].RowId = i;
	}
	return NewRel;
}


/*
 * Reorders an row stored array so it is sorted based on the values that belong to the 
 * same bucket. Stores the ordered array , the histogram and the Psum array in a 
 * ReorderedRelation variable.
 */
void ReorderArray(relation* RelArray, int n_lsb, ReorderedRelation** NewRel)
{
	//Check the arguments
	if ((RelArray == NULL) || (RelArray->num_tuples <= 0) || (n_lsb <= 0))
	{
		printf("Error in ReorderArray. Invalid arguments\n");
		exit(1);
	}
	int i = 0, flag = 0;
	uint32_t HashedValue = 0;

	(*NewRel) = malloc(sizeof(ReorderedRelation));
	(*NewRel)->Hist_size = -1;

	//Find the size of the Psum and the Hist arrays
	(*NewRel)->Hist_size = 1;
	for (i = 0; i < n_lsb; ++i)
	{
		(*NewRel)->Hist_size *= 2;
	}
	
	// Allocate space for the Hist and Psum arrays
	(*NewRel)->Psum = malloc((*NewRel)->Hist_size * sizeof(int*));
	(*NewRel)->Hist = malloc((*NewRel)->Hist_size * sizeof(int*));
	for (i = 0; i < (*NewRel)->Hist_size; ++i)
	{
		(*NewRel)->Psum[i] = malloc(2 * sizeof(int));
		(*NewRel)->Psum[i][0] = i; //Bucket number
		(*NewRel)->Psum[i][1] = -1; //Initialize the starting point of each bucket in the reordered array to -1

		(*NewRel)->Hist[i] = malloc(2 * sizeof(int));
		(*NewRel)->Hist[i][0] = i; // Bucket number
		(*NewRel)->Hist[i][1] = 0; // Each bucket starts with 0 allocated values
	}

	//Run RelArray with the hash function and build the histogram
	for (i = 0; i < RelArray->num_tuples; ++i)
	{
		HashedValue = hash_function_1((int32_t) RelArray->tuples[i].Value, n_lsb);
		(*NewRel)->Hist[HashedValue][1]++;
	}

	//Build the Psum array using the histogram
	int NewStartingPoint = 0;
	for (i = 0; i < (*NewRel)->Hist_size; ++i)
	{
		if ((*NewRel)->Psum[i][1] != -1)
		{
			printf("Error in initialization of Psum\n");
			exit(1);
		}
		/*
		 *If the current bucket has 0 values allocated to it then leave 
		 *Psum[CurrentBucket][1] to -1.
		*/
		if ((*NewRel)->Hist[i][1] > 0)
		{
			(*NewRel)->Psum[i][1] = NewStartingPoint;
			NewStartingPoint += (*NewRel)->Hist[i][1];
		}
	}
	/*
	printf("Hist:\n");
	for (i = 0; i < (*NewRel)->Hist_size; ++i)
	{
		printf("%d %d\n", (*NewRel)->Hist[i][0], (*NewRel)->Hist[i][1]);
	}
	printf("--------------------------------------\n");
	printf("Psum:\n");
	for (i = 0; i < (*NewRel)->Hist_size; ++i)
	{
		printf("%d %d\n", (*NewRel)->Psum[i][0], (*NewRel)->Psum[i][1]);
	}
	*/

	/*--------------------Build the reordered array----------------------*/

	//Allocate space for the ordered array in NewRel variable
	(*NewRel)->RelArray = malloc(sizeof(relation));
	(*NewRel)->RelArray->num_tuples = RelArray->num_tuples;
	(*NewRel)->RelArray->tuples = malloc(RelArray->num_tuples * sizeof(tuple));

	//Initialize the array
	for (i = 0; i < RelArray->num_tuples; ++i)
	{
		(*NewRel)->RelArray->tuples[i].Value = -1;
		(*NewRel)->RelArray->tuples[i].RowId = -1;
	}
	int StartingPoint = 0;
	int NextActiveBucket = 0;
	int UpperBound = -1;

	//Traverse through the original array
	for (i = 0; i < RelArray->num_tuples; ++i)
	{
		//Find the hash value of the current tuple
		HashedValue = hash_function_1((int32_t) RelArray->tuples[i].Value, n_lsb);
		//Using the hash value find the starting point using the Psum array
		StartingPoint = (*NewRel)->Psum[HashedValue][1];
		if (StartingPoint < 0 || StartingPoint > RelArray->num_tuples)
		{
			printf("Error, hash value is outside of array borders!\n");
			exit(1);
		}
		/*
		 *UpperBound is the starting point of the bucket + the 
		 *numofvalues that this bucket has on the Hist array 
		 */
		UpperBound = StartingPoint + (*NewRel)->Hist[HashedValue][1];
		/*
		 *We now have our starting point and the upper bound so 
		 *we can insert the value in the ordered array safely
		*/
		flag = 0;
		for (int j = StartingPoint; j < UpperBound; ++j)
		{
			if ((*NewRel)->RelArray->tuples[j].Value == -1)
			{
				(*NewRel)->RelArray->tuples[j].Value = RelArray->tuples[i].Value;
				(*NewRel)->RelArray->tuples[j].RowId = RelArray->tuples[i].RowId;
				flag = 1;
				break;
			}
		}
		if (flag == 0 )
		{
			printf("Error! StartingPoint = %d and UpperBound = %d\n",StartingPoint, UpperBound);
		}
	}
}


int main(int argc, char const *argv[])
{
	// Malloc and initialize the original array
	int NumOfRows = 10;
	int NumOfColumns = 50;
	int **OriginalArray = malloc(NumOfRows * sizeof(int*));
	relation* NewRel = malloc(sizeof(relation));
	NewRel->tuples = malloc(NumOfRows * sizeof(tuple));
	NewRel->num_tuples = NumOfRows;
	for (int i = 0; i < NumOfRows; ++i)
	{
		NewRel->tuples[i].Value = -1;
		NewRel->tuples[i].RowId = -1;
		OriginalArray[i] = malloc(NumOfColumns * sizeof(int));
		for (int j = 0; j < NumOfColumns; ++j)
		{
			OriginalArray[i][j] = (i*j)/3;
		}
	}

	relation* NewRel2 = malloc(sizeof(relation));
	NewRel2->tuples = malloc(NumOfRows * sizeof(tuple));
	NewRel2->num_tuples = NumOfRows;
	for (int i = 0; i < NumOfRows; ++i)
	{
		NewRel2->tuples[i].Value = -1;
		NewRel2->tuples[i].RowId = -1;
		OriginalArray[i] = malloc(NumOfColumns * sizeof(int));
		for (int j = 0; j < NumOfColumns; ++j)
		{
			OriginalArray[i][j] = (i*j)/3;
		}
	}

	//Transform the original array to row stored array using relation struct
	NewRel = ToRow(OriginalArray, 4, NewRel);
	NewRel2 = ToRow(OriginalArray, 2, NewRel2);
	/*for (int i = 0; i < 10; ++i)
	{
		printf("%d %d\n",NewRel->tuples[i].RowId,NewRel->tuples[i].Value );
	}
	for (int i = 0; i < 10; ++i)
	{
		printf("%d %d\n",NewRel2->tuples[i].RowId,NewRel2->tuples[i].Value );
	}*/



	ReorderedRelation* Reordered = malloc(sizeof(ReorderedRelation));
	Reordered->Hist_size = -1;
	
	//Reorder the original array and store it in Reordered 
	/*ReorderArray(NewRel, 3, &Reordered);

	for (int i = 0; i < NewRel->num_tuples; ++i)
	{
		printf("%2d %2d || %2d %2d\n", NewRel->tuples[i].Value, NewRel->tuples[i].RowId, Reordered->RelArray->tuples[i].Value, Reordered->RelArray->tuples[i].RowId);
	}*/

	RadixHashJoin(NewRel,NewRel2);

	/*-------------------------Free allocated space-------------------------*/


	//Free Original array
	for (int i = 0; i < NumOfRows; ++i)
	{
		free(OriginalArray[i]);
	}
	free(OriginalArray);

	//Free NewRel
	free(NewRel->tuples);
	free(NewRel);

	//Free Reordered
	/*if (Reordered != NULL)
	{
		//Free Psum and Hist
		if (Reordered->Hist_size != -1)
		{
			for (int i = 0; i < Reordered->Hist_size; ++i)
			{
				free(Reordered->Hist[i]);
				free(Reordered->Psum[i]);
			}
			free(Reordered->Psum);
			free(Reordered->Hist);
		}
		//Free RelArray
		if (Reordered->RelArray != NULL)
		{
			if (Reordered->RelArray->tuples != NULL)free(Reordered->RelArray->tuples);
			free(Reordered->RelArray);
		}
		free(Reordered);
	}*/
}
