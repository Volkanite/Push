#include <sl.h>
#include <push.h>

#include "Nvidia\nvapi.h"


int numTempPoints;
int numColumns;
int **arr;


void InitializeFanSettings()
{
	int fanHigh = 80;
	int fanLow = 30;
	int tempMax = 80;
	int tempMin = 24;
	int numFanPoints;	
	int temp;
	float fanIncrease;
	float fanSpeed;

	numFanPoints = fanHigh - fanLow;
	numTempPoints = tempMax - tempMin;
	fanIncrease = (float) numFanPoints / (float) numTempPoints;

	fanSpeed = (float) fanLow;
	temp = tempMin;

	arr = Memory_Allocate((numTempPoints+1) * sizeof(int *));
	numColumns = 2;

	if (arr == NULL)
	{
		Log(L"out of memory");
		return;
	}

	for (int i = 0; i < numTempPoints+1; i++)
	{
		arr[i] = Memory_Allocate(numColumns * sizeof(int));
	}

	int j = 0;

	for (int i = 0; i < numTempPoints+1; i++)
	{
		for (j = 0; j < numColumns; j++)
		{
			arr[i][0] = temp;
			arr[i][1] = (int) fanSpeed;
		}

		fanSpeed += fanIncrease;
		temp++;
	}

	/*for (int i = 0; i < numTempPoints+1; i++)
	{
		Log(L"[temp] = %i [fanSpeed] = %i", arr[i][0], arr[i][1]);
	}*/
}


UINT32 GetSpeedFromMap( UINT32 Temp )
{
	for (int i = 0; i < numTempPoints + 1; i++)
	{
		for (int j = 0; j < numColumns; j++)
		{
			if (arr[i][0] == Temp)
			{
				return arr[i][1];
			}
		}
	}

	return 80;
}


void UpdateFanSpeed()
{
	int temp;
	int fanSpeed;
	int speedForTemp;

	temp = Nvapi_GetTemperature();
	fanSpeed = Nvapi_GetFanDutyCycle();
	speedForTemp = GetSpeedFromMap(temp);

	//Log(L"temp %i, fan %i, sft %i", temp, fanSpeed, speedForTemp);

	if (fanSpeed != speedForTemp)
	{
		Nvapi_SetFanDutyCycle(speedForTemp);
	}
}